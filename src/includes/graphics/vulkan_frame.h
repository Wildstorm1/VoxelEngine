#pragma once

#include <vulkan/vulkan.h>
#include "../voxels/mesh.h"
#include "./structs.h"
#include "./vulkan_swapchain.h"
#include "./vulkan_render_pass.h"
#include "./vulkan_device_private.h"
#include "./vulkan_chunk_pipeline.h"

typedef int FrameResult;

#define FRAME_SUCCESS 0
#define FRAME_INIT_FAILURE 1

using namespace std;

/*
 * Module defines vulkan specific implementation for a frame
 */
class VulkanFrame {
    private:
        VulkanDevice *device;
        VulkanSwapchain *swapchain;
        VulkanRenderPass *render_pass;
        VulkanChunkPipeline *pipeline;
        VkCommandPool command_pool;
        VkCommandBuffer command_buffer;
        VkSemaphore image_available_semaphore;
        VkFence in_flight_fence;
        uint32_t image_index;
        size_t frame_index;

    public:
        VulkanFrame(
            VulkanDevice *device,
            VulkanSwapchain *swapchain,
            VulkanRenderPass *render_pass,
            VulkanChunkPipeline *pipeline,
            VkCommandPool command_pool,
            size_t frame_index
        );

        FrameResult init();

        FrameResult begin_frame();

        FrameResult set_camera(Camera camera);

        FrameResult draw_mesh(Mesh *mesh);

        FrameResult end_frame();

        VkCommandBuffer get_command_buffer();

        void destroy();
};