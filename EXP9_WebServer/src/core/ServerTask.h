#ifndef SERVER_TASK_H
#define SERVER_TASK_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QRunnable>
#include <QMimeDatabase>

/**
 * HTTP 请求解析结构体
 */
typedef struct {
    int contentLength;
    QList<QByteArray> headerLines;
    QByteArray body;
    bool keepAlive;
} HttpRequest;

class ServerTask : public QObject, public QRunnable {
    Q_OBJECT

    signals:
        void logMessage(QString data);
        void taskFinished(int clientSocket);

    public:
        explicit ServerTask(int clientSock, QString serverRoot, QString clientInfo, QObject *parent = nullptr);
        void run() override;

    private:
        int clientSock;
        QString serverRoot;
        QString clientInfo;
        const QMimeDatabase mimeDatabase;
        bool keepAlive = false;
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
         * 若 content 为 nullptr，可指定 length 参数（用于 HEAD 方法）
         * 若 content 非 nullptr，忽略 length 参数
         */
        bool sendResponse(int code, QString description, QByteArray* content, QString contentType, QString date, int length = 0);
        /**
         * 从 HTTP 请求路径获取系统文件绝对路径。若返回403代表路径非法，返回404代表找不到文件
         */
        QString getFilesystemPath(QString path);
        /**
         * 从文件路径获取 MIME 类型
         */
        QString getMimeType(QString& filePath);
        /**
         * 获取文件修改日期
         */
        QString getDate(QString& filePath);
};

#endif