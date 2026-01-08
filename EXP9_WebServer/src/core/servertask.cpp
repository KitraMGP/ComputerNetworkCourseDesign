#include <servertask.h>

ServerTask::ServerTask(int clientSock, QString clientInfo, QObject* parent) : QObject(parent) {
    this->clientSock = clientSock;
    this->clientInfo = clientInfo;
    // 设置自动删除
    setAutoDelete(true);
}

void ServerTask::run() {

}