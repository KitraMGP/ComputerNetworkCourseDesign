#include <HttpServerWorker.h>
#include <ServerTask.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <QTimer>
#include <QDebug>
#include <QCoreApplication>
#include <arpa/inet.h>

HttpServerWorker::HttpServerWorker(QObject* parent) : QObject(parent) {
    // 创建线程池
    this->threadPool = new QThreadPool(this);
    // 最大 20 个线程
    this->threadPool->setMaxThreadCount(20);
    // 空闲时间超过 30 秒就退出
    this->threadPool->setExpiryTimeout(30000);

    qDebug() << "线程池已创建";
}

bool HttpServerWorker::startServer(QString rootPath, int port) {
    this->rootDir = rootPath;
    // 创建 socket
    this->serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        emit logMessage("socket 创建失败：" + QString(strerror(errno)));
        stopServer();
        return false;
    }
    // 设置 SO_REUSEADDR 选项允许地址重用
    int opt = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        emit logMessage("setsockopt 失败：" + QString(strerror(errno)));
        close(serverSock);
        serverSock = -1;
        stopServer();
        return false;
    }
    // 初始化 serverAddr
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // bind
    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        emit logMessage("bind 失败：" + QString(strerror(errno)));
        close(serverSock);
        serverSock = -1;
        stopServer();
        return false;
    }
    // 开始监听
    if (listen(serverSock, 30) < 0) {
        emit logMessage("listen 失败：" + QString(strerror(errno)));
        close(serverSock);
        serverSock = -1;
        stopServer();
        return false;
    }

    isRunning = true;

    emit logMessage("开始监听传入连接");
    emit started();

    processServerLoop();
    return true;
}

void HttpServerWorker::stopServer() {
    if (!isRunning) {
        return;
    }
    if (serverSock != -1) {
        close(serverSock);
        serverSock = -1;
    }
    // 关闭所有socket
    threadPool->clear();
    QMutexLocker locker(&activeConnectionsMutex);
    for (int sock : activeConnections) {
        close(sock);
    }
    activeConnections.clear();
    qDebug() << "已关闭所有socket";

    // 等待所有任务退出
    if (threadPool) {
        threadPool->waitForDone(1000);
    }
    isRunning = false;
    emit logMessage("服务器已停止");
    emit stopped();
}

void HttpServerWorker::processServerLoop() {
    sockaddr_in clientAddr;
    unsigned int clientAddrLen = sizeof(clientAddr);
    
    // 服务器主循环
    while (isRunning) {
        // 设置超时时间 1s
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // 进行 select
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSock, &readfds);

        int selectResult = select(serverSock + 1, &readfds, NULL, NULL, &timeout);

        // 确保请求服务器停止的事件正确执行
        QCoreApplication::processEvents();

        if (!isRunning) {
            break;
        }

        // select 失败的处理
        if (selectResult < 0) {
            if (errno == EINTR) {
                continue;   // 被系统信号中断，正常情况
            } else {
                emit logMessage("select 错误：" + QString(strerror(errno)));
                break;

            }
        }

        // 超时，进行下一次循环
        if (selectResult == 0) {
            continue;
        }

        // 检查 serverSocks 是否有可读事件
        if (FD_ISSET(serverSock, &readfds)) {
            // 接受连接
            int clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSock < 0) {
                emit logMessage("accept 失败：" + QString(strerror(errno)));
                continue;
            }
            QString clientInfo = QString("%1:%2").arg(inet_ntoa(clientAddr.sin_addr)).arg(ntohs(clientAddr.sin_port));
            qDebug() << "客户端连接：" << clientInfo;

            // 开始创建线程处理请求

            // 达到最大连接数拒绝连接
            if (getActiveConnectionCount() > 100) {
                qDebug() << "达到最大连接数量，拒绝连接：" << clientInfo;
                QString errorResponse =
                    "HTTP/1.1 503 Service Unavailable\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: 133\r\n"
                    "\r\n"
                    "<html><head><meta charset=\"UTF-8\"></head><body><h1>503 Service Unavailable</h1>"
                    "<p>服务器繁忙，请稍后重试</p></body></html>";
                send(clientSock, errorResponse.toUtf8().constData(), errorResponse.toUtf8().length(), 0);
                close(clientSock);
                continue;
            }

            // 创建任务处理客户端请求
            ServerTask *task = new ServerTask(clientSock, rootDir, clientInfo);
            // 连接信号和槽
            connect(task, &ServerTask::taskFinished, this, &HttpServerWorker::serverTaskFinished);
            connect(task, &ServerTask::logMessage, this, &HttpServerWorker::serverTaskLogMessage);
            addActiveConnection(clientSock);
            // 提交到线程池
            threadPool->start(task);
            
            qDebug() << "任务已提交到线程池：" << clientInfo;
        }
    }

    // 等待所有任务完成
    threadPool->waitForDone(1000);
    qDebug() << "退出服务器主循环！";
    stopServer();
}

void HttpServerWorker::serverTaskLogMessage(QString message) {
    emit logMessage(message);
}

void HttpServerWorker::serverTaskFinished(int socket) {
    close(socket);
    removeActiveConnection(socket);
}

int HttpServerWorker::getActiveConnectionCount() {
    QMutexLocker locker(&activeConnectionsMutex);
    return activeConnections.size();
}

void HttpServerWorker::addActiveConnection(int socket) {
    QMutexLocker locker(&activeConnectionsMutex);
    activeConnections.insert(socket);
}

void HttpServerWorker::removeActiveConnection(int socket) {
    QMutexLocker locker(&activeConnectionsMutex);
    activeConnections.remove(socket);
    qDebug() << "连接关闭，剩余活跃连接数：" << activeConnections.size();
}

bool HttpServerWorker::isServerRunning() {
    return this->isRunning;
}

HttpServerWorker::~HttpServerWorker() {
    stopServer();
}