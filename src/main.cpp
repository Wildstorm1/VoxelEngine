#define SDL_MAIN_USE_CALLBACKS

#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>

#include "./includes/graphics/graphics.h"

using namespace std;

constexpr uint32_t windowStartWidth = 800;
constexpr uint32_t windowStartHeight = 400;
void *graphics_memory = nullptr;
Graphics *graphics = nullptr;
SDL_Window* window = nullptr;

SDL_AppResult SDL_Fail() {
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    // Your initialization code here (e.g., SDL_Init, creating window)
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        return SDL_Fail();
    }

    window = SDL_CreateWindow("SDL Minimal Sample", windowStartWidth, windowStartHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN);

    if (window == nullptr) {
        return SDL_Fail();
    }

    graphics_memory = malloc(sizeof(Graphics));

    if (graphics_memory == nullptr){
        return SDL_Fail();
    }

    graphics = new (graphics_memory) Graphics();
    int initResult = graphics->init(window);

    if (initResult == EXIT_FAILURE) {
        return SDL_Fail();
    }

    graphics->setup_draw_resources();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; // Safely exits the app
    }

    if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        graphics->resize_window();
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    // Your game loop / rendering code here
    graphics->begin_render_pass();
    graphics->draw();
    graphics->end_render_pass();

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    // Your cleanup code here
    SDL_Window* window = graphics->release();
    free(graphics_memory);
    SDL_DestroyWindow(window);
}