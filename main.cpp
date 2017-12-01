
#include "log.h"
#include <SDL.h>
#include <SDL_thread.h>

#include <iostream>

#include "PacketQueue.h"
#include "Audio.h"
#include "Media.h"
#include "VideoDisplay.h"
extern "C" {

#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libswscale\swscale.h>
#include <libswresample\swresample.h>
#include <libavutil/log.h>
}
using namespace std;

bool quit = false;

int main(int argc, char *argv[])
{
    EnterLine;
    av_log_set_level(AV_LOG_DEBUG);
    EnterLine;
	av_register_all();
    EnterLine;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    EnterLine;
//    SDL_Delay(13000);
    char* filename = "E:\\media\\video\\shishigurenlai.mkv";//;亲家过年DVD国语中字.rmvb
//    char* filename = "E:\\media\\video\\duowei.mp4";
	//char* filename = "F:\\test.rmvb";
	MediaState media(filename);
    SDL_Thread *pDemuxThread = NULL;
    if (media.openInput()){
        pDemuxThread = SDL_CreateThread(demux_thread, "demux_thread", &media); // 创建解码线程，读取packet到队列中缓存
    } else {
        return -1;
    }
//	media.audio->audio_play(); // create audio thread
	media.video->video_play(&media); // create video thread

	AVStream *audio_stream = media.pFormatCtx->streams[media.audio->stream_index];
	AVStream *video_stream = media.pFormatCtx->streams[media.video->stream_index];

	double audio_duration = audio_stream->duration * av_q2d(audio_stream->time_base);
	double video_duration = video_stream->duration * av_q2d(video_stream->time_base);

	cout << "audio时长：" << audio_duration << endl;
	cout << "video时长：" << video_duration << endl;

	SDL_Event event;
	while (true) // SDL event loop
	{
		SDL_WaitEvent(&event);
		switch (event.type)
		{
		case FF_QUIT_EVENT:
		case SDL_QUIT:
			quit = 1;
			SDL_Quit();

			return 0;
			break;

		case FF_REFRESH_EVENT:
			video_refresh_timer(&media);
			break;

		default:
			break;
        }
	}
    SDL_WaitThread(pDemuxThread, NULL);

	return 0;
}
