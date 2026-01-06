#ifndef SCAN_WORKER_H
#define SCAN_WORKER_H

#include <QObject>

using std::vector, std::string;

class ScanWorker : public QObject {
    Q_OBJECT

  public:
    explicit ScanWorker(QObject *parent = nullptr);

  signals:
    void scanProgress(const int percent);
    void addIP(const string ip);
    void scanFinished();

  public slots:
    void startScan(const vector<string> ipList);

};

#endif // SCAN_WORKER_H