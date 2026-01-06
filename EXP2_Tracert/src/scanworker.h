#ifndef SCAN_WORKER_H
#define SCAN_WORKER_H

#include <QObject>
#include <string>
#include <vector>

using std::vector, std::string;

class ScanWorker : public QObject {
    Q_OBJECT

  public:
    explicit ScanWorker(QObject *parent, vector<string> ipList);

  signals:
    void scanProgress(const int percent);
    void addIP(const string ip);
    void scanFinished();

  public slots:
    void startScan();
  
  private:
    vector<string> ipList;

};

#endif // SCAN_WORKER_H