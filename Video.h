
#ifndef VIDEO_H
#define VIDEO_H

#include "PacketQueue.h"
#include "FrameQueue.h"
#include "Media.h"

extern "C"{

#include <libswscale\swscale.h>
#include <libavutil\time.h>

}
struct MediaState;
/**
 * ������Ƶ��������ݷ�װ
 */
struct VideoState
{
	PacketQueue* videoq;        // �����video packet�Ķ��л���

	int stream_index;           // index of video stream
	AVCodecContext *video_ctx;  // have already be opened by avcodec_open2
	AVStream *stream;           // video stream

	FrameQueue frameq;          // ���������ԭʼ֡����
	AVFrame *frame;
	AVFrame *displayFrame;
    SwsContext *sws_ctx;

	double frame_timer;         // Sync fields
	double frame_last_pts;
	double frame_last_delay;
	double video_clock;

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *bmp;
	SDL_Rect rect;
    MediaState *media_state;
    SDL_Thread * thread;

    void video_play();

	double synchronize(AVFrame *srcFrame, double pts);

    VideoState(MediaState *media);

    ~VideoState();
};


int video_decode(void *arg); // ��packet���룬����������Frame����FrameQueue������


#endif
