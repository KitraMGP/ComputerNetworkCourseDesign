# 实验二：网络扫描程序

本项目依赖 Qt6，实现了利用 ICMP 回显请求报文（即 Ping）扫描给定 IP 地址范围的主机。

## 运行和调试

该程序由于使用了 raw socket，所以需要按照以下步骤授予权限才能正常运行。

执行 `xmake` 进行构建，还需要执行`sudo setcap cap_net_raw+ep build/linux/x86_64/debug/EXP2_Tracert`命令来为可执行文件添加创建rawsocket的权限。

也可以执行项目中的自定义 VSCode Task 来进行构建，它会自动执行上述命令。

调试的时候，请执行 `launch.json` 中定义的调试方法，先手动启动程序然后再**附加**调试器。
