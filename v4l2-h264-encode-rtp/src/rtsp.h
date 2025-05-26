#ifndef RTSP_H
#define RTSP_H

#include "rtp.h"
#include "log.h"
#include "socket.h"

#define SERVER_PORT      8554

#define SERVER_RTP_PORT  55532
#define SERVER_RTCP_PORT 55533
#define BUF_MAX_SIZE     (1024*1024)

static int handleCmd_OPTIONS(char* result, int cseq); //����ͻ��˵�OPTIONS����

static int rtpSendH264Frame(int serverRtpSockfd, const char *ip, int16_t port, 
    struct RtpPacket *rtpPacket, uint8_t *nalData, int nalSize);  //һ֡h264���ݴ���

static int handleCmd_DESCRIBE(char* result, int cseq, char* url); //����ͻ���DESCRIBE����

static int handleCmd_SETUP(char* result, int cseq, int clientRtpPort); //����ͻ���SETUP����

static int handleCmd_PLAY(char* result, int cseq); //����ͻ���PLAY����

void doClient(int clientSockfd, const char* clientIP, int clientPort); //����ͻ��˸��������ִ�к���

#endif