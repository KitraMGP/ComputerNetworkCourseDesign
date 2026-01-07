#ifndef HTTP_SERVER_WORKER_H
#define HTTP_SERVER_WORKER_H

#include <QObject>
#include <string>
#include <QString>

using std::string, std::atomic_bool;

class HttpServerWorker : public QObject {
    Q_OBJECT

    public:
        HttpServerWorker();
        ~HttpServerWorker();
        /**
         * 启动服务器
         */
        bool startServer(string rootPath, int port);
        /**
         * 停止服务器
         */
        void stopServer();
        /**
         * 探测服务器是否在运行
         */
        bool isRunning();
    
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
    
    private:
        int serverSock = -1;
        atomic_bool isRunning = false;
};

#endif