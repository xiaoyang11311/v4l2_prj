#include "log.h"
#include "socket.h"
#include "rtsp.h"
#include "v4l2_capture.h"
#include "h264_encode.h"

int main()
{
    //ip和端口
    string ip = "192.168.31.128";
    int port = 8554;

    char clientIP[16] = {0};
    int clientPort = 0;

    //socket
    int sockfd = createTcpSocket();
    bindSocketAddr(sockfd, ip, port);

    if(listen(sockfd, 1024) < 0)
    {
        printf("socket listen error: errron=%d errmsg=%s\n", errno, strerror(errno));
        return -1;
    }
    else
    {
        printf("socket listening ...\n");
    }

    int clientfd = acceptClient(sockfd, clientIP, &clientPort);

    //处理数据收发
    doClient(clientfd, clientIP, clientPort);

    return 0;
}