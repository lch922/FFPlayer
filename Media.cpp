
#include "Media.h"
#include "avpacket.h"
#include "VideoDisplay.h"
#include <iostream>
#include "log.h"
extern "C"{
#include <libavutil/time.h>
}
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25



MediaState::MediaState(char* input_file)
    :filename(input_file)
{
	pFormatCtx = NULL;
    audio = new AudioState(this);
    video = new VideoState(this);
    demuxFinish = false;
    quit = false;
    thread = NULL;
}

int MediaState::Loop()
{
    video->video_play(); // create video thread

    SDL_Event event;
    while (true) // SDL event loop
    {
        if (demuxFinish && video->videoq->queue.empty() && audio->audioq.queue.empty()) {
            qDebug() << "play finish";
            break;
        }
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
            video_refresh_timer(this);
            break;

        default:
            break;
        }
    }
}

MediaState::~MediaState()
{
    SDL_DestroyCond(continue_read_thread);
	if(audio)
        delete audio;

	if (video)
        delete video;
    if (NULL != pFormatCtx){
        avformat_close_input(&pFormatCtx);
        pFormatCtx = NULL;
    }

    if (thread == NULL) {
        SDL_WaitThread(thread, NULL);
    }
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
//    {
//        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio->stream_index < 0)
//        	audio->stream_index = i;

//    	if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video->stream_index < 0)
//        	video->stream_index = i;
//    }
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
    // 创建解码线程，读取packet到队列中缓存
    thread = SDL_CreateThread(demux_thread, "demux_thread", this);


    AVStream *audio_stream = pFormatCtx->streams[audio->stream_index];
    AVStream *video_stream = pFormatCtx->streams[video->stream_index];

    double audio_duration = audio_stream->duration * av_q2d(audio_stream->time_base);
    double video_duration = video_stream->duration * av_q2d(video_stream->time_base);
    LogDebug("audio_duration:%f, video_duration:%f", audio_duration, video_duration);
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
    media->demuxFinish = true;
    AVPacket nullPacket;
    nullPacket.buf = NULL;
    nullPacket.size = 0;
    nullPacket.duration = 0;
    nullPacket.stream_index = media->audio->stream_index;
    media->audio->audioq.enQueue(&nullPacket);
    nullPacket.stream_index = media->video->stream_index;
    media->video->videoq->enQueue(&nullPacket);
	return 0;
}
