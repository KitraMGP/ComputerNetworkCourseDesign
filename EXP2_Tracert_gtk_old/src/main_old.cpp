#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#include <signal.h>

using namespace std;

// 全局变量，用于socket清理
int sockfd = -1;

// 信号处理函数，用于优雅退出
void signal_handler(int signo) {
    if (sockfd != -1) {
        close(sockfd);
    }
    exit(0);
}

// 计算互联网校验和
uint16_t checksum(void *addr, int len) {
    uint16_t *buf = (uint16_t *)addr;
    uint32_t sum = 0;
    uint16_t result;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(uint8_t *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// 获取当前时间（毫秒）
long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 解析主机名到IP地址
bool resolve_host(const string &host, struct in_addr *addr) {
    struct hostent *he = gethostbyname(host.c_str());
    if (he == nullptr) {
        return false;
    }
    memcpy(addr, he->h_addr_list[0], he->h_length);
    return true;
}

int main_old(int argc, char *argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);

    // 检查参数
    if (argc != 2) {
        cerr << "用法: " << argv[0] << " <主机>" << endl;
        cerr << "示例: " << argv[0] << " www.baidu.com" << endl;
        cerr << "注意: 此程序需要 root 权限（使用 sudo 运行）" << endl;
        return 1;
    }

    string target_host = argv[1];
    struct in_addr target_addr;

    // 解析目标主机
    if (!resolve_host(target_host, &target_addr)) {
        cerr << "错误: 无法解析主机: " << target_host << endl;
        return 1;
    }

    cout << "追踪到 " << inet_ntoa(target_addr) << " (" << target_host << ")" << endl;
    cout << string(60, '-') << endl;

    // 创建 raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        if (errno == EPERM) {
            cerr << "错误: 权限被拒绝。此程序需要 root 权限。" << endl;
            cerr << "请使用 sudo 运行: sudo " << argv[0] << " " << target_host << endl;
        } else {
            cerr << "错误: 无法创建 socket: " << strerror(errno) << endl;
        }
        return 1;
    }

    // 设置接收超时
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "错误: 无法设置 socket 超时: " << strerror(errno) << endl;
        close(sockfd);
        return 1;
    }

    const int MAX_HOPS = 30;
    const int PACKETS_PER_HOP = 3;
    const pid_t pid = getpid();
    uint16_t seq = 0;
    bool reached_destination = false;

    // 主循环：逐跳探测
    for (int ttl = 1; ttl <= MAX_HOPS && !reached_destination; ttl++) {
        // 设置 TTL
        if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
            cerr << "错误: 无法设置 TTL: " << strerror(errno) << endl;
            break;
        }

        cout << right << setw(2) << ttl << "  ";

        struct sockaddr_in dest_addr;
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr = target_addr;

        vector<string> responses;
        vector<long> rtt_times;

        // 每跳发送3个包
        for (int p = 0; p < PACKETS_PER_HOP; p++) {
            // 构建 ICMP 包
            char packet[64];
            memset(packet, 0, sizeof(packet));

            struct icmphdr *icmp = (struct icmphdr *)packet;
            icmp->type = ICMP_ECHO;
            icmp->code = 0;
            icmp->un.echo.id = pid;
            icmp->un.echo.sequence = seq++;
            icmp->checksum = 0;

            // 添加一些数据
            memset(packet + sizeof(struct icmphdr), 0xAA, sizeof(packet) - sizeof(struct icmphdr));

            // 计算校验和
            icmp->checksum = checksum(packet, sizeof(packet));

            // 发送包
            long send_time = get_current_time_ms();
            if (sendto(sockfd, packet, sizeof(packet), 0,
                      (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
                cout << left << setw(27) << "* " << flush;
                continue;
            }

            // 接收响应
            char recv_buf[512];
            struct sockaddr_in recv_addr;
            socklen_t addr_len = sizeof(recv_addr);

            int recv_len = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0,
                                    (struct sockaddr *)&recv_addr, &addr_len);

            if (recv_len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    cout << left << setw(27) << "* " << flush;
                } else {
                    cout << left << setw(27) << "* " << flush;
                }
                continue;
            }

            long recv_time = get_current_time_ms();
            long rtt = recv_time - send_time;

            // 解析 IP 头
            struct iphdr *ip = (struct iphdr *)recv_buf;
            int ip_header_len = ip->ihl * 4;

            // 解析 ICMP 包
            struct icmphdr *recv_icmp = (struct icmphdr *)(recv_buf + ip_header_len);

            // 跳过原始 IP 头，找到 ICMP payload
            struct iphdr *orig_ip = nullptr;
            struct icmphdr *orig_icmp = nullptr;

            if (recv_icmp->type == ICMP_TIME_EXCEEDED) {
                // Time Exceeded: payload contains original IP header + ICMP header
                orig_ip = (struct iphdr *)(recv_buf + ip_header_len + 8); // +8 for ICMP header
                int orig_ip_header_len = orig_ip->ihl * 4;
                orig_icmp = (struct icmphdr *)((char *)orig_ip + orig_ip_header_len);

                // 检查是否是我们的包
                if (orig_icmp->un.echo.id == pid) {
                    string ip_str = inet_ntoa(recv_addr.sin_addr);
                    responses.push_back(ip_str);
                    rtt_times.push_back(rtt);
                    cout << left << setw(15) << ip_str << "  " << left << setw(8) << (to_string(rtt) + "ms") << "  " << flush;
                }
            } else if (recv_icmp->type == ICMP_ECHOREPLY) {
                // Echo Reply: reached destination
                if (recv_icmp->un.echo.id == pid) {
                    string ip_str = inet_ntoa(recv_addr.sin_addr);
                    responses.push_back(ip_str);
                    rtt_times.push_back(rtt);
                    cout << left << setw(15) << ip_str << "  " << left << setw(8) << (to_string(rtt) + "ms") << "  " << flush;
                    reached_destination = true;
                }
            }
        }

        cout << endl;

        // 如果到达目的地，退出
        if (reached_destination) {
            break;
        }

        // 短暂延迟，避免发送过快
        usleep(100000); // 100ms
    }

    if (sockfd != -1) {
        close(sockfd);
    }

    return 0;
}
