#ifndef SERVER_TASK_H
#define SERVER_TASK_H

#include <QObject>
#include <QString>
#include <QRunnable>

class ServerTask : public QObject, public QRunnable {
    Q_OBJECT

    signals:
        void taskFinished(int clientSocket);

    public:
        int clientSock;
        QString clientInfo;
        explicit ServerTask(int clientSock, QString clientInfo, QObject *parent = nullptr);
        void run() override;
};

#endif