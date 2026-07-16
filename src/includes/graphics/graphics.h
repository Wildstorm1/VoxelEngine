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
#include "../voxels/mesh.h"

using namespace std;

struct WindowSize {
    uint32_t width;
    uint32_t height;
};

class Graphics {
    private:
        VulkanInstance instance;
        VulkanDevice device;
        VulkanImage image;

        SDL_Window *window;
        VkCommandPool command_pool;
        vector<Frame*> frame_buffer;
        uint32_t frame_index;

    public:
        Graphics();

        int init(SDL_Window *window);
        int resize_window();
        void begin_render_pass();
        void set_camera(Camera camera);
        void draw_mesh(Mesh *mesh);
        void end_render_pass();
        WindowSize get_window_size();
        SDL_Window *release();
};