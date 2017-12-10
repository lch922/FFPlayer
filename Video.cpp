
#include "Video.h"
#include "VideoDisplay.h"

extern "C"{
#include <libswscale\swscale.h>
}

VideoState::VideoState(MediaState *media)
{
	video_ctx        = NULL;
	stream_index     = -1;
	stream           = NULL;

	window           = NULL;
	bmp              = NULL;
	renderer         = NULL;

	frame            = NULL;
	displayFrame     = NULL;
    sws_ctx = NULL;

    videoq           = new PacketQueue(media);
    media_state      = media;
	frame_timer      = 0.0;
	frame_last_delay = 0.0;
	frame_last_pts   = 0.0;
	video_clock      = 0.0;
    thread = NULL;
}

VideoState::~VideoState()
{
	delete videoq;

	av_frame_free(&frame);
	av_free(displayFrame->data[0]);
	av_frame_free(&displayFrame);
    if (NULL != sws_ctx){
        sws_freeContext(sws_ctx);
    }
    if (NULL != video_ctx){
        avcodec_close(video_ctx);
    }
    if (NULL != thread){
        SDL_WaitThread(thread, NULL);
        thread = NULL;
    }
    if (bmp)
        SDL_DestroyTexture(bmp);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
}

void VideoState::video_play()
{
	int width = 800;
	int height = 600;
    // 创建sdl窗口
	window = SDL_CreateWindow("FFmpeg Decode", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height, SDL_WINDOW_OPENGL);
	renderer = SDL_CreateRenderer(window, -1, 0);
    bmp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
        width, height);

	rect.x = 0;
	rect.y = 0;
	rect.w = width;
	rect.h = height;

	frame = av_frame_alloc();
	displayFrame = av_frame_alloc();

	displayFrame->format = AV_PIX_FMT_YUV420P;
	displayFrame->width = width;
	displayFrame->height = height;
	int numBytes = avpicture_get_size((AVPixelFormat)displayFrame->format,displayFrame->width, displayFrame->height);
	uint8_t *buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

	avpicture_fill((AVPicture*)displayFrame, buffer, (AVPixelFormat)displayFrame->format, displayFrame->width, displayFrame->height);

    thread = SDL_CreateThread(video_decode, "video_decode_thread", this);

    schedule_refresh(media_state, 40); // start display
}

double VideoState::synchronize(AVFrame *srcFrame, double pts)
{
	double frame_delay;

	if (pts != 0)
        video_clock = pts; // Get pts,then set video clock to it
	else
        pts = video_clock; // Don't get pts,set it to video clock

	frame_delay = av_q2d(stream->codec->time_base);
	frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

	video_clock += frame_delay;

	return pts;
}


int  video_decode(void *arg)
{
	VideoState *video = (VideoState*)arg;

	AVFrame *frame = av_frame_alloc();
    video->media_state->audio->audio_play();
	AVPacket packet;
	double pts;

	while (true)
    {
        video->videoq->deQueue(&packet, true);
        if (packet.buf == NULL && packet.size == 0) {
            //播放完成
            break;
        }
        //TODO
//    	int ret = avcodec_send_packet(video->video_ctx, &packet);
//    	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
//        	continue;

//    	ret = avcodec_receive_frame(video->video_ctx, frame);
//    	if (ret < 0 && ret != AVERROR_EOF)
//        	continue;
        int got_frame;
        int ret = avcodec_decode_video2(video->video_ctx, frame, &got_frame, &packet);
        if (ret < 0) {
            printf("Error decoding video frame (%d)\n", ret);
            continue;
        }

        if ((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE)
            pts = 0;

        pts *= av_q2d(video->stream->time_base);

        pts = video->synchronize(frame, pts);

        frame->opaque = &pts;

        if (video->frameq.nb_frames >= FrameQueue::capacity)
            SDL_Delay(500 * 2);

        video->frameq.enQueue(frame);

        av_frame_unref(frame);
    }


	av_frame_free(&frame);

	return 0;
}

