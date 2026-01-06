#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <vector>
#include "scanworker.h"

using std::vector, std::string;

namespace Ui {
class MainWindow;
}

// 前置声明
class QThread;

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
  
  private slots:
    void onScanButtonClicked();
    void onStopScanButtonClicked();
    void updateProgress(const int percent);
    void receiveNewIP(const string ip);
    void scanFinished();

  private:
    Ui::MainWindow *ui;
    string ipInput1;
    string ipInput2;
    string ipInput3;
    string ipInput4;
    string ipInput5;
    int timeout;
    ScanWorker *worker;
    QThread *workerThread;
    /**
    * 验证用户输入的IP地址范围和超时时间是否合法，并设置基本参数
    */
    bool validateInputs();
    /**
     * 生成要扫描的IP地址列表
     */
    vector<string> getScanIPs();
    /**
     * 清空扫描结果表格
     */
    void clearScanResults();
    /**
     * 启动ScanWorker，建立信号槽连接并初始化进度
     */
    void startWorker(const vector<string> ipList);
};

#endif // MAINWINDOW_H
