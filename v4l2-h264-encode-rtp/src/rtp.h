#ifndef RTP_H
#define RTP_H

#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define RTP_VESION                 2
#define RTP_PAYLOAD_TYPE_H264       96
#define RTP_PAYLOAD_TYPE_AAC        97

#define RTP_HEADER_SIZE             12
#define RTP_MAX_PKT_SIZE            1400

 /*
  *    0                   1                   2                   3
  *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |                           timestamp                           |
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |           synchronization source (SSRC) identifier            |
  *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
  *   |            contributing source (CSRC) identifiers             |
  *   :                             ....                              :
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *
  */

struct RtpHeader
{
    //byte 0
    uint8_t csrcLen : 4;
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;

    //byte 1
    uint8_t payloadType : 7;
    uint8_t market : 1;

    //byte 2-3
    uint16_t seq;

    //byte 4-7
    uint32_t timestamp;

    //byte 8-11
    uint32_t ssrc;
};

struct RtpPacket
{
    RtpHeader rtpHeader;
    uint8_t payload[0];
};

void rtpHeaderInit(RtpPacket *rtpPacket, uint8_t csrcLen, uint8_t extension, 
    uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t market, 
    uint16_t seq, uint32_t timestamp, uint32_t ssrc);

int rtpSendPacketOverUdp(int serverRtpSockfd, const char *ip, int16_t port, RtpPacket *rtpPacket, uint32_t dataSize);



#endif