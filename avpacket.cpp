#include "avpacket.h"


AVPacket *av_packet_alloc(void)
{
    AVPacket *pkt = (AVPacket *)av_mallocz(sizeof(AVPacket));
    if (!pkt)
        return pkt;

    av_packet_unref(pkt);

    return pkt;
}

void av_packet_free(AVPacket **pkt)
{
    if (!pkt || !*pkt)
        return;

    av_packet_unref(*pkt);
    av_freep(pkt);
}
