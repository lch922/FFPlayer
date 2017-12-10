
#include "PacketQueue.h"
#include "avpacket.h"
#include "Media.h"
#include <iostream>


PacketQueue::PacketQueue(MediaState *media)
{
	nb_packets = 0;
	size       = 0;
    duration = 0;
    media_state = media;
	mutex      = SDL_CreateMutex();
    cond       = SDL_CreateCond();
}

PacketQueue::~PacketQueue()
{
    SDL_DestroyCond(cond);
    SDL_DestroyMutex(mutex);
}

bool PacketQueue::enQueue(const AVPacket *packet)
{
	AVPacket *pkt = av_packet_alloc();
    if (packet->buf != NULL && packet->size > 0){
        if (av_packet_ref(pkt, packet) < 0)
            return false;
    }

	SDL_LockMutex(mutex);
	queue.push(*pkt);

	size += pkt->size;
	nb_packets++;
    duration += pkt->duration;
	SDL_CondSignal(cond);
	SDL_UnlockMutex(mutex);
	return true;
}

bool PacketQueue::deQueue(AVPacket *packet, bool block)
{
	bool ret = false;

	SDL_LockMutex(mutex);
	while (true)
    {
        if (media_state->quit)
        {
            ret = false;
            break;
        }

        if (!queue.empty())
        {
            AVPacket pkt = queue.front();
            queue.pop();
            if (pkt.buf == NULL && pkt.size == 0) {
                //≤•∑≈ÕÍ≥…
                *packet = pkt;
                ret = true;
                break;
            }
            if (av_packet_ref(packet, &pkt) < 0)
            {
                ret = false;
                break;
            }

            av_packet_unref(&pkt);
            nb_packets--;
            duration -= pkt.duration;
            size -= packet->size;

            ret = true;
            break;
        }
        else if (!block)
        {
            ret = false;
            break;
        }
        else
        {
            SDL_CondSignal(empty_queue_cond);
            SDL_CondWait(cond, mutex);
        }
    }
	SDL_UnlockMutex(mutex);
	return ret;
}
