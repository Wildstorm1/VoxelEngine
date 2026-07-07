#pragma once

#include <optional>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

using namespace std;

struct QueueFamilyIndicies {
    optional<uint32_t> graphics_index;
    optional<uint32_t> presentation_index;
};

class Graphics {
    private:
        SDL_Window *window;
        VkInstance instance;
        VkSurfaceKHR surface;
        VkDevice logical_device;
        VkQueue graphics_queue;
        VkQueue presentation_queue;

    public:
        Graphics();

        int init(SDL_Window *window);
        int select_device();
        SDL_Window *release();
};