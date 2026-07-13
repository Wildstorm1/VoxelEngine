#define SDL_MAIN_USE_CALLBACKS
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <iostream>
#include <chrono>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "./includes/graphics/frame.h"
#include "./includes/graphics/graphics.h"

using namespace std;

UniformBufferObject camera;
float x = 400;
float y = 300;

float yaw = 0;
float pitch = 0;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

constexpr uint32_t windowStartWidth = 800;
constexpr uint32_t windowStartHeight = 600;
chrono::steady_clock::time_point start_time;
float delta_time = 0;
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

    SDL_SetWindowRelativeMouseMode(window, true);
    SDL_CaptureMouse(true);
    SDL_WarpMouseInWindow(window, windowStartWidth / 2, windowStartHeight / 2);

    graphics_memory = malloc(sizeof(Graphics));

    if (graphics_memory == nullptr){
        return SDL_Fail();
    }

    graphics = new (graphics_memory) Graphics();
    int initResult = graphics->init(window);

    if (initResult == EXIT_FAILURE) {
        return SDL_Fail();
    }

    start_time = chrono::high_resolution_clock::now();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; // Safely exits the app
    }

    if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        graphics->resize_window();
    }

    if (event->type == SDL_EVENT_KEY_UP) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS; // Safely exits the app
        }
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_W) {
            cameraPos += 0.05f * cameraFront;
        } else if (event->key.key == SDLK_S) {
            cameraPos -= 0.05f * cameraFront;
        } else if (event->key.key == SDLK_A) {
            cameraPos -= 0.05f * glm::normalize(glm::cross(cameraFront, cameraUp));
        } else if (event->key.key == SDLK_D) {
            cameraPos += 0.05f * glm::normalize(glm::cross(cameraFront, cameraUp));
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    // Your game loop / rendering code here
    chrono::steady_clock::time_point now_time = chrono::high_resolution_clock::now();
    delta_time = chrono::duration<float, chrono::seconds::period>(now_time - start_time).count();
    start_time = now_time;

    float mx, my;
    SDL_GetMouseState(&mx, &my);

    const float sensitivity = 0.01f;
    float dx, dy;
    dx = (mx - x) * sensitivity;
    dy = (my - y) * sensitivity;

    yaw = yaw + dx;
    pitch = pitch - dy;

    if (pitch > glm::half_pi<float>() - 0.017) {
        pitch = glm::half_pi<float>() - 0.017;
    }
 
    if (pitch < -1 * glm::half_pi<float>() + 0.017) {
        pitch = -1 * glm::half_pi<float>() + 0.017;
    }

    glm::vec3 direction;
    direction.x = cos(yaw) * cos(pitch);
    direction.y = sin(pitch);
    direction.z = sin(yaw) * cos(pitch);
    cameraFront = glm::normalize(direction);

    SDL_WarpMouseInWindow(window, windowStartWidth / 2, windowStartHeight / 2);

    WindowSize size = graphics->get_window_size();

    camera.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //camera.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    camera.view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    camera.proj = glm::perspective(glm::radians(45.0f), size.width / (float) size.height, 0.1f, 10.0f);
    camera.proj[1][1] *= -1;

    graphics->set_camera(camera);
    graphics->begin_render_pass();
    graphics->draw(delta_time);
    graphics->end_render_pass();

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    // Your cleanup code here
    SDL_Window* window = graphics->release();
    free(graphics_memory);
    SDL_DestroyWindow(window);
}