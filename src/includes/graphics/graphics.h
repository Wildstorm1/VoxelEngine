#pragma once

#include <optional>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

using namespace std;

class Graphics {
    private:
        optional<uint32_t> graphics_index;
        optional<uint32_t> presentation_index;
        VkPhysicalDevice device;
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
        vector<VkFramebuffer> swap_chain_framebuffers;
        VkCommandPool command_pool;
        VkCommandBuffer command_buffer;
        VkSemaphore image_available_semaphore;
        VkSemaphore render_finished_semaphore;
        VkFence in_flight_fence;
        uint32_t image_index;

    public:
        Graphics();

        int init(SDL_Window *window);
        int select_device();
        int setup_pipeline();
        int setup_draw_resources();
        void begin_render_pass();
        void draw();
        void end_render_pass();
        SDL_Window *release();
};