
#include "Media.h"
#include "avpacket.h"
#include <iostream>

extern "C"{
#include <libavutil/time.h>
}
extern bool quit;
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25



MediaState::MediaState(char* input_file)
	:filename(input_file)
{
	pFormatCtx = NULL;
	audio = new AudioState();

	video = new VideoState();

	//quit = false;
}

MediaState::~MediaState()
{
	if(audio)
		delete audio;

	if (video)
		delete video;
}

bool MediaState::openInput()
{
    if (!(continue_read_thread = SDL_CreateCond())) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
        return false;
    }
    audio->audioq.empty_queue_cond = continue_read_thread;
    video->videoq->empty_queue_cond = continue_read_thread;
	// Open input file
	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) < 0)
		return false;

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		return false;

	// Output the stream info to standard 
	av_dump_format(pFormatCtx, 0, filename, 0);

//	for (uint32_t i = 0; i < pFormatCtx->nb_streams; i++)
//	{
//        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio->stream_index < 0)
//			audio->stream_index = i;

//		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video->stream_index < 0)
//			video->stream_index = i;
//	}
    video->stream_index =
        av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO,
                            video->stream_index, -1, NULL, 0);
    audio->stream_index =
        av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO,
                            audio->stream_index,
                            video->stream_index,
                            NULL, 0);
	if (audio->stream_index < 0 || video->stream_index < 0)
		return false;

	// Fill audio state
	AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[audio->stream_index]->codec->codec_id);
	if (!pCodec)
		return false;

	audio->stream = pFormatCtx->streams[audio->stream_index];

	audio->audio_ctx = avcodec_alloc_context3(pCodec);
	if (avcodec_copy_context(audio->audio_ctx, pFormatCtx->streams[audio->stream_index]->codec) != 0)
		return false;

	avcodec_open2(audio->audio_ctx, pCodec, NULL);

	// Fill video state
	AVCodec *pVCodec = avcodec_find_decoder(pFormatCtx->streams[video->stream_index]->codec->codec_id);
	if (!pVCodec)
		return false;

	video->stream = pFormatCtx->streams[video->stream_index];

	video->video_ctx = avcodec_alloc_context3(pVCodec);
	if (avcodec_copy_context(video->video_ctx, pFormatCtx->streams[video->stream_index]->codec) != 0)
		return false;

	avcodec_open2(video->video_ctx, pVCodec, NULL);

	video->frame_timer = static_cast<double>(av_gettime()) / 1000000.0;
	video->frame_last_delay = 40e-3;

	return true;
}

static int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
    return stream_id < 0 ||
           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
           queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}
static char err_buf[256] = {0};
static char* av_get_err(int errnum)
{
    av_strerror(errnum, err_buf, sizeof(err_buf));
    return err_buf;
}

int demux_thread(void *data)
{
	MediaState *media = (MediaState*)data;

	AVPacket *packet = av_packet_alloc();
    SDL_mutex *wait_mutex = SDL_CreateMutex();
    if (!wait_mutex) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
        return -1;
    }
	while (true)
	{
        if (media->audio->audioq.nb_packets + media->video->videoq->nb_packets > MAX_QUEUE_SIZE
                    || (stream_has_enough_packets(media->audio->stream, media->audio->stream_index, &media->audio->audioq) &&
                        stream_has_enough_packets(media->video->stream, media->video->stream_index, media->video->videoq))) {
            /* wait 10 ms */
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(media->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
			continue;
        }
		int ret = av_read_frame(media->pFormatCtx, packet);
		if (ret < 0)
		{
            printf("av_err2str:%s", av_get_err(ret));
			if (ret == AVERROR_EOF)
				break;
			if (media->pFormatCtx->pb->error == 0) // No error,wait for user input
			{
				SDL_Delay(100);
				continue;
			}
			else
				break;
		}
        if (packet->stream_index == media->audio->stream_index) {
            // audio stream
			media->audio->audioq.enQueue(packet);
            av_packet_unref(packet);
        } else if (packet->stream_index == media->video->stream_index) {
            // video stream
			media->video->videoq->enQueue(packet);
            av_packet_unref(packet);
		}		
		else
			av_packet_unref(packet);
	}

	av_packet_free(&packet);

	return 0;
}
