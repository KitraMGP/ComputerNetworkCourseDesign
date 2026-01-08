# HttpServer

这是一个基于 socket 编程的 HTTP 服务器实现。

本服务器有图形界面控制面板，具有以下特性：

- 基于 select() 方法监听连接，避免长期阻塞，服务器可以优雅地在控制面板启动或停止
- 实现了 HTTP GET 方法、HEADER 方法，可搭建静态页面服务器
- 支持 HTTP/1.1 长连接
- 响应头中包含资源修改时间 Date 字段
- 支持并发访问，利用线程池机制复用线程，在并发连接数达到指定上限时拒绝连接
- 实现了一个简单接口 /testPostApi，用来演示 POST 方法的使用，其接受类型为 `application/x-www-form-urlencoded` 的表单
