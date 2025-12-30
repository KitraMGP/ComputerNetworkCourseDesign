#ifndef PACKET_H
#define PACKET_H

#include <stddef.h>
#include <unistd.h>

ssize_t send_msg(int sock, const char *buf, size_t len);
ssize_t recv_msg(int sock, char *buf, size_t max_len);

#endif
