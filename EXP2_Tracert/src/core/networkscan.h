#ifndef NETWORKSCAN_H
#define NETWORKSCAN_H

#include <string>

using std::string;

/**
 * 对给定的IP进行Ping，并指定超时毫秒数
 */
bool ping(string ip, int timeout);

#endif