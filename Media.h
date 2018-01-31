
#ifndef MEDIA_H
#define MEDIA_H

#include <string>
#include "Audio.h"
#include "Video.h"

extern "C" {

#include <libavformat\avformat.h>

}

struct VideoState;

struct MediaState
{
	AudioState *audio;
	VideoState *video;
	AVFormatContext *pFormatCtx;
    SDL_cond *continue_read_thread;
	char* filename;
    bool demuxFinish;
    bool quit;
    SDL_Thread *thread;
	MediaState(char *filename);
    void Start();
    bool ProcEvent(SDL_Event &event);
    bool PlayFinish();

    ~MediaState();

	bool openInput();
};

int demux_thread(void *data);

#endif
