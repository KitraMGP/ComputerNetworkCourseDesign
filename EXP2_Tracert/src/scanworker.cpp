#include "scanworker.h"
#include <QThread>

using std::vector, std::string;

ScanWorker::ScanWorker(QObject *parent, const vector<string> ipList) : QObject(parent) {
    this->ipList = ipList;
}

void ScanWorker::startScan() {
    for(int i = 0; i <= 100; i++) {
        QThread::msleep(20);
        emit scanProgress(i);
    }
    emit scanFinished();
}
