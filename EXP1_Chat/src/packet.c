/**
 * 网络传输注意事项：
 * 收发数据的数据包格式为 [4字节长度] + [消息内容]
 * 发送消息的时候不应该在末尾带上字符串结束符 '\0'
 * 接收到消息后应该在末尾添加字符串结束符，recv_buf[len] = '\0'
 */

#include "packet.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * 发送带长度前缀的消息
 * 返回发送的消息内容部分长度，-1 表示失败
 */
ssize_t send_msg(int sock, const char *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return -1;
    }

    // 1. 发送消息长度（4字节，网络字节序）
    uint32_t net_len = htonl((uint32_t)len);
    ssize_t sent = send(sock, &net_len, sizeof(net_len), 0);
    if (sent != sizeof(net_len)) {
        return -1;
    }

    // 2. 发送消息内容
    sent = send(sock, buf, len, 0);
    if (sent != (ssize_t)len) {
        return -1;
    }

    return len;
}

/**
 * 接收带长度前缀的消息
 * 返回接收到的消息内容部分长度，0 代表连接终止，-1 代表失败
 */
ssize_t recv_msg(int sock, char *buf, size_t max_len)
{
    if (buf == NULL || max_len == 0) {
        return -1;
    }

    // 1. 先接收消息长度（4字节）
    uint32_t net_len = 0;
    ssize_t received = recv(sock, &net_len, sizeof(net_len), 0);

    if (received == 0) {
        // 连接已关闭
        return 0;
    }
    if (received != sizeof(net_len)) {
        // 接收长度头失败
        return -1;
    }

    // 2. 将网络字节序转换为主机字节序
    uint32_t msg_len = ntohl(net_len);

    // 检查消息长度是否合法
    if (msg_len > max_len) {
        // 消息过长，缓冲区不够
        return -1;
    }

    // 3. 接收消息内容（循环接收直到完整）
    size_t total_received = 0;
    char *ptr = (char *)buf;

    while (total_received < msg_len) {
        received = recv(sock, ptr + total_received, msg_len - total_received, 0);

        if (received == 0) {
            // 连接在接收过程中关闭
            return 0;
        }
        if (received < 0) {
            // 接收失败
            return -1;
        }

        total_received += received;
    }

    return (ssize_t)total_received;
}
