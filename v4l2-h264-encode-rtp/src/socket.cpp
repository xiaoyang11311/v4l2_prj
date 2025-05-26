#include "socket.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int createTcpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

    return sockfd;
}

int creatUdpSocket()
{
    int sockfd;
    int on = 1;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        LOGE("socket creat error");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

    return sockfd;    
}

int bindSocketAddr(int sockfd, string ip, int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        LOGE("socket bind error");
        return -1;
    }

    return 0;
}

int acceptClient(int sockfd, char* ip, int* port)
{
    int clientfd = -1;
    socklen_t len = 0;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);

    while(clientfd < 0)
    {
        clientfd = accept(sockfd, (struct sockaddr*)&addr, &len);
    }

    strcpy(ip, inet_ntoa(addr.sin_addr));
    *port = ntohs(addr.sin_port);

    return clientfd;
}