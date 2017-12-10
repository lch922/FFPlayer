
#include "log.h"
#include <SDL.h>
#include <SDL_thread.h>

#include <iostream>

#include "PacketQueue.h"
#include "Audio.h"
#include "Media.h"
extern "C" {

#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libswscale\swscale.h>
#include <libswresample\swresample.h>
#include <libavutil/log.h>
}
using namespace std;


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
    //    char* filename = "E:\\media\\video\\shishigurenlai.mkv";//;亲家过年DVD国语中字.rmvb
    //    char* filename = "E:\\media\\video\\duowei.mp4";
    //char* filename = "F:\\test.rmvb";
    while (true) {
        //        char* filename = "E:\\media\\video\\hight\\AvatarTest_30.mp4";
        char* filename = "E:\\media\\video\\fanbingbing_1.mp4";
        MediaState media(filename);
        if (media.openInput()){

        } else {
            return -1;
        }
        media.Loop();
    }

    return 0;
}
