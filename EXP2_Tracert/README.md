# 实验二

本项目依赖 Qt6。

## 运行和调试

执行 `xmake` 进行构建，还需要执行`sudo setcap cap_net_raw+ep build/linux/x86_64/debug/EXP2_Tracert`命令来为其添加创建rawsocket的权限。

也可以执行项目中的自定义 VSCode Task 来进行构建。

调试的时候，请执行 `launch.json` 中定义的调试方法，先手动启动程序然后再附加调试器。
