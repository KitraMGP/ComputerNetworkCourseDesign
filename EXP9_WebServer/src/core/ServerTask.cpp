#include <ServerTask.h>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <sys/socket.h>
#include <errno.h>

using std::string;

ServerTask::ServerTask(int clientSock, QString serverRoot, QString clientInfo, QObject* parent) : QObject(parent) {
    this->clientSock = clientSock;
    if (serverRoot.endsWith("/")) {
        serverRoot = serverRoot.left(serverRoot.length() - 1);
    }
    this->serverRoot = serverRoot;
    this->clientInfo = clientInfo;
    // 设置自动删除
    setAutoDelete(true);
}

void ServerTask::run() {
    char* recvBuf = new char[8192];
    QByteArray requestData;
    HttpRequest* httpRequest;

    // 设置 30s 接收超时
    timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    if (setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        qDebug() << "设置 socket 接收超时失败";
    }

    // 接收客户端发来的数据
    while (true) {
        int recvlen = recv(clientSock, recvBuf, sizeof(recvBuf), 0);
        if (recvlen < 0) {
            // 检查是否是超时或暂时性错误
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // 超时或被信号中断，正常退出长连接
                break;
            } else {
                // 真正的错误
                qDebug() << "接收数据发生错误：" << strerror(errno);
            }
            break;
        } else if (recvlen == 0) {
            qDebug() << "客户端关闭连接";
            break;  // 连接关闭
        }
        requestData.append(recvBuf, recvlen);

        // 若接收到了完整的 HTTP 请求报文则处理请求
        httpRequest = parseRequest(requestData);
        if (httpRequest) {
            this->keepAlive = httpRequest->keepAlive;
            processHttpRequest(httpRequest);

            // 如果是长连接，保留连接继续接收下一次请求
            if (keepAlive) {
                requestData.clear();
                delete httpRequest;
                httpRequest = nullptr;
                continue;
            }
        }
    }
    
    // 连接关闭
    delete recvBuf;
    if (httpRequest) {
        delete httpRequest;
    }
    // 操作结束
    emit taskFinished(clientSock);
}

HttpRequest* ServerTask::parseRequest(const QByteArray& data) {
    // 查找请求头结束标记 \r\n\r\n
    int headerEnd = data.indexOf("\r\n\r\n");
    if (headerEnd == -1) {
        return nullptr;   // 头部不完整
    }

    // 解析请求头
    QByteArray headerData = data.left(headerEnd);
    QList<QByteArray> headerLines = headerData.split('\n');

    // 查找 Content-Length
    int contentLength = 0;
    for (QByteArray& line : headerLines) {
        if (line.startsWith("Content-Length:")) {
            QList<QByteArray> parts = line.split(':');
            if (parts.size() >= 2) {
                contentLength = parts[1].trimmed().toInt();
            }
            break;
        }
    }

    // 查找 Connection
    bool keepAlive = false;
    for (QByteArray& line : headerLines) {
        if (line.startsWith("Connection:")) {
            keepAlive = line.contains("keep-alive");
            break;
        }
    }

    // 检查请求体是否完整
    int bodyStart = headerEnd + 4;
    if (data.size() >= bodyStart + contentLength) {
        HttpRequest* result = new HttpRequest();
        result->body = data.mid(bodyStart, contentLength);
        result->contentLength = contentLength;
        result->headerLines = headerLines;
        result->keepAlive = keepAlive;
        return result;
    } else {
        return nullptr;
    }
}

void ServerTask::processHttpRequest(HttpRequest* request) {
    // 获取 HTTP 请求方法和路径
    QList<QByteArray> firstLineParts = request->headerLines[0].split(' ');
    if (firstLineParts.size() < 2) {
        // 第一行格式不正确
        QByteArray content;
        content.append("<html><head><meta charset=\"UTF-8\"></head><body><h1>400 Bad Request</h1><p>请求无效</p></body></html>");
        sendResponse(400, "Bad Request", &content, "text/html", "");
        return;
    }

    QString method = firstLineParts[0];
    QString path = firstLineParts[1];

    if (method == "GET" || method == "HEAD") {
        QString filePath = getFilesystemPath(path);
        if (filePath == "404") {
            QByteArray content;
            content.append(
                QString("<html><head><meta charset=\"UTF-8\"></head><body><h1>404 Not Found</h1><p>请求的资源 %1 不存在</p></body></html>").arg(path).toUtf8());
            sendResponse(404, "Not Found", &content, "text/html", "");
            return;
        } else if (filePath == "403") {
            QByteArray content;
            content.append(QString("<html><head><meta charset=\"UTF-8\"></head><body><h1>403 Forbidden</h1><p>你没有权限访问位于 %1 的资源</p></body></html>").arg(path).toUtf8());
            sendResponse(404, "Not Found", &content, "text/html", "");
            return;
        }

        emit logMessage(QString("已处理来自%1的请求，方法：%2，路径：%3").arg(clientInfo).arg(method).arg(path));

        QByteArray content;
        QString mimeType = getMimeType(filePath);
        QString date = getDate(filePath);

        if (method == "GET") {
            // 读取文件
            QFile file(filePath);
            if (file.open(QIODeviceBase::ReadOnly)) {
                content = file.readAll();
                sendResponse(200, "OK", &content, mimeType, date);
            } else {
                content.append(QString("<html><head><meta charset=\"UTF-8\"></head><body><h1>403 Forbidden</h1><p>你没有权限访问位于 %1 的资源</p></body></html>").arg(path).toUtf8());
                sendResponse(404, "Not Found", &content, "text/html", "");
            }
        } else {    // HEAD 方法
            // 获取文件大小
            QFileInfo fileInfo(filePath);
            sendResponse(200, "OK", nullptr, mimeType, date, fileInfo.size());
        }
        
    } else if (method == "POST") {
        // 测试 POST 接口
        if (path == "/testPostApi") {
            // 读取表单
            QList<QByteArray> fields = request->body.split('&');
            QString name;
            if (fields.size() > 0) {
                // 找到name字段
                for (QString field : fields) {
                    if (field.startsWith("name=")) {
                        name = field.mid(5, field.size() - 1);
                        break;
                    }
                }
            }
            QByteArray content;
            content.append(QString("Hello %1!").arg(name).toUtf8());
            sendResponse(200, "OK", &content, "text/plain", "");
        } else {
            QByteArray content;
            content.append(
                QString("<html><head><meta charset=\"UTF-8\"></head><body><h1>404 Not Found</h1><p>请求的资源 %1 不存在</p></body></html>").arg(path).toUtf8());
            sendResponse(404, "Not Found", &content, "text/html", "");
            return;
        }
    } else {
        QByteArray content;
        content.append(QString("<html><head><meta charset=\"UTF-8\"></head><body><h1>405 Method Not Allowed</h1><p>不支持使用 %1 方法</p></body></html>").arg(method).toUtf8());
        sendResponse(404, "Not Found", &content, "text/html", "");
    }

    return;
}

bool ServerTask::sendResponse(int code, QString description, QByteArray* content, QString contentType, QString date, int length) {
    // 状态行
    QString statusLine = QString("HTTP/1.1 %1 %2\r\n").arg(code).arg(description);
    // 响应头
    QString header = QString("Server: MyCustomServer\r\n");

    // Content-Length
    if (content != nullptr) {    // 普通有响应体的响应
        header = header.append(QString("Content-Length: %1\r\n").arg(content->size()));
    } else if (length > 0) {    // HEAD 方法
        header = header.append(QString("Content-Length: %1\r\n").arg(length));
    }

    // Content-Type
    if (!contentType.isEmpty()) {
        header = header.append(QString("Content-Type: %1\r\n").arg(contentType));
    }

    // Date
    if (!date.isEmpty()) {
        header = header.append(QString("Date: %1\r\n").arg(date));
    }

    // Connection
    if (this->keepAlive) {
        header = header.append("Connection: keep-alive\r\n");
    } else {
        header = header.append("Connection: close\r\n");
    }

    header = header.append("\r\n");

    // 发送数据
    send(clientSock, statusLine.toUtf8().constData(), statusLine.toUtf8().size(), 0);
    send(clientSock, header.toUtf8().constData(), header.toUtf8().size(), 0);
    if (content != nullptr && content->size() > 0) {
        send(clientSock, content->constData(), content->size(), 0);
    }
    return true;
}

QString ServerTask::getFilesystemPath(QString requestPath) {
    if (!requestPath.startsWith("/")) {
        return "404";
    }
    if (requestPath == "/") {
        requestPath = "/index.html";
    }
    if (requestPath.endsWith("/")) {
        requestPath += "index.html";
    }
    // 判断是否是非法路径
    if (requestPath.contains("..")) {
        return "403";
    }
    QString filesystemPath = serverRoot + requestPath;
    // 查询文件信息
    QFileInfo info(filesystemPath);
    if (!info.exists() || info.isDir()) {
        return "404";
    }
    return info.absoluteFilePath();
}

QString ServerTask::getMimeType(QString& filePath) {
    QFileInfo fileInfo(filePath);
    return mimeDatabase.mimeTypeForFile(fileInfo).name();
}

QString ServerTask::getDate(QString& filePath) {
    QFileInfo fileInfo(filePath);
    return fileInfo.lastModified().toString(Qt::RFC2822Date);
}