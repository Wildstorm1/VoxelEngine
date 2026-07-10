#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "./vulkan_device_private.h"

typedef int VulkanImageResult;

#define VULKAN_IMAGE_INIT_SUCCESS 0
#define VULKAN_IMAGE_INIT_FAILURE 1

using namespace std;

/*
 * Module wraps around vulkan resources which describe an image or
 * are used to draw an image
 */
class VulkanImage {
    private:
        VkSwapchainKHR swap_chain;
        vector<VkFramebuffer> swap_chain_framebuffers;
        vector<VkSemaphore> render_framebuffer_semaphores;
        vector<VkImage> swap_chain_images;
        vector<VkImageView> swap_chain_image_views;
        VkFormat swap_chain_image_format;
        VkExtent2D swap_chain_extent;
        VkRenderPass render_pass;
        VkPipelineLayout pipeline_layout;
        VkPipeline graphics_pipeline;

    public:
        VulkanImage();

        /*
         * Initializes a new instance of image related resources
         *
         * Should not be invoked more than once
         */
        VulkanImageResult init(SDL_Window *window, VulkanDevice *device);

        /*
         * Return a handle to the swapchain, should only be called after
         * init and before destroy
         */
        VkSwapchainKHR get_swapchain();

        /*
         * Return a handle to the swapchain extent
         */
        VkExtent2D get_swapchain_extent();

        /*
         * Return a handle to the render pass
         */
        VkRenderPass get_render_pass();

        /*
         * Return a handle to the i'th framebuffer semaphore
         */
        VkSemaphore get_framebuffer_semaphore(size_t i);

        /*
         * Return a handle to the i'th framebuffer
         */
        VkFramebuffer get_framebuffer(size_t i);

        /*
         * Return a handle to the graphics pipeline
         */
        VkPipeline get_graphics_pipeline();

        /*
         * Frees the image related resources
         *
         * Should only be called once
         */
        void destroy(VulkanDevice *device);
};