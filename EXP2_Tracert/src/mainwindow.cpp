#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <string>
#include <scanworker.h>
#include <QThread>

using std::string;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // 表格不允许编辑
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // IP地址输入框限制
    ui->ipLineEdit_1->setValidator(new QIntValidator(0, 255, this));
    ui->ipLineEdit_2->setValidator(new QIntValidator(0, 255, this));
    ui->ipLineEdit_3->setValidator(new QIntValidator(0, 255, this));
    ui->ipLineEdit_4->setValidator(new QIntValidator(0, 255, this));
    ui->ipLineEdit_5->setValidator(new QIntValidator(0, 255, this));
    // 超时输入框限制
    ui->timeoutLineEdit->setValidator(new QIntValidator(0, 30000, this));

    // 信号和槽连接
    connect(ui->startScanBtn, &QPushButton::clicked, this, &MainWindow::onScanButtonClicked);
    connect(ui->stopScanBtn, &QPushButton::clicked, this, &MainWindow::onStopScanButtonClicked);

    this->worker = nullptr;
    this->workerThread = nullptr;
}

void MainWindow::onScanButtonClicked() {
    // 输入参数不合法，结束操作
    if (!validateInputs()) {
        return;
    }

    // 禁用开始扫描按钮，启用停止扫描按钮
    this->ui->startScanBtn->setEnabled(false);
    this->ui->stopScanBtn->setEnabled(true);

    // 清空扫描结果
    clearScanResults();

    // 生成IP地址范围
    vector<string> scanIPs = getScanIPs();

    // 创建Worker开始扫描
    startWorker(scanIPs);
}

void MainWindow::onStopScanButtonClicked() {
    if (this->worker) {
        this->worker->stopWorker();
    }
}

void MainWindow::updateProgress(const int progress) {
    ui->progressBar->setValue(progress);
}

void MainWindow::receiveNewIP(const string ip) {
    // 为table增加一行并插入数据
    int count = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(count + 1);
    ui->tableWidget->setItem(count, 0, new QTableWidgetItem(QString::fromStdString(ip)));
}

void MainWindow::scanFinished() {
    // 设置进度条
    ui->progressBar->setValue(100);
    // 禁用停止按钮，启用开始按钮
    ui->stopScanBtn->setEnabled(false);
    ui->startScanBtn->setEnabled(true);
}

bool MainWindow::validateInputs() {
    // 检验IP地址范围是否合法
    string ip1, ip2, ip3, ip4, ip5;
    ip1 = this->ui->ipLineEdit_1->text().toStdString();
    ip2 = this->ui->ipLineEdit_2->text().toStdString();
    ip3 = this->ui->ipLineEdit_3->text().toStdString();
    ip4 = this->ui->ipLineEdit_4->text().toStdString();
    ip5 = this->ui->ipLineEdit_5->text().toStdString();
    try {
        int ip1num = std::stoi(ip1);
        int ip2num = std::stoi(ip2);
        int ip3num = std::stoi(ip3);
        int ip4num = std::stoi(ip4);
        int ip5num = std::stoi(ip5);
        if (ip1num > 255 || ip2num > 255 || ip3num > 255 || ip4num > 255 || ip5num > 255 || ip4num > ip5num) {
            QMessageBox::information(this, "提示", "输入的IP地址范围不合法", QMessageBox::StandardButton::Ok);
            return false;
        }
    } catch (std::exception) {
        QMessageBox::information(this, "提示", "输入的IP地址范围不合法", QMessageBox::StandardButton::Ok);
        return false;
    }
    // 检验timeout是否合法
    int timeout;
    try {
        timeout = std::stoi(this->ui->timeoutLineEdit->text().toStdString());
    } catch (std::exception) {
        QMessageBox::information(this, "提示", "输入的超时时间不合法", QMessageBox::StandardButton::Ok);
        return false;
    }
    // 设置输入的参数
    this->ipInput1 = ip1;
    this->ipInput2 = ip2;
    this->ipInput3 = ip3;
    this->ipInput4 = ip4;
    this->ipInput5 = ip5;
    this->timeout = timeout;
    return true;
}

vector<string> MainWindow::getScanIPs() {
    int ip4num = std::stoi(this->ipInput4);
    int ip5num = std::stoi(this->ipInput5);
    vector<string> ipList;
    for (int i = ip4num; i <= ip5num; i++) {
        string ipString = ipInput1 + '.' + ipInput2 + '.' + ipInput3 + '.' + std::to_string(i);
        ipList.push_back(ipString);
    }
    return ipList;
}

void MainWindow::clearScanResults() {
    ui->tableWidget->clearContents();
}

void MainWindow::startWorker(const vector<string> ipList) {
    // 重置进度条
    ui->progressBar->setValue(0);
    // 创建worker并移动到worker线程
    this->worker = new ScanWorker(nullptr, ipList);
    this->workerThread = new QThread(this);
    this->worker->moveToThread(this->workerThread);
    // 设置信号槽连接
    connect(worker, &ScanWorker::scanProgress, this, &MainWindow::updateProgress);
    connect(worker, &ScanWorker::addIP, this, &MainWindow::receiveNewIP);
    // 工作完成后自动结束线程
    connect(worker, &ScanWorker::scanFinished, workerThread, &QThread::quit);
    // 线程结束后删除worker对象
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    // 线程结束后清理MainWindow中的资源
    connect(workerThread, &QThread::finished, this, [this]() {
        // 清理资源
        this->worker = nullptr;
        this->workerThread = nullptr;
        this->scanFinished();
    });
    // 启动worker
    this->workerThread->start();
    QMetaObject::invokeMethod(this->worker, &ScanWorker::startScan, Qt::QueuedConnection);
}

MainWindow::~MainWindow() {
    delete ui;
}
