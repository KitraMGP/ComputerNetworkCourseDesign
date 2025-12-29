#include <stdio.h>
#include <string.h>
#include "server.h"
#include "client.h"

void print_usage()
{
    printf("用法：\n");
    printf("chat --client\t启动客户端\n");
    printf("chat --server\t启动服务端\n");
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        print_usage();
        return 0;
    }
    if (strcmp(argv[1], "--client") == 0)
    {
        return client_main();
    }
    else if (strcmp(argv[1], "--server") == 0)
    {
        return server_main();
    }
    else
    {
        print_usage();
        return 0;
    }

    return 0;
}