
#include "Audio.h"

#include <iostream>
#include <fstream>
extern "C" {

#include <libswresample\swresample.h>

}
/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

extern bool quit;

AudioState::AudioState()
    :BUFFER_SIZE(192000 * 2)
{
    audio_ctx = NULL;
    stream_index = -1;
    stream = NULL;
    audio_clock = 0;

    audio_buff = new uint8_t[BUFFER_SIZE];
    audio_buff_size = 0;
    audio_buff_index = 0;
    audio_volume = 80;//SDL_MIX_MAXVOLUME;


    swr_ctx = NULL;
}

AudioState::AudioState(AVCodecContext *audioCtx, int index, int volume)
    :BUFFER_SIZE(192000)
{
    audio_ctx = audioCtx;
    stream_index = index;

    audio_volume = volume;
    audio_buff = new uint8_t[BUFFER_SIZE];
    audio_buff_size = 0;
    audio_buff_index = 0;
}

AudioState::~AudioState()
{
    if (audio_buff)
        delete[] audio_buff;

    swr_free(&swr_ctx);
}

bool AudioState::audio_play()
{
    if (NULL == audio_ctx) {
        return true;
    }
    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;
    SDL_AudioSpec wanted_spec, spec;
    int64_t wanted_channel_layout = audio_ctx->channel_layout,
            wanted_nb_channels = audio_ctx->channels,
            wanted_sample_rate = audio_ctx->sample_rate;

    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = wanted_sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = this;
    while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
               wanted_spec.channels, wanted_spec.freq, SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                av_log(NULL, AV_LOG_ERROR,
                       "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }

    SDL_PauseAudio(0); // playing

    return true;
}

double AudioState::get_audio_clock()
{
    int hw_buf_size = audio_buff_size - audio_buff_index;
    int bytes_per_sec = stream->codec->sample_rate * audio_ctx->channels * 2;

    double pts = audio_clock - static_cast<double>(hw_buf_size) / bytes_per_sec;


    return pts;
}

/**
* 向设备发送audio数据的回调函数
*/
void audio_callback(void* userdata, Uint8 *stream, int len)
{
    AudioState *audio_state = (AudioState*)userdata;

//    SDL_memset(stream, 0, len);

    int audio_size = 0;
    int len1 = 0;
    while (len > 0)// 向设备发送长度为len的数据
    {
        if (audio_state->audio_buff_index >= audio_state->audio_buff_size) // 缓冲区中无数据
        {
            // 从packet中解码数据
            audio_size = audio_decode_frame(audio_state, audio_state->audio_buff, sizeof(audio_state->audio_buff));
            if (audio_size < 0) // 没有解码到数据或出错，填充0
            {
                audio_state->audio_buff_size = 0;
                memset(audio_state->audio_buff, 0, audio_state->audio_buff_size);
            }
            else
                audio_state->audio_buff_size = audio_size;

            audio_state->audio_buff_index = 0;
        }
        len1 = audio_state->audio_buff_size - audio_state->audio_buff_index; // 缓冲区中剩下的数据长度
        if (len1 > len) // 向设备发送的数据长度为len
            len1 = len;
        if (audio_state->audio_buff && audio_state->audio_volume == SDL_MIX_MAXVOLUME){
            memcpy(stream, (uint8_t *)audio_state->audio_buff + audio_state->audio_buff_index, len1);
        } else {
            memset(stream, 0, len1);
            if (audio_state->audio_buff)
                SDL_MixAudio(stream, (uint8_t *)audio_state->audio_buff + audio_state->audio_buff_index, len1, audio_state->audio_volume);
        }

        len -= len1;
        stream += len1;
        audio_state->audio_buff_index += len1;
    }
}

int audio_decode_frame(AudioState *audio_state, uint8_t *audio_buf, int buf_size)
{
    AVFrame *frame = av_frame_alloc();
    int data_size = 0;
    AVPacket pkt;

    static double clock = 0;

    if (quit)
        return -1;
    if (!audio_state->audioq.deQueue(&pkt, true))
        return -1;

    if (pkt.pts != AV_NOPTS_VALUE)
    {
        audio_state->audio_clock = av_q2d(audio_state->stream->time_base) * pkt.pts;
    }
    //TODO
    //	int ret = avcodec_send_packet(audio_state->audio_ctx, &pkt);
    //	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    //		return -1;

    //	ret = avcodec_receive_frame(audio_state->audio_ctx, frame);
    //	if (ret < 0 && ret != AVERROR_EOF)
    //		return -1;
    int got_picture;
    int ret = avcodec_decode_audio4( audio_state->audio_ctx, frame,&got_picture, &pkt);
    if ( ret < 0 ) {
        printf("Error in decoding audio frame.\n");
        return -1;
    }
    if (frame->sample_rate != audio_state->audio_ctx->sample_rate) {
        printf("sample_rate:%d need:%d not match.\n", frame->sample_rate, audio_state->audio_ctx->sample_rate);
        return -1;
    }

    // 设置通道数或channel_layout
    if (frame->channels > 0 && frame->channel_layout == 0)
        frame->channel_layout = av_get_default_channel_layout(frame->channels);
    else if (frame->channels == 0 && frame->channel_layout > 0)
        frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);

    AVSampleFormat dst_format = AV_SAMPLE_FMT_S16;//av_get_packed_sample_fmt((AVSampleFormat)frame->format);
    Uint64 dst_layout = av_get_default_channel_layout(frame->channels);
    // 设置转换参数
    if (NULL == audio_state->swr_ctx){
        audio_state->swr_ctx = swr_alloc_set_opts(NULL, dst_layout, dst_format, frame->sample_rate,
                                                  frame->channel_layout, (AVSampleFormat)frame->format, frame->sample_rate, 0, NULL);
        if (!audio_state->swr_ctx || swr_init(audio_state->swr_ctx) < 0)
            return -1;
    }

    // 计算转换后的sample个数 a * b / c
    uint64_t dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_state->swr_ctx, frame->sample_rate) +
                                             frame->nb_samples,
                                             frame->sample_rate,
                                             frame->sample_rate, AVRounding(1));
    // 转换，返回值为转换后的sample个数
    int nb = swr_convert(audio_state->swr_ctx, &audio_buf, static_cast<int>(dst_nb_samples),
                         (const uint8_t**)frame->data,
                         frame->nb_samples);
    data_size = frame->channels * nb * av_get_bytes_per_sample(dst_format);

    // 每秒钟音频播放的字节数 sample_rate * channels * sample_format(一个sample占用的字节数)
    audio_state->audio_clock += static_cast<double>(data_size) / (2 * audio_state->stream->codec->channels * audio_state->stream->codec->sample_rate);
    return data_size;
}

