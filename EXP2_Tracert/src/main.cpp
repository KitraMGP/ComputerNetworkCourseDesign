#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QStyle>
#include <qtablewidget.h>

int main(int argc, char *argv[]) {

    QApplication a(argc, argv);

    // // 打印可用的样式（用于调试）
    // QStringList styles = QStyleFactory::keys();
    // qDebug() << "Available styles:" << styles;
    // qDebug() << "Current style className:" << a.style()->metaObject()->className();

    MainWindow w;
    w.show();
    return a.exec();
}
