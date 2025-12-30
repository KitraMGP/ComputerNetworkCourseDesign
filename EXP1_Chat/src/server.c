#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>
#include "config.h"
#include "packet.h"

static __thread char send_buf[MAX_BUFF_SIZE] = {0};
static __thread char recv_buf[MAX_BUFF_SIZE] = {0};

struct list_node
{
    int sock;
    char name[MAX_NAME_LEN]; // 用户名
    struct list_node *next;
};

struct session_list_struct
{
    struct list_node *first;
    pthread_rwlock_t rwlock; // 用于保证线程安全的读写锁
} session_list;

// 创建新节点，并将所有参数设为空
struct list_node *session_list_create_node()
{
    struct list_node *new_node = malloc(sizeof(struct list_node));
    new_node->sock = -1;
    new_node->next = NULL;
    memset(new_node->name, 0, sizeof(new_node->name));
    return new_node;
}

// 初始化 session_list
void session_list_init()
{
    session_list.first = NULL;
    pthread_rwlock_init(&session_list.rwlock, NULL);
}

// 向 session_list 插入值
void session_list_insert(int sock)
{
    pthread_rwlock_wrlock(&session_list.rwlock);
    if (session_list.first == NULL)
    {
        session_list.first = session_list_create_node();
        session_list.first->sock = sock;
        session_list.first->next = NULL;
    }
    else
    {
        struct list_node *pos = session_list.first;
        while (pos->next != NULL)
        {
            pos = pos->next;
        }
        pos->next = session_list_create_node();
        pos->next->sock = sock;
        pos->next->next = NULL;
    }
    pthread_rwlock_unlock(&session_list.rwlock);
}

// 从 session_list 中删除值
// 若删除成功，返回 true。若没找到，返回 false。
bool session_list_remove(int sock)
{
    pthread_rwlock_wrlock(&session_list.rwlock);
    struct list_node *pos = session_list.first;
    if (pos == NULL)
    {
        pthread_rwlock_unlock(&session_list.rwlock);
        return false;
    }
    // 第一个节点匹配的情况
    if (pos->sock == sock)
    {
        session_list.first = pos->next;
        free(pos);
        pthread_rwlock_unlock(&session_list.rwlock);
        return true;
    }
    // 不断往后扫描
    while (pos->next != NULL)
    {
        if (pos->next->sock == sock)
        {
            struct list_node *to_delete = pos->next;
            pos->next = pos->next->next;
            free(to_delete);
            pthread_rwlock_unlock(&session_list.rwlock);
            return true;
        }
        pos = pos->next;
    }
    pthread_rwlock_unlock(&session_list.rwlock);
    return false;
}

// 为一个会话设置用户名
bool session_list_set_name(int sock, const char* name)
{
    pthread_rwlock_wrlock(&session_list.rwlock);
    struct list_node *pos = session_list.first;
    while (pos != NULL && pos->sock != sock)
    {
        pos = pos->next;
    }
    if (pos->sock == sock)
    {
        strncpy(pos->name, name, MAX_NAME_LEN);
        pthread_rwlock_unlock(&session_list.rwlock);
        return true;
    }
    else
    {
        pthread_rwlock_unlock(&session_list.rwlock);
        return false;
    }
}

// 获取一个会话的名字，如果会话不存在，返回 NULL
const char* session_list_get_name(int sock)
{
    pthread_rwlock_rdlock(&session_list.rwlock);
    struct list_node *pos = session_list.first;
    while (pos != NULL && pos->sock != sock)
    {
        pos = pos->next;
    }
    if (pos->sock == sock)
    {
        pthread_rwlock_unlock(&session_list.rwlock);
        return pos->name;
    }
    else
    {
        pthread_rwlock_unlock(&session_list.rwlock);
        return "NULL";
    }
}

// 判断字符串是否全为空白字符
bool is_blank(const char* str, size_t buff_max_size)
{
    size_t len = strnlen(str, buff_max_size);
    for (size_t i = 0; i < len; i++)
    {
        if (!isblank(str[i]))
        {
            return false;
        }
    }
    return true;
}

// 向所有客户端 socket 发送消息
void broadcast_message(const char* message)
{
    pthread_rwlock_rdlock(&session_list.rwlock);
    struct list_node* pos = session_list.first;
    while (pos != NULL)
    {
        send_msg(pos->sock, message, strnlen(message, MAX_BUFF_SIZE));
        pos = pos->next;
    }
    pthread_rwlock_unlock(&session_list.rwlock);
}

// 为一个会话服务的线程
void *session_thread(void *sock_ptr)
{
    int sock = *(int *)sock_ptr;
    free(sock_ptr);
    // 用户是否已设置名称
    bool set_name = false;
    static char* welcome_msg = "\n欢迎来到聊天室！\n";
    static char* name_prompt = "请输入你的名字：";
    static char* name_too_long_msg = "名字过长，请重新输入\n";
    static char* name_is_empty_msg = "名字不能为空\n";
    static char* successful_msg = "设置成功！\n";
    send_msg(sock, welcome_msg, strlen(welcome_msg));
    send_msg(sock, name_prompt, strlen(name_prompt));

    while (1)
    {
        ssize_t recv_len;
        recv_len = recv_msg(sock, recv_buf, MAX_BUFF_SIZE - 1);

        if (recv_len == -1)
        {
            printf("客户端 %d 数据接收失败\n", sock);
        }
        else if (recv_len == 0)
        {
            printf("客户端 %d 关闭连接，连接终止\n", sock);
            // 广播用户退出消息
            if (set_name)
            {
                snprintf(send_buf, MAX_BUFF_SIZE, "用户 %s 退出聊天室\n", session_list_get_name(sock));
            }
            broadcast_message(send_buf);
            break;
        }
        // 若 set_name 为 false 则将收到的字符串设为用户名
        recv_buf[recv_len] = '\0';
        if (!set_name)
        {
            // 名字过长
            if (strnlen(recv_buf, MAX_BUFF_SIZE) > MAX_NAME_LEN - 1)
            {
                send_msg(sock, name_too_long_msg, strlen(name_too_long_msg));
                send_msg(sock, name_prompt, strlen(name_prompt));
                continue;
            }
            // 名字字符串全为空白字符
            if (is_blank(recv_buf, MAX_BUFF_SIZE))
            {
                send_msg(sock, name_is_empty_msg, strlen(name_is_empty_msg));
                send_msg(sock, name_prompt, strlen(name_prompt));
                continue;
            }
            // 设置名字
            session_list_set_name(sock, recv_buf);
            set_name = true;
            printf("客户端 %d 设置了名字 %s\n", sock, recv_buf);
            send_msg(sock, successful_msg, strlen(successful_msg));
            // 广播用户加入聊天室的消息
            snprintf(send_buf, MAX_BUFF_SIZE, "用户 %s 加入聊天室\n", recv_buf);
            broadcast_message(send_buf);
            continue;
        }

        // 服务端打印消息
        if (set_name)
        {
            const char* name = session_list_get_name(sock);
            printf("客户端 %d 昵称 %s 发送消息：%s\n", sock, name, recv_buf);
        }
        else
        {
            printf("客户端 %d 发送消息：%s\n", sock, recv_buf);
        }
        
        // 广播消息 [用户名] 消息内容\n
        char* tmp_buf = malloc(MAX_BUFF_SIZE);
        snprintf(tmp_buf, MAX_BUFF_SIZE, "[%s] %s\n", session_list_get_name(sock), recv_buf);
        broadcast_message(tmp_buf);
        free(tmp_buf);
    }

    // 连接关闭后的操作
    if (!session_list_remove(sock))
    {
        printf("从 session_list 移除元素 %d 失败\n", sock);
    }
}

int server_main()
{
    int server_sock = -1;
    struct sockaddr_in server_addr;

    session_list_init();

    // 1. 创建 socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        printf("socket 创建失败！\n");
        exit(1);
    }
    printf("socket 创建成功\n");

    // 设置 SO_REUSEADDR 选项，允许地址重用
    // 这样服务器终止后可以立即重新启动，不必等待 TIME_WAIT 结束
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        printf("setsockopt 失败！\n");
        exit(1);
    }

    // 2. 初始化 serverAddr 并 bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    // 使用 htons 和 htonl 的作用是转换字节序
    // x86 使用小端序，而网络传输标准使用大端序
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        printf("bind 失败！\n");
        exit(2);
    }
    printf("bind 成功\n");

    // 3. 开始监听
    if (listen(server_sock, 10) == -1)
    {
        printf("listen 失败！\n");
        exit(3);
    }
    printf("开始监听连接请求\n");

    while (1)
    {
        int client_sock = accept(server_sock, NULL, NULL);
        printf("已接受连接 %d\n", client_sock);
        session_list_insert(client_sock);
        // 创建线程并传入 sock 参数
        pthread_t session_thread_handle;
        int *sock_arg = malloc(sizeof(int));
        *sock_arg = client_sock;
        if (pthread_create(&session_thread_handle, NULL, session_thread, sock_arg) == 0)
        {
            pthread_detach(session_thread_handle);
        }
        else
        {
            printf("会话服务线程创建失败\n");
        }
    }

    return 0;
}