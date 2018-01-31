
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

int Test()
{
    SDL_Window *win_1 = NULL;
    SDL_Window *win_2 = NULL;

    SDL_Event event;
    int width = 800;
    int height = 600;
    int running = 1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == -1) {
        fprintf(stderr, "Initializing video subsystem failed: %s\n",
                SDL_GetError());
        return EXIT_FAILURE;
    }

    atexit(SDL_Quit);

    win_1 = SDL_CreateWindow("Window #1",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             width, height, SDL_WINDOW_OPENGL);
    if (NULL == win_1) {
        fprintf(stderr, "Creating window #1 failed: %s\n",
                SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(win_1, 0, 0);
    SDL_Texture *bmp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
        width, height);

    win_2 = SDL_CreateWindow("Window #2",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             width, height, SDL_WINDOW_OPENGL);
    if (NULL == win_2) {
        SDL_DestroyWindow(win_1);
        fprintf(stderr, "Creating window #2 failed: %s\n",
                SDL_GetError());
        return EXIT_FAILURE;
    }
    renderer = SDL_CreateRenderer(win_2, 0, 0);
    bmp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
        200, 200);

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_WINDOWEVENT:
                if (SDL_WINDOWEVENT_CLOSE == event.window.event) {
                    running = 0;
                }
                break;
            default: break;
            }
        }

        SDL_UpdateWindowSurface(win_1);
        SDL_UpdateWindowSurface(win_2);
    }

    SDL_DestroyWindow(win_1);
    SDL_DestroyWindow(win_2);

    return 0;
}

int main(int argc, char *argv[])
{
//    return Test();
    av_log_set_level(AV_LOG_VERBOSE);
    EnterLine;
    av_register_all();
    EnterLine;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

//    MediaState *media1 = new MediaState("E:\\media\\video\\fanbingbing_1.mp4");
    MediaState *media2 = new MediaState("E:\\media\\video\\062.3gp");
//    media1->Start();
    media2->Start();
    SDL_Event event;
    while (true)
    {
        SDL_WaitEvent(&event);
//        if (!media1->ProcEvent(event)) {
            media2->ProcEvent(event);
//        }
//        if (media1->PlayFinish()) {
//            delete media1;
//            media1 = new MediaState("E:\\media\\video\\fanbingbing_1.mp4");
//            media1->Start();
//        }
        if (media2->PlayFinish()) {
            delete media2;
            media2 = new MediaState("E:\\media\\video\\062.3gp");
            media2->Start();
        }
    }

    return 0;
}
