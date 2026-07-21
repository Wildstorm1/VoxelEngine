#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "./vulkan_render_pass.h"
#include "./vulkan_swapchain.h"
#include "./vulkan_device_private.h"
#include "./vulkan_mesh_heap.h"

typedef int VulkanChunkPipelineResult;

#define VULKAN_CHUNK_PIPELINE_INIT_SUCCESS 0
#define VULKAN_CHUNK_PIPELINE_INIT_FAILURE 1

using namespace std;

/*
 * Module wraps around vulkan resources which describe the pipeline
 * used for drawing voxel chunks
 */
class VulkanChunkPipeline {
    private:
        VkPipelineLayout pipeline_layout;
        VkPipeline graphics_pipeline;

    public:
        VulkanChunkPipeline();

        /*
         * Initializes a new instance of image related resources
         *
         * Should not be invoked more than once
         */
        VulkanChunkPipelineResult init(
            VulkanDevice *device,
            VulkanRenderPass *render_pass,
            VulkanSwapchain *swapchain,
            VulkanArrayMeshHeap *heap
        );

        /*
         * Return a handle to the graphics pipeline layout
         */
        VkPipelineLayout get_graphics_pipeline_layout();

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