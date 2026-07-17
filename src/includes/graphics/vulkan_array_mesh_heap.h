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

struct GpuMapping {
    size_t offset;
    size_t size;
};

/*
 * Module defines vulkan specific implementation for meshheap
 */
class VulkanArrayMeshHeap : public MeshHeap {
    private:
        VulkanDevice *device;
        VulkanImage *image;
        VkCommandPool command_pool;
        VkCommandBuffer command_buffer;
        size_t frame_index;
        vector<VkDescriptorSet> descriptor_sets;
        vector<GpuMapping> model_vertex_metadata;
        vector<GpuMapping> model_index_metadata;
        vector<VkDeviceMemory> uniform_frame_buffers_memory;
        vector<VkBuffer> uniform_frame_buffers;
        vector<void*> uniform_frame_buffers_cpu_mapped;
        VkDeviceMemory vertex_buffer_memory;
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_staging_buffer_memory;
        VkBuffer vertex_staging_buffer;
        void* vertex_staging_buffer_cpu_mapped;
        VkDeviceMemory index_buffer_memory;
        VkBuffer index_buffer;
        VkDeviceMemory index_staging_buffer_memory;
        VkBuffer index_staging_buffer;
        void* index_staging_buffer_cpu_mapped;

    public:
        VulkanArrayMeshHeap(
            VulkanDevice *device,
            VulkanImage *image,
            VkCommandPool command_pool
        );

        /*
         * Initializes gpu buffers
         */
        MeshHeapResult init();

        MeshHeapResult begin_frame(size_t frame_index, VkCommandBuffer command_buffer);

        MeshHeapResult buffer_model_on_cpu(mat4 model, Mesh mesh);

        MeshHeapResult write_models_to_gpu();

        MeshHeapResult draw_mesh(size_t index);

        size_t get_mesh_count();

        /*
         * Frees the image related resources
         *
         * Should only be called once
         */
        void destroy();
};