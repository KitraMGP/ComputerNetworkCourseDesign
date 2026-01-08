#include <ServerTask.h>
#include <QDebug>
#include <sys/socket.h>
#include <errno.h>

using std::string;

ServerTask::ServerTask(int clientSock, QString clientInfo, QObject* parent) : QObject(parent) {
    this->clientSock = clientSock;
    this->clientInfo = clientInfo;
    // 设置自动删除
    setAutoDelete(true);
}

void ServerTask::run() {
    char* recvBuf = new char[8192];
    QByteArray requestData;
    HttpRequest* httpRequest;
    
    // 接收客户端发来的数据
    while (true) {
        int recvlen = recv(clientSock, recvBuf, sizeof(recvBuf), 0);
        if (recvlen < 0) {
            qDebug() << "接收数据发生错误：" << strerror(errno);  // 发生错误
        } else if (recvlen == 0) {
            break;  // 连接关闭
        }
        requestData.append(recvBuf, recvlen);

        // 若接收到了完整的 HTTP 请求报文则结束循环
        httpRequest = parseRequest(requestData);
        if (httpRequest) {
            break;
        }
    }

    // 处理 HTTP 请求
    if (httpRequest) {
        processHttpRequest(httpRequest);
    }
    
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

    // 检查请求体是否完整
    int bodyStart = headerEnd + 4;
    if (data.size() >= bodyStart + contentLength) {
        HttpRequest* result = new HttpRequest();
        result->body = data.mid(bodyStart, contentLength);
        result->contentLength = contentLength;
        result->headerLines = headerLines;
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
        sendResponse(400, "Bad Request", &content, "text/html");
        return;
    }

    QString method = firstLineParts[0];
    QString path = firstLineParts[1];

    emit logMessage(QString("已处理来自%1的请求，方法：%2，路径：%3").arg(clientInfo).arg(method).arg(path));
    
    QByteArray content;
    content.append("<html><head><meta charset=\"UTF-8\"></head><body><h1>It Works!</h1></body></html>");
    sendResponse(200, "OK", &content, "text/html");
    return;
}

bool ServerTask::sendResponse(int code, QString description, QByteArray* content, QString contentType) {
    // 状态行
    QString statusLine = QString("HTTP/1.0 %1 %2\r\n").arg(code).arg(description);
    // 响应头
    QString header = QString("Server: MyCustomServer\r\n");
    if (content != nullptr && content->size() > 0) {
        header = header.append("Content-Length: %1\r\nContent-Type: %2\r\n").arg(content->size()).arg(contentType);
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