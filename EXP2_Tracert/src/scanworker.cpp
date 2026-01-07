#include "scanworker.h"
#include "core/networkscan.h"
#include <QThread>

using std::vector, std::string, std::floor;

ScanWorker::ScanWorker(QObject *parent, const vector<string> ipList, int timeout) : QObject(parent) {
    this->ipList = ipList;
    this->timeout = timeout;
}

void ScanWorker::startScan() {
    for(int i = 0; i < ipList.size(); i++) {
        if (this->requestedStop)
            break;
        //QThread::msleep(20);
        emit scanProgress(floor((float)i / ipList.size() * 100));
        if (ping(ipList[i], timeout)) {
            emit addIP(ipList[i]);
        }
    }
    emit scanFinished();
}

void ScanWorker::stopWorker() {
    this->requestedStop = true;
}
