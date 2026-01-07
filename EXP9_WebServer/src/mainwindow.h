#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

  public slots:
    void browseServerRootDir();
    void onStartServerBtnClicked();
    void onStopServerBtnClicked();

  private:
    Ui::MainWindow *ui;
    bool isServerRunning = false;
    /**
     * 校验用户输入并弹窗报告错误
     */
    bool validateInput();
    /**
     * 启动服务器时禁用相关按钮和文本框等
     */
    void disableControls();
    /**
     * 关闭服务器时启用相关按钮和文本框等
     */
    void enableControls();
};

#endif // MAINWINDOW_H
