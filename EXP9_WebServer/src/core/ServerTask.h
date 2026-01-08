#ifndef SERVER_TASK_H
#define SERVER_TASK_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QRunnable>

/**
 * HTTP 请求解析结构体
 */
typedef struct {
    int contentLength;
    QList<QByteArray> headerLines;
    QByteArray body;
} HttpRequest;

class ServerTask : public QObject, public QRunnable {
    Q_OBJECT

    signals:
        void logMessage(QString data);
        void taskFinished(int clientSocket);

    public:
        int clientSock;
        QString clientInfo;
        explicit ServerTask(int clientSock, QString clientInfo, QObject *parent = nullptr);
        void run() override;

    private:
        /**
         * 解析 HTTP 请求，返回解析结果，nullptr 代表请求不完整
         */
        HttpRequest* parseRequest(const QByteArray& data);
        /**
         * 处理 HTTP 请求
         */
        void processHttpRequest(HttpRequest* request);
        /**
         * 发送响应，返回是否成功。content 可为 nullptr。
         */
        bool sendResponse(int code, QString description, QByteArray* content, QString contentType);
};

#endif