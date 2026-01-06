#include "scanworker.h"
#include <QThread>

using std::vector, std::string;

ScanWorker::ScanWorker(QObject *parent) {

}

void ScanWorker::startScan(const vector<string> ipList) {
    for(int i = 0; i <= 100; i++) {
        QThread::sleep(30);
        emit scanProgress(i);
    }
    emit scanFinished();
}
