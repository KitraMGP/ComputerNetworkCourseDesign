#include "networkscan.h"
#include <arpa/inet.h>
#include <stdint.h>

// ICMP类型字段
const char ICMP_ECHO_REQUEST = 8;   // 请求回显
const char ICMP_ECHO_REPLY = 0;     // 回显应答
const char ICMP_TIMEOUT = 11;       // 传输超时

// 其他常量定义
const int DEF_ICMP_DATA_SIZE = 32;  // ICMP报文默认数据字段长度
const int MAX_ICMP_PACKET_SIZE = 1024;  // ICMP报文最大长度（包括报头）

// IP报头
typedef struct
{
    uint8_t hdr_len:4;      // 4位服务类型
    uint8_t version:4;      // 4位版本号
    uint8_t tos;            // 8位服务类型
    uint16_t total_len;     // 16位总长度
    uint16_t identifier;    // 16位标识符
    uint16_t frag_and_flags;// 3位标志加13位片偏移
    uint8_t ttl;            // 8位生存时间
    uint8_t protocol;       // 8位上层协议号
    uint16_t checksum;      // 16位校验和
    uint32_t sourceIP;      // 32位源IP地址
    uint32_t destIP;        // 32位目的IP地址
} IP_HEADER;

// ICMP报头
typedef struct
{
    int8_t type;    // 8位类型字段
    int8_t code;    // 8位代码字段
    uint16_t cksum; // 16位校验和
    uint16_t id;    // 16位标识符
    uint16_t seq;   // 16位序列号
};

// 计算网际校验和函数
uint16_t checksum(uint16_t *pBuf, int iSize) {
    uint32_t cksum = 0;
    while (iSize > 1) {
        cksum += *pBuf++;
        iSize -= sizeof(uint16_t);
    }
    if (iSize) {
        cksum += *(uint8_t*)pBuf;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (uint16_t)(~cksum);
}

bool ping(string ip, int timeout) {
    // 初始化地址
    unsigned long ulDestIp = inet_addr(ip.c_str());
    sockaddr_in destSockAddr;
    destSockAddr.sin_family = AF_INET;
    destSockAddr.sin_addr.s_addr = ulDestIp;
    int rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    // 设置超时时间
    setsockopt(rawSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(rawSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    // 定义发送接收缓冲区
    //char icmpSendBuf[]
}