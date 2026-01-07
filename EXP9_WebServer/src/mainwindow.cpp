#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QIntValidator>
#include <QMessageBox>
#include <filesystem>
#include <fstream>

#include "httpserverworker.h"

using std::filesystem::exists, std::filesystem::is_directory, std::stoi;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // 设置输入校验
    ui->setverPortLineEdit->setValidator(new QIntValidator(1, 65535, this));

    // 连接信号和槽
    connect(ui->webRootPathBrowseBtn, &QPushButton::clicked, this, &MainWindow::browseServerRootDir);
    connect(ui->startServerBtn, &QPushButton::clicked, this, &MainWindow::onStartServerBtnClicked);
    connect(ui->stopServerBtn, &QPushButton::clicked, this, &MainWindow::onStopServerBtnClicked);
}

void MainWindow::browseServerRootDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "请选择Web根目录路径");
    if (!dir.isEmpty()) {
        ui->webRootPathLineEdit->setText(dir);
    }
}

void MainWindow::onStartServerBtnClicked() {
    if (isServerRunning) {
        return;
    }
    disableControls();
    if (!validateInput()) {
        enableControls();
        return;
    }

    // 启动服务器

    isServerRunning = true;
}

void MainWindow::onStopServerBtnClicked() {
    if (!isServerRunning) {
        return;
    }

    // 关闭服务器

    isServerRunning = false;
    enableControls();
}

void MainWindow::disableControls() {
    ui->webRootPathLineEdit->setEnabled(false);
    ui->webRootPathBrowseBtn->setEnabled(false);
    ui->setverPortLineEdit->setEnabled(false);
    ui->startServerBtn->setEnabled(false);
    ui->stopServerBtn->setEnabled(true);
    ui->serverStatusLabel->setText("服务器运行中");
}

void MainWindow::enableControls() {
    ui->webRootPathLineEdit->setEnabled(true);
    ui->webRootPathBrowseBtn->setEnabled(true);
    ui->setverPortLineEdit->setEnabled(true);
    ui->startServerBtn->setEnabled(true);
    ui->stopServerBtn->setEnabled(false);
    ui->serverStatusLabel->setText("服务器已停止");
}

bool MainWindow::validateInput() {
    if (!exists(ui->webRootPathLineEdit->text().toStdString())) {
        QMessageBox::information(this, "无法启动服务器", "Web根目录路径不存在，请重新选择。", QMessageBox::StandardButton::Ok);
        return false;
    }
    if (!is_directory(ui->webRootPathLineEdit->text().toStdString())) {
        QMessageBox::information(this, "无法启动服务器", "选择的Web根目录路径不是目录，请重新选择。", QMessageBox::StandardButton::Ok);
        return false;
    }
    try {
        int port = stoi(ui->setverPortLineEdit->text().toStdString());
        if (port < 1 || port > 65535) {
            QMessageBox::information(this, "无法启动服务器", "端口无效，请重新输入。", QMessageBox::StandardButton::Ok);
            return false;
        }
    } catch (std::exception) {
        QMessageBox::information(this, "无法启动服务器", "端口无效，请重新输入。", QMessageBox::StandardButton::Ok);
        return false;
    }
    return true;
}

MainWindow::~MainWindow() {
    delete ui;
}
