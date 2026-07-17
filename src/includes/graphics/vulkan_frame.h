#pragma once

#include <vulkan/vulkan.h>
#include "../voxels/mesh.h"
#include "./vulkan_image_private.h"
#include "./vulkan_device_private.h"
#include "./vulkan_mesh_heap.h"
#include "./frame.h"

typedef int FrameResult;

#define FRAME_INIT_SUCCESS 0
#define FRAME_INIT_FAILURE 1

using namespace std;

/*
 * Module defines vulkan specific implementation for
 */
class VulkanFrame : public Frame {
    private:
        VulkanDevice *device;
        VulkanImage *image;
        VulkanArrayMeshHeap *mesh_heap;
        VkCommandPool command_pool;
        VkCommandBuffer command_buffer;
        VkSemaphore image_available_semaphore;
        VkFence in_flight_fence;
        uint32_t image_index;
        size_t frame_index;

    public:
        VulkanFrame(
            VulkanDevice *device,
            VulkanImage *image,
            VkCommandPool command_pool,
            size_t frame_index,
            VulkanArrayMeshHeap *mesh_heap
        );

        FrameResult init();

        FrameResult begin_frame();

        FrameResult set_camera(Camera camera);

        FrameResult draw_mesh(Mesh *mesh);

        FrameResult end_frame();

        void destroy();
};