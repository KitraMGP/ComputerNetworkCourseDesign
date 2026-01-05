#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <string>

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
}

void MainWindow::onScanButtonClicked() {
    validateInputs();
}

bool MainWindow::validateInputs() {
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
    } catch (std::invalid_argument) {
        QMessageBox::information(this, "提示", "输入的IP地址范围不合法", QMessageBox::StandardButton::Ok);
        return false;
    }

    return true;
}

MainWindow::~MainWindow() {
    delete ui;
}
