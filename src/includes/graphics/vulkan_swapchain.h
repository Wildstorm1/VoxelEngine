#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "./vulkan_device_private.h"

typedef int VulkanSwapchainResult;

#define VULKAN_SWAPCHAIN_INIT_SUCCESS 0
#define VULKAN_SWAPCHAIN_INIT_FAILURE 1

using namespace std;

/*
 * Module wraps around vulkan resources which describe the creation
 * of a swapchain
 */
class VulkanSwapchain {
    private:
        VkSwapchainKHR swap_chain;
        vector<VkImage> swap_chain_images;
        vector<VkImageView> swap_chain_image_views;
        VkFormat swap_chain_image_format;
        VkExtent2D swap_chain_extent;

    public:
        VulkanSwapchain();

        /*
         * Initializes a new instance of image related resources
         *
         * Should not be invoked more than once
         */
        VulkanSwapchainResult init(SDL_Window *window, VulkanDevice *device);

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
         * Return a handle to the swapchain image format
         */
        VkFormat get_swap_chain_image_format();

        /*
         * Return a handle to the swapchain image views
         */
        vector<VkImageView> get_swap_chain_image_views();

        /*
         * Frees the swapchain related resources
         *
         * Should only be called once
         */
        void destroy(VulkanDevice *device);
};