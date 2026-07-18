#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "../voxels/mesh.h"
#include "./structs.h"
#include "./vulkan_device_private.h"

typedef int MeshHeapResult;

#define HEAP_SUCCESS 0
#define HEAP_FAILURE 1

using namespace std;
using namespace glm;

struct GpuMapping {
    size_t offset;
    size_t size;
};

/*
 * Module defines vulkan specific implementation for meshheap
 */
class VulkanArrayMeshHeap {
    private:
        VulkanDevice *device;
        VkCommandPool command_pool;
        VkCommandBuffer command_buffer;
        size_t frame_index;
        vector<GpuMapping> model_vertex_metadata;
        vector<GpuMapping> model_index_metadata;
        vector<VkDeviceMemory> uniform_frame_buffers_memory;
        vector<VkBuffer> uniform_frame_buffers;
        vector<void*> uniform_frame_buffers_cpu_mapped;
        vector<VkDescriptorSet> descriptor_sets;
        VkDescriptorPool descriptor_pool;
        VkDescriptorSetLayout descriptor_set_layout;
        VkDeviceMemory vertex_buffer_memory;
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_staging_buffer_memory;
        void* vertex_staging_buffer_cpu_mapped;
        VkBuffer vertex_staging_buffer;
        VkDeviceMemory index_buffer_memory;
        VkBuffer index_buffer;
        VkDeviceMemory index_staging_buffer_memory;
        VkBuffer index_staging_buffer;
        void* index_staging_buffer_cpu_mapped;

    public:
        VulkanArrayMeshHeap(
            VulkanDevice *device,
            VkCommandPool command_pool
        );

        /*
         * Initializes gpu buffers
         */
        MeshHeapResult init();

        MeshHeapResult begin_frame(size_t frame_index, VkCommandBuffer command_buffer);

        MeshHeapResult buffer_model_on_cpu(UniformBufferObject *model, Mesh mesh);

        MeshHeapResult write_models_to_gpu();

        MeshHeapResult draw_mesh(size_t index, VkPipelineLayout pipeline_layout);

        size_t get_mesh_count();

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
         * Return a handle to the ith frames descriptor set
         */
        VkDescriptorSet* get_frame_descriptor_set(size_t i);

        /*
         * Return a handle to the ith frames uniform buffer
         */
        void* get_uniform_frame_buffer_cpu_mapped(size_t i);

        /*
         * Return a handle to the descriptor pool
         */
        VkDescriptorPool* get_descriptor_pool();

        /*
         * Return a handle to the descriptor set layout
         */
        VkDescriptorSetLayout* get_descriptor_set_layout();

        /*
         * Frees the image related resources
         *
         * Should only be called once
         */
        void destroy();
};