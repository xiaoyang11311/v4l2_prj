#ifndef SOCKET_H
#define SOCKET_H

#include "log.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int createTcpSocket();
int creatUdpSocket();
int bindSocketAddr(int sockfd, string ip, int port);
int acceptClient(int sockfd, char* ip, int* port);

#endif