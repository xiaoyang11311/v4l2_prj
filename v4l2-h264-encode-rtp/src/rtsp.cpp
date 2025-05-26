#include "rtsp.h"
#include "h264_encode.h"
#include "v4l2_capture.h"

//base64编码
/* int base64_encode(const unsigned char *in, int in_len, char *out)
{
    const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int out_len = 0;
    int i;
    for (i = 0; i < in_len; i += 3) {
        int val = 0;
        int len = 0;
        for (int j = 0; j < 3; j++) {
            val <<= 8;
            if (i + j < in_len) {
                val |= in[i + j];
                len++;
            }
        }

        for (int j = 0; j < 4; j++) {
            if (j <= (len + 0)) {
                out[out_len++] = base64_table[(val >> (18 - j * 6)) & 0x3F];
            } else {
                out[out_len++] = '=';
            }
        }
    }
    out[out_len] = '\0'; // 结束符
    return out_len;
} */

static int handleCmd_OPTIONS(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
        "\r\n",
        cseq);

    return 0;
}

static int rtpSendH264Frame(int serverRtpSockfd, const char *ip, int16_t port, 
    struct RtpPacket *rtpPacket, uint8_t *nalData, int nalSize)  //发送单个nal
{
    uint8_t naluType = nalData[0];
/*     printf("nalData: ");
    for (int i = 0; i < nalSize && i < 30; ++i) {
        printf("%02X ", nalData[i]);
    }
    printf("(size=%d)\n", nalSize); */
    int sendBytes = 0;
    int ret;


    printf("frameSize=%d\n", nalSize);

    if(nalSize <= RTP_MAX_PKT_SIZE)
    {
        //*   0 1 2 3 4 5 6 7 8 9
         //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         //*  |F|NRI|  Type   | a single NAL unit ... |
         //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        memcpy(rtpPacket->payload, nalData, nalSize);
        ret = rtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, nalSize);
        if(ret < 0)
            return -1;

        rtpPacket->rtpHeader.seq++;
        sendBytes += ret;
        if((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8)
            goto out;
    }
    else
    {
         //*  0                   1                   2
         //*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
         //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         //* | FU indicator  |   FU header   |   FU payload   ...  |
         //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



         //*     FU Indicator
         //*    0 1 2 3 4 5 6 7
         //*   +-+-+-+-+-+-+-+-+
         //*   |F|NRI|  Type   |
         //*   +---------------+



         //*      FU Header
         //*    0 1 2 3 4 5 6 7
         //*   +-+-+-+-+-+-+-+-+
         //*   |S|E|R|  Type   |
         //*            
        
        int pktNum = nalSize / RTP_MAX_PKT_SIZE;
        int remainPktSize = nalSize % RTP_MAX_PKT_SIZE;
        int pos = 1;

        for(int i = 0; i < pktNum; i++)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;

            if(i == 0)
                rtpPacket->payload[1] |= 0x80;
            else if(remainPktSize == 0 && i == pktNum -1)
                rtpPacket->payload[1] |= 0x40;

            memcpy(rtpPacket->payload + 2, nalData + pos, RTP_MAX_PKT_SIZE);
            ret = rtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, RTP_MAX_PKT_SIZE + 2); 
            if(ret < 0)
                return -1;
            
            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
            pos += RTP_MAX_PKT_SIZE;
        }

        if(remainPktSize > 0)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            rtpPacket->payload[1] |= 0x40;

            memcpy(rtpPacket->payload + 2, nalData + pos, remainPktSize);
            ret = rtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, remainPktSize + 2); 
            if(ret < 0)
                return -1;
            
            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;               
        }
    }

    LOGI("rtpSendH264Frame: sendBytes=%d, timestamp=%d", sendBytes, rtpPacket->rtpHeader.timestamp);

    out:

    return sendBytes;
}

static int handleCmd_DESCRIBE(char* result, int cseq, char* url)
{
/*     //base编码

    //提前取的sps的数据
    unsigned char sps[] = {
        0x67, 0x42, 0xC0, 0x14, 0xDA, 0x0A, 0x11, 0xF9,
        0x61, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00,
        0x03, 0x00, 0x3C, 0x8F, 0x14, 0x2A, 0xA0
    };
    //提前取的pps的数据
    unsigned char pps[] = { 0x68, 0xCE, 0x3C, 0x80 };

    char sps_b64[128] = {0};
    char pps_b64[128] = {0};

    base64_encode(sps, sizeof(sps), sps_b64);
    base64_encode(pps, sizeof(pps), pps_b64); */

    //sdp填写

    char sdp[1024];
    char localIp[100];

    sscanf(url, "rtsp://%[^:]:", localIp);
 
    sprintf(sdp, "v=0\r\n"
        "o=- 9%ld 1 IN IP4 %s\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "m=video 0 RTP/AVP 96\r\n"
        "a=rtpmap:96 H264/90000\r\n"
        //"a=fmtp:96 packetization-mode=1; sprop-parameter-sets=%s,%s; profile-level-id=42A01E\r\n"
        //"a=framesize:96 160-120\r\n"
        "a=control:track0\r\n",
        time(NULL), localIp);
        //time(NULL), localIp, sps_b64, pps_b64);

    sprintf(result, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
        "Content-Base: %s\r\n"
        "Content-type: application/sdp\r\n"
        "Content-length: %zu\r\n\r\n"
        "%s",
        cseq,
        url,
        strlen(sdp),
        sdp);

    return 0;
}

static int handleCmd_SETUP(char* result, int cseq, int clientRtpPort)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
        "Session: 66334873\r\n"
        "\r\n",
        cseq,
        clientRtpPort,
        clientRtpPort + 1,
        SERVER_RTP_PORT,
        SERVER_RTCP_PORT);

    return 0;
}

static int handleCmd_PLAY(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Range: npt=0.000-\r\n"
        "Session: 66334873; timeout=10\r\n\r\n",
        cseq);

    return 0;
}

void doClient(int clientSockfd, const char* clientIP, int clientPort) {

    char method[40];
    char url[100];
    char version[40];
    int CSeq;

    int serverRtpSockfd = -1, serverRtcpSockfd = -1;
    int clientRtpPort, clientRtcpPort;
    char* rBuf = (char*)malloc(BUF_MAX_SIZE);
    char* sBuf = (char*)malloc(BUF_MAX_SIZE);

    while (true) {
        int recvLen;

        recvLen = recv(clientSockfd, rBuf, BUF_MAX_SIZE, 0);
        if (recvLen <= 0) {
            break;
        }

        rBuf[recvLen] = '\0';
        std::string recvStr = rBuf;
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        printf("%s rBuf = %s \n",__FUNCTION__,rBuf);

        const char* sep = "\n";
        char* line = strtok(rBuf, sep);
        while (line) {
         
            if (strstr(line, "OPTIONS") ||
                strstr(line, "DESCRIBE") ||
                strstr(line, "SETUP") ||
                strstr(line, "PLAY")) {

                if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3) {
                    // error
                }
            }
            else if (strstr(line, "CSeq")) {
                if (sscanf(line, "CSeq: %d\r\n", &CSeq) != 1) {
                    // error
                }
            }
            else if (!strncmp(line, "Transport:", strlen("Transport:"))) {
                // Transport: RTP/AVP/UDP;unicast;client_port=13358-13359
                // Transport: RTP/AVP;unicast;client_port=13358-13359

                if (sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n",
                    &clientRtpPort, &clientRtcpPort) != 2) {
                    // error
                    printf("parse Transport error \n");
                }
            }
            line = strtok(NULL, sep);
        }

        if (!strcmp(method, "OPTIONS")) {
            if (handleCmd_OPTIONS(sBuf, CSeq))
            {
                printf("failed to handle options\n");
                break;
            }
        }
        else if (!strcmp(method, "DESCRIBE")) {
            if (handleCmd_DESCRIBE(sBuf, CSeq, url))
            {
                printf("failed to handle describe\n");
                break;
            }
       
        }
        else if (!strcmp(method, "SETUP")) {
            if (handleCmd_SETUP(sBuf, CSeq, clientRtpPort))
            {
                printf("failed to handle setup\n");
                break;
            }

            serverRtpSockfd = creatUdpSocket();
            serverRtcpSockfd = creatUdpSocket();
            if (serverRtpSockfd < 0 || serverRtcpSockfd < 0)
            {
                printf("failed to create udp socket\n");
                break;
            }

            if (bindSocketAddr(serverRtpSockfd, "0.0.0.0", SERVER_RTP_PORT) < 0 ||
            bindSocketAddr(serverRtcpSockfd, "0.0.0.0", SERVER_RTCP_PORT) < 0)
            {
                printf("failed to bind addr\n");
                break;
            }          
        }
        else if (!strcmp(method, "PLAY")) {
            if (handleCmd_PLAY(sBuf, CSeq))
            {
                printf("failed to handle play\n");
                break;
            }
        }
        else {
            printf("未定义的method = %s \n", method);
            break;
        }
        printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        printf("%s sBuf = %s \n", __FUNCTION__, sBuf);

        send(clientSockfd, sBuf, strlen(sBuf), 0);


        //开始播放，发送RTP包
        if (!strcmp(method, "PLAY")) {
            //编码器初始化
            struct x264_param_t param;
            struct x264_picture_t pic_in;
            struct x264_picture_t pic_out;
            struct x264_t* encoder = NULL;
            int frame_num = 0;
            h264_encoder_init(&param, &pic_in, &encoder, 160, 120, 30);

            //定义nal
            x264_nal_t* nals;
            int i_nals = 0;

            //定义rtppacket
            struct RtpPacket* rtpPacket = (struct RtpPacket*)malloc(500000);

            //摄像头初始化
            int fd = open_dev("/dev/video0");
            struct v4l2_format v4l_format;
            set_v4l2_fmt(fd, 160, 120, &v4l_format);
            unsigned char* mptr[4];
            unsigned int buf_size[4];
            struct v4l2_buffer v4l_buffer;
            v4l2_buf_mmap(fd, mptr, buf_size, &v4l_buffer);
            capture_start(fd);

            rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION , RTP_PAYLOAD_TYPE_H264, 0,
                0, 0, 0x88923423);

            printf("start play\n");
            printf("client ip:%s\n", clientIP);
            printf("client port:%d\n", clientRtpPort);

            //保存流媒体文件
            //FILE *fp = fopen("test.h264", "wb");
            //if (!fp) {
            //    perror("fopen failed");
            //    break;
            //}

            //主循环，处理视频采集的数据并进行编码及发送
            while(1)
            {
                int ret = ioctl(fd, VIDIOC_DQBUF, &v4l_buffer);
                if(ret < 0)
                {
                    perror("VIDIOC_DQBUF failed");
                }

                h264_encode(encoder, &pic_in, &pic_out, &frame_num, mptr[v4l_buffer.index], &nals, &i_nals, 160, 120);
                

                //第一帧nalu存在多个nal(如pps和sps以及SEI和slice)，所以需要设置for循环将其分别发送，且时间戳不递增
                for(int i = 0; i < i_nals; ++i)
                {
                    //uint8_t* nalu_ptr = nals[i].p_payload;
                    //int nalu_size = nals[i].i_payload;
                    //// 写入原始NALU（包含起始码）
                    //fwrite(nalu_ptr, 1, nalu_size, fp);

                    //去掉起始码
                    uint8_t* payload = nals[i].p_payload;
                    int payload_size = nals[i].i_payload;


                    if (payload_size >= 4 && payload[0] == 0x00 && payload[1] == 0x00 && payload[2] == 0x00 && payload[3] == 0x01) {
                        payload += 4;
                        payload_size -= 4;
                    } else if (payload_size >= 3 && payload[0] == 0x00 && payload[1] == 0x00 && payload[2] == 0x01) {
                        payload += 3;
                        payload_size -= 3;
                    }

                    if (rtpSendH264Frame(serverRtpSockfd, clientIP, clientRtpPort, rtpPacket, payload, payload_size) < 0)
                    {
                        LOGE("failed to send rtp packet");
                        break;
                    }
                }

                //更新时间戳
                rtpPacket->rtpHeader.timestamp += 90000 / 25;

                ret = ioctl(fd, VIDIOC_QBUF, &v4l_buffer);
                if(ret < 0)
                {
                    perror("VIDIOC_QBUF failed");
                }
            }

            h264_encoder_release(encoder, &pic_in);

            capture_stop(fd, mptr, buf_size);
        }

        memset(method,0,sizeof(method)/sizeof(char));
        memset(url,0,sizeof(url)/sizeof(char));
        CSeq = 0;

    }

    close(clientSockfd);
    if (serverRtpSockfd) {
        close(serverRtpSockfd);
    }
    if (serverRtcpSockfd > 0) {
        close(serverRtcpSockfd);
    }
    free(rBuf);
    free(sBuf);

}