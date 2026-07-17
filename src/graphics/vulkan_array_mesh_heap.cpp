#include <iostream>
#include <glm/glm.hpp>
#include "../includes/voxels/mesh.h"
#include "../includes/graphics/frame.h"
#include "../includes/graphics/memory.h"
#include "../includes/graphics/vulkan_device_private.h"
#include "../includes/graphics/vulkan_image_private.h"
#include "../includes/graphics/vulkan_array_mesh_heap.h"

using namespace glm;

VulkanArrayMeshHeap::VulkanArrayMeshHeap(
    VulkanDevice *device,
    VulkanImage *image,
    VkCommandPool command_pool
) {
    this->device = device;
    this->image = image;
    this->command_pool = command_pool;
    this->model_index_metadata = {};
    this->model_vertex_metadata = {};
}

MeshHeapResult VulkanArrayMeshHeap::init() {
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
        return HEAP_FAILURE;
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
        return HEAP_FAILURE;
    }

    vkMapMemory(device->get_device(), this->vertex_staging_buffer_memory, 0, sizeof(Vertex) * 4097152, 0, &this->vertex_staging_buffer_cpu_mapped);

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
        return HEAP_FAILURE;
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
        return HEAP_FAILURE;
    }

    vkMapMemory(device->get_device(), this->index_staging_buffer_memory, 0, sizeof(uint16_t) * 4097152, 0, &this->index_staging_buffer_cpu_mapped);

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
            return HEAP_FAILURE;
        }

        // TODO: Come back to this, multiply by 1000?
        vkMapMemory(device->get_device(), this->uniform_frame_buffers_memory[i], 0, sizeof(UniformBufferObject), 0, &this->uniform_frame_buffers_cpu_mapped[i]);
    }

    vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, this->image->get_descriptor_set_layout());
    VkDescriptorSetAllocateInfo dsetAllocInfo {};
    dsetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsetAllocInfo.descriptorPool = this->image->get_descriptor_pool();
    dsetAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    dsetAllocInfo.pSetLayouts = layouts.data();

    this->descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);

    VkResult allocateDescriptorSet = vkAllocateDescriptorSets(device->get_device(), &dsetAllocInfo, this->descriptor_sets.data());

    if (allocateDescriptorSet == VK_SUCCESS) {
        cout << "Successfully created vulkan descriptor set" << endl;
    } else {
        cout << "Could not create descriptor set" << endl;
        return FRAME_FAILURE;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = this->uniform_frame_buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = this->descriptor_sets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        vkUpdateDescriptorSets(device->get_device(), 1, &descriptorWrite, 0, nullptr);
    }

    return HEAP_SUCCESS;
}

MeshHeapResult VulkanArrayMeshHeap::begin_frame(
    size_t frame_index,
    VkCommandBuffer command_buffer
) {
    this->model_index_metadata.clear();
    this->model_vertex_metadata.clear();
    this->frame_index = frame_index;
    this->command_buffer = command_buffer;
    return HEAP_SUCCESS;
}

MeshHeapResult VulkanArrayMeshHeap::buffer_model_on_cpu(
    mat4 model,
    Mesh mesh
) {
    vector<Vertex> *vertex_vector = mesh.get_vertex_vector();
    vector<uint32_t> *index_vector = mesh.get_index_vector();

    size_t index = this->model_index_metadata.size();

    GpuMapping vertex_mapping = {};
    vertex_mapping.offset = 0;
    vertex_mapping.size = vertex_vector->size() * (size_t) sizeof(Vertex);
    GpuMapping index_mapping = {};
    index_mapping.offset = 0;
    index_mapping.size = index_vector->size() * (size_t) sizeof(uint32_t);

    if (index != 0) {
        GpuMapping previous_vertex_mapping = this->model_vertex_metadata[index - 1];
        vertex_mapping.offset = previous_vertex_mapping.offset + previous_vertex_mapping.size;
        GpuMapping previous_index_mapping = this->model_index_metadata[index - 1];
        index_mapping.offset = previous_index_mapping.offset + previous_index_mapping.size;
    }

    this->model_vertex_metadata.push_back(vertex_mapping);
    this->model_index_metadata.push_back(index_mapping);

    memcpy((char*) this->vertex_staging_buffer_cpu_mapped + vertex_mapping.offset, vertex_vector->data(), vertex_mapping.size);
    memcpy((char*) this->index_staging_buffer_cpu_mapped + index_mapping.offset, index_vector->data(), index_mapping.size);

    UniformBufferObject model_buffer;
    model_buffer.model = model;

    UniformBufferObject* buffer = (UniformBufferObject*) this->uniform_frame_buffers_cpu_mapped[this->frame_index];
    memcpy((void*)(buffer + index), &model, sizeof(UniformBufferObject));
    return HEAP_SUCCESS;
}

void copy_cpu_staging_buffer_to_gpu(
    VulkanDevice *device,
    VkCommandPool command_pool,
    size_t copy_bytes,
    VkBuffer staging_buffer,
    VkBuffer buffer
) {
    // Transfer from staging buffer to gpu buffer
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device->get_device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = copy_bytes;
    vkCmdCopyBuffer(commandBuffer, staging_buffer, buffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    // Submit the copy for processing
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device->get_graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->get_graphics_queue());

    // Free the command buffer
    vkFreeCommandBuffers(device->get_device(), command_pool, 1, &commandBuffer);
}
 
MeshHeapResult VulkanArrayMeshHeap::write_models_to_gpu() {
    copy_cpu_staging_buffer_to_gpu(
        this->device,
        this->command_pool,
        (size_t) sizeof(Vertex) * 4097152,
        this->vertex_staging_buffer,
        this->vertex_buffer
    );

    copy_cpu_staging_buffer_to_gpu(
        this->device,
        this->command_pool,
        (size_t) sizeof(uint16_t) * 4097152, // Didn't I switch to u32?
        this->index_staging_buffer,
        this->index_buffer
    );

    return HEAP_SUCCESS;
}

MeshHeapResult VulkanArrayMeshHeap::draw_mesh(
    size_t index
) {
    // Draw what is in the gpu buffer
    VkBuffer vertexBuffers[] = { this->vertex_buffer };
    GpuMapping vertex_mapping = this->model_vertex_metadata[index];
    GpuMapping index_mapping = this->model_index_metadata[index];

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(this->command_buffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(this->command_buffer, this->index_buffer, 0, VK_INDEX_TYPE_UINT32);

    uint32_t dynamicOffset = (uint32_t)(index * sizeof(UniformBufferObject));
    VkDescriptorSet descriptor_set[] = { this->descriptor_sets[this->frame_index] };
    vkCmdBindDescriptorSets(
        this->command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        this->image->get_graphics_pipeline_layout(),
        0,
        1,
        descriptor_set,
        1,
        &dynamicOffset
    );

    // TODO: Is this right? Offsets?
    vkCmdDrawIndexed(this->command_buffer, (uint32_t) index_mapping.size, 1, (uint32_t) index_mapping.offset, (uint32_t) vertex_mapping.offset, 0);
    return HEAP_SUCCESS;
}

size_t VulkanArrayMeshHeap::get_mesh_count() {
    return this->model_index_metadata.size();
}

void VulkanArrayMeshHeap::destroy() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device->get_device(), this->uniform_frame_buffers[i], nullptr);
        vkFreeMemory(device->get_device(), this->uniform_frame_buffers_memory[i], nullptr);
    }

    vkDestroyBuffer(device->get_device(), this->index_staging_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->index_staging_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->index_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->index_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->vertex_staging_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->vertex_staging_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->vertex_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->vertex_buffer_memory, nullptr);
}