#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include "./vulkan_device_private.h"

typedef int VulkanImageResult;

#define VULKAN_IMAGE_INIT_SUCCESS 0
#define VULKAN_IMAGE_INIT_FAILURE 1

using namespace std;
using namespace glm;

struct Vertex {
    vec2 pos;
    vec3 color;
};

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
        VkDeviceMemory vertex_buffer_memory;
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_staging_buffer_memory;
        VkBuffer vertex_staging_buffer;
        VkDeviceMemory index_buffer_memory;
        VkBuffer index_buffer;
        VkDeviceMemory index_staging_buffer_memory;
        VkBuffer index_staging_buffer;

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
         * Return a handle to the vertex buffers device memory
         */
        VkDeviceMemory get_vertex_buffer_device_memory();

        /*
         * Return a handle to the vertex buffer
         */
        VkBuffer get_vertex_buffer();

        /*
         * Return a handle to the staging buffers device memory
         */
        VkDeviceMemory get_vertex_staging_buffer_device_memory();

        /*
         * Return a handle to the staging buffer
         */
        VkBuffer get_vertex_staging_buffer();

        /*
         * Return a handle to the index buffers device memory
         */
        VkDeviceMemory get_index_buffer_device_memory();

        /*
         * Return a handle to the index buffer
         */
        VkBuffer get_index_buffer();

        /*
         * Return a handle to the index buffers device memory
         */
        VkDeviceMemory get_index_staging_buffer_device_memory();

        /*
         * Return a handle to the index buffer
         */
        VkBuffer get_index_staging_buffer();

        /*
         * Frees the image related resources
         *
         * Should only be called once
         */
        void destroy(VulkanDevice *device);
};