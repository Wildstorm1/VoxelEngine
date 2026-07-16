#include <iostream>
#include <glm/glm.hpp>
#include "../includes/voxels/mesh.h"
#include "../includes/graphics/memory.h"
#include "../includes/graphics/vulkan_device_private.h"
#include "../includes/graphics/vulkan_mesh_heap.h"

using namespace glm;

VulkanArrayMeshHeap::VulkanArrayMeshHeap(VulkanDevice *device) {
    VkResult vertex_buffer_result = create_buffer(
        device,
        sizeof(Vertex) * 4097152,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &this->vertex_buffer,
        &this->vertex_buffer_memory
    );

    if (vertex_buffer_result == VK_SUCCESS) {
        cout << "Successfully created vulkan vertex buffer" << endl;
    } else {
        cout << "Could not create vertex buffer" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkResult staging_buffer_result = create_buffer(
        device,
        sizeof(Vertex) * 4097152,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &this->vertex_staging_buffer,
        &this->vertex_staging_buffer_memory
    );

    if (staging_buffer_result == VK_SUCCESS) {
        cout << "Successfully created vulkan staging buffer" << endl;
    } else {
        cout << "Could not create staging buffer" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkResult index_buffer_result = create_buffer(
        device,
        sizeof(uint16_t) * 4097152,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &this->index_buffer,
        &this->index_buffer_memory
    );

    if (index_buffer_result == VK_SUCCESS) {
        cout << "Successfully created vulkan index buffer" << endl;
    } else {
        cout << "Could not create index buffer" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkResult index_staging_buffer_result = create_buffer(
        device,
        sizeof(uint16_t) * 4097152,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &this->index_staging_buffer,
        &this->index_staging_buffer_memory
    );

    if (index_staging_buffer_result == VK_SUCCESS) {
        cout << "Successfully created vulkan index staging buffer" << endl;
    } else {
        cout << "Could not create index staging buffer" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    this->uniform_frame_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    this->uniform_frame_buffers_cpu_mapped.resize(MAX_FRAMES_IN_FLIGHT);
    this->uniform_frame_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult ubo_buffer_result = create_buffer(
            device,
            1000 * sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &this->uniform_frame_buffers[i],
            &this->uniform_frame_buffers_memory[i]
        );

        if (ubo_buffer_result == VK_SUCCESS) {
            cout << "Successfully created vulkan ubo buffer" << endl;
        } else {
            cout << "Could not create ubo buffer" << endl;
            return FRAME_FAILURE;
        }

        vkMapMemory(device->get_device(), this->uniform_frame_buffers_memory[i], 0, sizeof(UniformBufferObject), 0, &this->uniform_frame_buffers_cpu_mapped[i]);
    }
}

MeshHeapResult VulkanArrayMeshHeap::buffer_model_on_cpu(mat4 model, Mesh mesh) {
}

void VulkanArrayMeshHeap::destroy(VulkanDevice *device) {
}