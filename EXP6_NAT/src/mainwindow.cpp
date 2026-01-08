#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>
#include <QMessageBox>

const QRegularExpression numberRegex("^[0-9]+$");

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // 禁止编辑表格
    ui->natTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 信号和槽连接
    connect(ui->lanSendBtn, &QPushButton::clicked, this, &MainWindow::onLanSendBtnClicked);
    connect(ui->wanSendBtn, &QPushButton::clicked, this, &MainWindow::onWanSendBtnClicked);
    connect(ui->clearLogsBtn, &QPushButton::clicked, this, &MainWindow::onClearLogsBtnClicked);
    connect(ui->setNatWanAddrBtn, &QPushButton::clicked, this, &MainWindow::onSetNatWanAddrBtnClicked);
}

void MainWindow::refreshTable() {
    ui->natTableWidget->setRowCount(0);
    // 遍历两个 map 填充表格
    for (const auto& key : lan2WanMap.keys()) {
        int rowCount = ui->natTableWidget->rowCount();
        ui->natTableWidget->setRowCount(rowCount + 1);
        ui->natTableWidget->setItem(rowCount, 0, new QTableWidgetItem("从专用网发往互联网"));
        ui->natTableWidget->setItem(rowCount, 1, new QTableWidgetItem("源IP地址:TCP源端口"));
        ui->natTableWidget->setItem(rowCount, 2, new QTableWidgetItem(key));
        ui->natTableWidget->setItem(rowCount, 3, new QTableWidgetItem(lan2WanMap.value(key)));
    }
    for (const auto& key : wan2LanMap.keys()) {
        int rowCount = ui->natTableWidget->rowCount();
        ui->natTableWidget->setRowCount(rowCount + 1);
        ui->natTableWidget->setItem(rowCount, 0, new QTableWidgetItem("从互联网发往专用网"));
        ui->natTableWidget->setItem(rowCount, 1, new QTableWidgetItem("目的IP地址:TCP目的端口"));
        ui->natTableWidget->setItem(rowCount, 2, new QTableWidgetItem(key));
        ui->natTableWidget->setItem(rowCount, 3, new QTableWidgetItem(wan2LanMap.value(key)));
    }
}

void MainWindow::onLanSendBtnClicked() {
    if (!isHostPortValid(ui->lanSrcLineEdit->text()) || !isHostPortValid(ui->lanDestLineEdit->text())) {
        QMessageBox::information(this, "错误", "源地址或目的地址不合法", QMessageBox::StandardButton::Ok);
        return;
    }
    // 查找地址转换表，若没有条目则创建
    QString src = ui->lanSrcLineEdit->text();
    QString dst = ui->lanDestLineEdit->text();
    bool found = false;
    if (lan2WanMap.contains(src)) {
        found = true;
    }
    // 未匹配，说明需要创建记录
    if (!found) {
        // 获取一个空闲端口
        int newPort = lastPort++;
        // 生成转换后的地址
        QString wanAddr = QString("%1:%2").arg(natWanAddr).arg(newPort);
        // 插入map
        lan2WanMap.insert(src, wanAddr);
        wan2LanMap.insert(wanAddr, src);
        printLog(QString("创建新记录 内网：%1，外网：%2").arg(src).arg(wanAddr));
        refreshTable();
    }

    printLog(QString("内网->外网发送数据成功: %1->%2 经由路由器公网地址%3").arg(src).arg(dst).arg(lan2WanMap.value(src)));
}

void MainWindow::onWanSendBtnClicked() {
    if (!isHostPortValid(ui->wanSrcLineEdit->text()) || !isHostPortValid(ui->wanDestLineEdit->text())) {
        QMessageBox::information(this, "错误", "源地址或目的地址不合法", QMessageBox::StandardButton::Ok);
        return;
    }
    // 查找地址转换表
    QString src = ui->wanSrcLineEdit->text();
    QString dst = ui->wanDestLineEdit->text();
    bool found = wan2LanMap.contains(dst);

    // 未匹配，拒绝操作
    if (!found) {
        QMessageBox::information(this, "错误", "找不到匹配的NAT记录", QMessageBox::StandardButton::Ok);
        printLog(QString("外网->内网发送数据失败: %1->%2").arg(src).arg(dst));
        return;
    }

    printLog(QString("外网->内网发送数据成功: %1->%2 发送到内网主机%3").arg(src).arg(dst).arg(wan2LanMap.value(dst)));
}

void MainWindow::onSetNatWanAddrBtnClicked() {
    bool ok;
    QString text = QInputDialog::getText(this, "请输入NAT路由器公网IP地址", "IP地址", QLineEdit::Normal, "", &ok);
    if (!ok) {
        return;
    }
    if (!isIpValid(text)) {
        QMessageBox::information(this, "错误", "IP地址无效", QMessageBox::StandardButton::Ok);
        return;
    }
    natWanAddr = text;
    ui->natWanAddrLabel->setText(text);
    lan2WanMap.clear();
    wan2LanMap.clear();
    refreshTable();
    lastPort = 20000;
    printLog("设置路由器公网IP为" + text);
}

bool MainWindow::isIpValid(QString ip) {
    if (ip.isEmpty()) {
        return false;
    }
    QList<QString> list = ip.split('.');
    if (list.size() != 4) {
        return false;
    }
    for (QString part : list) {
        if (!numberRegex.match(part).hasMatch()) {
            return false;
        }
    }
    return true;
}

bool MainWindow::isHostPortValid(QString text) {
    QList<QString> parts = text.split(':');
    if (parts.size() != 2) {
        return false;
    }
    if (!isIpValid(parts[0])) {
        return false;
    }
    if (!numberRegex.match(parts[1]).hasMatch()) {
        return false;
    }
    try {
        int ignored = parts[1].toInt();
    } catch (std::exception) {
        return false;
    }
    return true;
}

int MainWindow::getPort(QString hostPort) {
    return hostPort.split(':')[1].toInt();
}

void MainWindow::printLog(QString text) {
    ui->logsArea->appendPlainText(text);
}

void MainWindow::onClearLogsBtnClicked() {
    ui->logsArea->clear();
}

MainWindow::~MainWindow() {
    delete ui;
}
