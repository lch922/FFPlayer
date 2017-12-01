#ifndef AVPACKET_H
#define AVPACKET_H
extern "C"{
#include <libavcodec/avcodec.h>
}
AVPacket *av_packet_alloc(void);
void av_packet_free(AVPacket **pkt);

#endif // AVPACKET_H
