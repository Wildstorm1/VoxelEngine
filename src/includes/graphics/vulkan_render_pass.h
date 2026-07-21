#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "./vulkan_swapchain.h"
#include "./vulkan_device_private.h"

typedef int VulkanRenderPassResult;

#define VULKAN_IMAGE_INIT_SUCCESS 0
#define VULKAN_IMAGE_INIT_FAILURE 1

using namespace std;

/*
 * Module wraps around vulkan resources for a render pass
 */
class VulkanRenderPass {
    private:
        vector<VkFramebuffer> swap_chain_framebuffers;
        vector<VkSemaphore> render_framebuffer_semaphores;
        VkRenderPass render_pass;
        VkImage depth_image;
        VkDeviceMemory depth_image_memory;
        VkImageView depth_image_view;

    public:
        VulkanRenderPass();

        /*
         * Initializes a new instance of image related resources
         *
         * Should not be invoked more than once
         */
        VulkanRenderPassResult init(
            SDL_Window *window,
            VulkanDevice *device,
            VulkanSwapchain *swapchain
        );

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
         * Frees the image related resources
         *
         * Should only be called once
         */
        void destroy(VulkanDevice *device);
};