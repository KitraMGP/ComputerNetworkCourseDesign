#include <httpserverworker.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <QTimer>

HttpServerWorker::HttpServerWorker() {

}

bool HttpServerWorker::startServer(string rootPath, int port) {
    // 创建 socket
    this->serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        emit logMessage("socket 创建失败：" + QString(strerror(errno)));
        return false;
    }
    // 设置 SO_REUSEADDR 选项允许地址重用
    int opt = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        emit logMessage("setsockopt 失败：" + QString(strerror(errno)));
        close(serverSock);
        serverSock = -1;
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
        return false;
    }
    // 开始监听
    if (listen(serverSock, 30) < 0) {
        emit logMessage("listen 失败：" + QString(strerror(errno)));
        close(serverSock);
        serverSock = -1;
        return false;
    }
    emit logMessage("开始监听传入连接");
    emit started();

    // 让当前方法正常执行结束，在下一次界面消息循环的时候启动循环
    // 启动的循环在 Worker 线程执行，不影响UI线程
    QTimer::singleShot(0, this, &HttpServerWorker::processServerLoop);
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
    emit logMessage("服务器已停止");
    emit stopped();
}

void HttpServerWorker::processServerLoop() {
    sockaddr_in clientAddr;
    
    // 服务器主循环
    while (isRunning) {
        // 设置超时时间 1s
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
    }
}

HttpServerWorker::~HttpServerWorker() {
    
}