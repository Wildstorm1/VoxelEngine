#pragma once

#include <optional>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

class Graphics {
    private:
        SDL_Window *window;
        VkInstance instance;
        VkSurfaceKHR surface;
        VkDevice logical_device;
        VkQueue graphics_queue;
        VkQueue presentation_queue;
        VkSwapchainKHR swap_chain;

    public:
        Graphics();

        int init(SDL_Window *window);
        int select_device();
        int setup_image();
        SDL_Window *release();
};