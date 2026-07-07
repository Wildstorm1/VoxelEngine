#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

class Graphics {
    private:
        SDL_Window *window;
        VkInstance instance;
        VkDevice logical_device;
        VkQueue graphics_queue;

    public:
        Graphics();

        int init(SDL_Window *window);
        int select_device();
        SDL_Window *release();
};