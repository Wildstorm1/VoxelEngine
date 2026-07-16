#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "../voxels/mesh.h"
#include "./vulkan_image_private.h"
#include "./vulkan_device_private.h"
#include "./mesh_heap.h"

using namespace std;
using namespace glm;

/*
 * Module defines vulkan specific implementation for meshheap
 */
class VulkanArrayMeshHeap : public MeshHeap {
    private:
        vector<VkDeviceMemory> uniform_frame_buffers_memory;
        vector<VkBuffer> uniform_frame_buffers;
        vector<void*> uniform_frame_buffers_cpu_mapped;
        VkDeviceMemory vertex_buffer_memory;
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_staging_buffer_memory;
        VkBuffer vertex_staging_buffer;
        VkDeviceMemory index_buffer_memory;
        VkBuffer index_buffer;
        VkDeviceMemory index_staging_buffer_memory;
        VkBuffer index_staging_buffer;

    public:
        VulkanArrayMeshHeap(VulkanDevice *device);

        MeshHeapResult buffer_model_on_cpu(mat4 model, Mesh mesh);

        MeshHeapResult write_models_to_gpu();

        /*
         * Frees the image related resources
         *
         * Should only be called once
         */
        void destroy(VulkanDevice *device);
};