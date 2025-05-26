#ifndef RTSP_H
#define RTSP_H

#include "rtp.h"
#include "log.h"
#include "socket.h"

#define SERVER_PORT      8554

#define SERVER_RTP_PORT  55532
#define SERVER_RTCP_PORT 55533
#define BUF_MAX_SIZE     (1024*1024)

static int handleCmd_OPTIONS(char* result, int cseq); //处理客户端的OPTIONS请求

static int rtpSendH264Frame(int serverRtpSockfd, const char *ip, int16_t port, 
    struct RtpPacket *rtpPacket, uint8_t *nalData, int nalSize);  //一帧h264数据传输

static int handleCmd_DESCRIBE(char* result, int cseq, char* url); //处理客户端DESCRIBE请求

static int handleCmd_SETUP(char* result, int cseq, int clientRtpPort); //处理客户端SETUP请求

static int handleCmd_PLAY(char* result, int cseq); //处理客户端PLAY请求

void doClient(int clientSockfd, const char* clientIP, int clientPort); //处理客户端各种请求的执行函数

#endif