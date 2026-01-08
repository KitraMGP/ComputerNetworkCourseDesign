#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMap>
#include <QSet>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
  
  private slots:
    void onLanSendBtnClicked();
    void onWanSendBtnClicked();
    void onClearLogsBtnClicked();
    void onSetNatWanAddrBtnClicked();

  private:
    Ui::MainWindow *ui;
    QString natWanAddr = "172.38.1.5";
    QSet<int> portSet;
    int lastPort = 20000;
    QMap<QString, QString> lan2WanMap;
    QMap<QString, QString> wan2LanMap;
    /**
     * 向日志区域打印日志
     */
    void printLog(QString text);
    /**
     * 检测IP地址是否有效
     */
    static bool isIpValid(QString ip);
    /**
     * 更新地址转换表
     */
    void refreshTable();
    /**
     * 检查IP:端口字符串是否合法
     */
    static bool isHostPortValid(QString hostPort);
    /**
     * 从IP:端口字符串获取端口
     */
    static int getPort(QString hostPort);
};

#endif // MAINWINDOW_H
