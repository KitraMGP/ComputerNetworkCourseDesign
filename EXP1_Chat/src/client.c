#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "config.h"
#include "packet.h"

static char send_buf[MAX_BUFF_SIZE] = {0};
static char recv_buf[MAX_BUFF_SIZE] = {0};

int client_sock = -1;

// 消息接收线程
void* recv_thread(void* arg)
{
    while (1)
    {
        ssize_t recv_len;
        recv_len = recv_msg(client_sock, recv_buf, MAX_BUFF_SIZE - 1);

        if (recv_len == -1)
        {
            printf("数据接收失败\n");
        }
        else if (recv_len == 0)
        {
            printf("服务端关闭连接，连接终止\n");
            exit(0);
        }
        recv_buf[recv_len] = '\0';
        printf("%s", recv_buf);
        fflush(stdout);
    }
}

int client_main()
{
    struct sockaddr_in server_addr;

    // 1. 创建 socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1)
    {
        printf("sock 创建失败\n");
        exit(1);
    }

    // 2. 初始化地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 3. 连接
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("连接失败！\n");
        exit(2);
    }
    printf("连接成功\n要关闭连接，请输入 /quit。\n");

    // 创建消息接收线程
    pthread_t recv_thread_handle;
    if (pthread_create(&recv_thread_handle, NULL, recv_thread, NULL) != 0)
    {
        printf("消息接收线程创建失败\n");
        close(client_sock);
        exit(3);
    }
    pthread_detach(recv_thread_handle);

    // 读取和发送消息
    while(1)
    {
        fgets(send_buf, MAX_BUFF_SIZE, stdin);
        
        size_t len = strnlen(send_buf, MAX_BUFF_SIZE);
        // 忽略空消息
        if (len == 1 && send_buf[0] == '\n')
        {
            continue;
        }
        // 去除字符串尾部换行符
        if (len > 0 && send_buf[len - 1] == '\n')
        {
            send_buf[len - 1] = '\0';
        }
        // 退出
        if (strcmp(send_buf, "/quit") == 0)
        {
            close(client_sock);
            break;
        }
        send_msg(client_sock, send_buf, strnlen(send_buf, MAX_BUFF_SIZE));
    }

    return 0;
}