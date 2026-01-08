#ifndef HTTP_SERVER_WORKER_H
#define HTTP_SERVER_WORKER_H

#include <QObject>
#include <QString>
#include <QThreadPool>
#include <QSet>
#include <QMutex>

using std::atomic_bool;

class HttpServerWorker : public QObject {
    Q_OBJECT

    public:
        explicit HttpServerWorker(QObject* parent = nullptr);
        ~HttpServerWorker();
        /**
         * 探测服务器是否在运行
         */
        bool isServerRunning();
    
    public slots:
        /**
         * 启动服务器
         */
        bool startServer(QString rootPath, int port);
        /**
         * 停止服务器
         */
        void stopServer();

    signals:
        /**
         * 打印日志信号
         */
        void logMessage(QString message);
        /**
         * 服务器启动信号
         */
        void started();
        /**
         * 服务器停止信号
         */
        void stopped();

    private slots:
        /**
         * 服务器内部接受请求的循环
         */
        void processServerLoop();
        /**
         * ServerTask 完成操作，移除 socket
         */
        void serverTaskFinished(int clientSocket);
        /**
         * ServerTask 打印日志
         */
        void serverTaskLogMessage(QString message);
    
    private:
        int serverSock = -1;
        QString rootDir = "";
        atomic_bool isRunning = false;
        // 供 ServerTask 使用的线程池
        QThreadPool* threadPool = nullptr;
        // 活跃连接集合
        QSet<int> activeConnections;
        // 确保 activeConnections 线程安全
        QMutex activeConnectionsMutex;

        /**
         * 获取活跃连接数
         */
        int getActiveConnectionCount();
        /**
         * 增加活跃连接
         */
        void addActiveConnection(int socket);
        /**
         * 移除活跃连接
         */
        void removeActiveConnection(int socket);
};

#endif