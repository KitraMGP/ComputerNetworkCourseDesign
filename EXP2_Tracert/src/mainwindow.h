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
  
  private slots:
    void onScanButtonClicked();

  private:
    Ui::MainWindow *ui;
    
    /**
    * 验证用户输入的IP地址范围和超时时间是否合法，并设置基本参数
    */
    bool validateInputs();
};

#endif // MAINWINDOW_H
