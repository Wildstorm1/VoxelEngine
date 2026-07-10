#pragma once

#include <optional>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "./vulkan_instance_private.h"
#include "./vulkan_image_private.h"
#include "./vulkan_device_private.h"

using namespace std;

class Graphics {
    private:
        VulkanInstance instance;
        VulkanDevice device;
        VulkanImage image;

        SDL_Window *window;
        VkCommandPool command_pool;
        vector<VkCommandBuffer> command_buffers;
        vector<VkSemaphore> image_available_semaphores;
        vector<VkSemaphore> render_finished_semaphores;
        vector<VkFence> in_flight_fences;
        uint32_t image_index;
        uint32_t frame_index;

    public:
        Graphics();

        int init(SDL_Window *window);
        int setup_draw_resources();
        int resize_window();
        void begin_render_pass();
        void draw();
        void end_render_pass();
        SDL_Window *release();
};