#ifndef SCAN_WORKER_H
#define SCAN_WORKER_H

#include <QObject>
#include <string>
#include <vector>

using std::vector, std::string, std::atomic_bool;

class ScanWorker : public QObject {
    Q_OBJECT

  public:
    explicit ScanWorker(QObject *parent, vector<string> ipList, int timeout);
    void stopWorker();

  signals:
    void scanProgress(const int percent);
    void addIP(const string ip);
    void scanFinished();

  public slots:
    void startScan();
  
  private:
    vector<string> ipList;
    int timeout;
    atomic_bool requestedStop = false;
};

#endif // SCAN_WORKER_H