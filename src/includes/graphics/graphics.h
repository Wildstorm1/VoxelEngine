#pragma once

#include <optional>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "./frame.h"
#include "./vulkan_frame.h"
#include "./vulkan_instance_private.h"
#include "./vulkan_image_private.h"
#include "./vulkan_device_private.h"

using namespace std;

class Graphics {
    private:
        VulkanInstance instance;
        VulkanDevice device;
        VulkanImage image;

        UniformBufferObject camera;
        SDL_Window *window;
        VkCommandPool command_pool;
        vector<Frame*> frame_buffer;
        uint32_t frame_index;

    public:
        Graphics();

        int init(SDL_Window *window);
        int resize_window();
        void begin_render_pass();
        void draw(float delta_time);
        void end_render_pass();
        SDL_Window *release();
};