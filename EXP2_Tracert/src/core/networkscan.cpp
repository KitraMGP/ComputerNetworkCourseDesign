#include "networkscan.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <QDebug>
#include <fstream>
#include <sys/time.h>
#include <netdb.h>

using std::ifstream, std::istringstream;

extern int errno;

// ICMP类型字段
const char ICMP_ECHO_REQUEST = 8;   // 请求回显
const char ICMP_ECHO_REPLY = 0;     // 回显应答
const char ICMP_TIMEOUT = 11;       // 传输超时

// 其他常量定义
const int DEF_ICMP_DATA_SIZE = 32;  // ICMP报文默认数据字段长度
const int MAX_ICMP_PACKET_SIZE = 1024;  // ICMP报文最大长度（包括报头）

// IP报头
typedef struct {
    uint8_t hdr_len : 4;      // 4位服务类型
    uint8_t version : 4;      // 4位版本号
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
typedef struct {
    int8_t type;    // 8位类型字段
    int8_t code;    // 8位代码字段
    uint16_t cksum; // 16位校验和
    uint16_t id;    // 16位标识符
    uint16_t seq;   // 16位序列号
} ICMP_HEADER;

// 报文解码结构
typedef struct {
    uint16_t usSeqNo;       // 序列号
} DECODE_RESULT;

// 计算网际校验和函数
uint16_t checksum(uint16_t* pBuf, int iSize) {
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

bool decodeIcmpResponse(char *pBuf, int packetSize, DECODE_RESULT &decodeResult) {
    // 检查数据报大小的合法性
    IP_HEADER* pIpHeader = (IP_HEADER*)pBuf;
    int ipHeaderLen = pIpHeader->hdr_len * 4;
    if (packetSize < (int)(ipHeaderLen + sizeof(ICMP_HEADER)))
        return false;
    // 根据ICMP报文类型提取ID字段和序列号字段
    ICMP_HEADER *pIcmpHeader = (ICMP_HEADER*)(pBuf + ipHeaderLen);

    qDebug() << "收到 ICMP 包: type=" << pIcmpHeader->type
             << ", code=" << pIcmpHeader->code
             << ", id=" << pIcmpHeader->id
             << ", seq=" << pIcmpHeader->seq
             << ", getpid=" << getpid()
             << ", expectedSeq=" << decodeResult.usSeqNo;

    uint16_t usID, usSeqNo;
    if (pIcmpHeader->type == ICMP_ECHO_REPLY) { // ICMP回显应答报文
        usID = pIcmpHeader->id;
        usSeqNo = ntohs(pIcmpHeader->seq);
    } else {
        qDebug() << "不是 ICMP_ECHO_REPLY (type=0)，收到 type=" << pIcmpHeader->type;
        return false;
    }

    // 检查ID和序列号以确定收到期待的数据报
    if (usID != (uint16_t) getpid() || usSeqNo != decodeResult.usSeqNo)
    {
        qDebug() << "ID 或序列号不匹配: usID=" << usID << ", usSeqNo=" << usSeqNo;
        return false;
    }
    return true;
}

bool ping(string ip, int timeout) {
    qDebug() << "Ping IP " << ip;
    // 初始化地址
    unsigned long ulDestIp = inet_addr(ip.c_str());
    sockaddr_in destSockAddr;
    destSockAddr.sin_family = AF_INET;
    destSockAddr.sin_addr.s_addr = ulDestIp;
    int rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (rawSocket < 0) {
        qDebug() << "socket creation failed: " << strerror(errno);
        return false;
    }
    // 设置超时时间
    struct timeval tv;
    tv.tv_sec = timeout / 1000;  // 毫秒转秒
    tv.tv_usec = (timeout % 1000) * 1000;  // 剩余毫秒转微秒
    setsockopt(rawSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(rawSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // 定义发送接收缓冲区
    char icmpSendBuf[sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE];
    memset(icmpSendBuf, 0, sizeof(icmpSendBuf));
    char icmpRecvBuf[MAX_ICMP_PACKET_SIZE];
    memset(icmpRecvBuf, 0, sizeof(icmpRecvBuf));

    // 填充ICMP报文
    ICMP_HEADER* icmpHeader = (ICMP_HEADER*)icmpSendBuf;
    icmpHeader->type = ICMP_ECHO_REQUEST;   // 类型为请求回显
    icmpHeader->code = 0;   // 代码设为0
    icmpHeader->id = getpid();  // id设为当前进程PID
    memset(icmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE); // 填充数据字段

    unsigned short usSeqNo = 0; // ICMP报文序列号
    // 填充ICMP报文其他字段
    ((ICMP_HEADER*)icmpSendBuf)->cksum = 0;
    ((ICMP_HEADER*)icmpSendBuf)->seq = htons(usSeqNo);
    ((ICMP_HEADER*)icmpSendBuf)->cksum = checksum((uint16_t*)icmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE);

    // 初始化decodeResult
    DECODE_RESULT decodeResult;
    decodeResult.usSeqNo = usSeqNo;

    // 发送ICMP报文
    int l = sendto(rawSocket, icmpSendBuf, sizeof(icmpSendBuf), 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr));
    if (l == -1) {
        qDebug() << "sendto: " << strerror(errno);
        return false;
    }

    // 接收ICMP报文并解析
    int readDataLen;
    sockaddr_in from;
    unsigned int fromLen = sizeof(from);
    readDataLen = recvfrom(rawSocket, icmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (sockaddr*)&from, &fromLen);
    if (readDataLen > 0)
    {
        // 对数据包进行解码和检验
        if (decodeIcmpResponse(icmpRecvBuf, readDataLen, decodeResult)) {
            return true;
        } else {
            qDebug() << "decodeIcmpResponse 返回 false";
        }
    } else if (readDataLen < 0) {
        qDebug() << "recvfrom: " << strerror(errno);
    }
    close(rawSocket);
    return false;
}

string getmac(string ip) {
    // 打开文件
    ifstream arpFile("/proc/net/arp");
    if (!arpFile.is_open()) {
        qDebug() << "无法打开/proc/net/arp文件";
        return "";
    }
    // 寻找和ip匹配的行
    string line;
    while (getline(arpFile, line)) {
        if (!line.starts_with(ip)) {
            continue;
        }
        // 已找到匹配项
        istringstream iss(line);
        string ip, hwtype, flags, hwaddr, mask, device;
        iss >> ip >> hwtype >> flags >> hwaddr >> mask >> device;
        return hwaddr;
    }
    return "";
}

string gethostname(string ip)
{
    sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = 0;
    char hostname[128];
    memset(hostname, 0, sizeof(hostname));
    // 解析IP地址
    inet_pton(AF_INET, ip.c_str(), &sockAddr.sin_addr);
    if (getnameinfo((sockaddr*)&sockAddr, sizeof(sockAddr), hostname, sizeof(hostname), NULL, 0, 0) != 0) {
        return "";
    }
    return string(hostname);
}