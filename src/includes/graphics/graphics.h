#pragma once

#include <optional>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

using namespace std;

class Graphics {
    private:
        SDL_Window *window;
        VkInstance instance;
        VkSurfaceKHR surface;
        VkDevice logical_device;
        VkQueue graphics_queue;
        VkQueue presentation_queue;
        VkSwapchainKHR swap_chain;
        vector<VkImage> swap_chain_images;
        vector<VkImageView> swap_chain_image_views;
        VkFormat swap_chain_image_format;
        VkExtent2D swap_chain_extent;
        VkRenderPass render_pass;
        VkPipelineLayout pipeline_layout;
        VkPipeline graphics_pipeline;

    public:
        Graphics();

        int init(SDL_Window *window);
        int select_device();
        int setup_pipeline();
        SDL_Window *release();
};