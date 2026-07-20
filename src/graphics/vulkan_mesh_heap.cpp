#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <vector>
#include <array>
#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>
#include "../includes/graphics/structs.h"
#include "../includes/graphics/vulkan_mesh_heap.h"
#include "../includes/graphics/memory.h"
#include "../includes/voxels/mesh.h"

using namespace glm;

VulkanArrayMeshHeap::VulkanArrayMeshHeap(
    VulkanDevice *device,
    VkCommandPool command_pool
) {
    this->device = device;
    this->command_pool = command_pool;
}

VkResult create_descriptor_set_layout(
    VulkanDevice *device,
    VkDescriptorSetLayout *descriptor_set_layout
) {
    array<VkDescriptorBindingFlags, 1> flags {};
    flags.at(0) = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags {};
    bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlags.pNext = nullptr;
    bindingFlags.pBindingFlags = flags.data();
    bindingFlags.bindingCount = 1;

    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.pBindings = &uboLayoutBinding;
    layoutInfo.pNext = &bindingFlags;

    return vkCreateDescriptorSetLayout(device->get_device(), &layoutInfo, nullptr, descriptor_set_layout);
}

VkResult create_descriptor_pool(
    VulkanDevice *device,
    VkDescriptorPool *descriptor_pool
) {
    // Allocate 1 per frame, but the size is ambiguous (magic 2's)
    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSize.descriptorCount = 1 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    return vkCreateDescriptorPool(device->get_device(), &poolInfo, nullptr, descriptor_pool);
}

MeshHeapResult VulkanArrayMeshHeap::init() {
    VkResult vertex_buffer_result = create_buffer(
        device,
        sizeof(Vertex) * 25000000,
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
        sizeof(Vertex) * 25000000,
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

    vkMapMemory(device->get_device(), this->vertex_staging_buffer_memory, 0, sizeof(Vertex) * 25000000, 0, &this->vertex_staging_buffer_cpu_mapped);

    VkResult index_buffer_result = create_buffer(
        device,
        sizeof(uint32_t) * 25000000,
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
        sizeof(uint32_t) * 25000000,
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

    vkMapMemory(device->get_device(), this->index_staging_buffer_memory, 0, sizeof(uint32_t) * 25000000, 0, &this->index_staging_buffer_cpu_mapped);

    VkResult descriptorSetLayoutResult = create_descriptor_set_layout(device, &descriptor_set_layout);

    if (descriptorSetLayoutResult == VK_SUCCESS) {
        cout << "Successfully created vulkan descriptor set layout" << endl;
    } else {
        cout << "Could not create descriptor set layout" << endl;
        return HEAP_FAILURE;
    }

    VkResult descriptorSetPoolResult = create_descriptor_pool(device, &descriptor_pool);

    if (descriptorSetPoolResult == VK_SUCCESS) {
        cout << "Successfully created vulkan descriptor pool" << endl;
    } else {
        cout << "Could not create descriptor pool" << endl;
        return HEAP_FAILURE;
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
            return HEAP_FAILURE;
        }

        vkMapMemory(device->get_device(), this->uniform_frame_buffers_memory[i], 0, 1000 * sizeof(UniformBufferObject), 0, &this->uniform_frame_buffers_cpu_mapped[i]);
    }

    vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, this->descriptor_set_layout);
    VkDescriptorSetAllocateInfo dsetAllocInfo {};
    dsetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsetAllocInfo.descriptorPool = this->descriptor_pool;
    dsetAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    dsetAllocInfo.pSetLayouts = layouts.data();

    this->descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);

    VkResult allocateDescriptorSet = vkAllocateDescriptorSets(device->get_device(), &dsetAllocInfo, this->descriptor_sets.data());

    if (allocateDescriptorSet == VK_SUCCESS) {
        cout << "Successfully created vulkan descriptor set" << endl;
    } else {
        cout << "Could not create descriptor set" << endl;
        return HEAP_FAILURE;
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
    UniformBufferObject *model,
    Mesh mesh
) {
    vector<Vertex> *vertex_vector = mesh.get_vertex_vector();
    vector<uint32_t> *index_vector = mesh.get_index_vector();

    size_t index = this->model_index_metadata.size();

    GpuMapping vertex_mapping = {};
    vertex_mapping.offset = 0;
    vertex_mapping.size = vertex_vector->size();
    GpuMapping index_mapping = {};
    index_mapping.offset = 0;
    index_mapping.size = index_vector->size();

    if (index != 0) {
        GpuMapping previous_vertex_mapping = this->model_vertex_metadata[index - 1];
        vertex_mapping.offset = previous_vertex_mapping.offset + previous_vertex_mapping.size;
        GpuMapping previous_index_mapping = this->model_index_metadata[index - 1];
        index_mapping.offset = previous_index_mapping.offset + previous_index_mapping.size;
    }

    this->model_vertex_metadata.push_back(vertex_mapping);
    this->model_index_metadata.push_back(index_mapping);

    memcpy(
        (char*) this->vertex_staging_buffer_cpu_mapped + (vertex_mapping.offset * (size_t) sizeof(Vertex)),
        vertex_vector->data(),
        vertex_mapping.size * (size_t) sizeof(Vertex)
    );
    memcpy(
        (char*) this->index_staging_buffer_cpu_mapped + (index_mapping.offset * (size_t) sizeof(uint32_t)),
        index_vector->data(),
        index_mapping.size * (size_t) sizeof(uint32_t)
    );

    UniformBufferObject* buffer = (UniformBufferObject*) this->uniform_frame_buffers_cpu_mapped[this->frame_index];
    *(buffer + index) = *model;

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
        (size_t) sizeof(Vertex) * 25000000,
        this->vertex_staging_buffer,
        this->vertex_buffer
    );

    copy_cpu_staging_buffer_to_gpu(
        this->device,
        this->command_pool,
        (size_t) sizeof(uint32_t) * 25000000,
        this->index_staging_buffer,
        this->index_buffer
    );

    return HEAP_SUCCESS;
}

MeshHeapResult VulkanArrayMeshHeap::draw_mesh(
    size_t index,
    VkPipelineLayout pipeline_layout
) {
    // Draw what is in the gpu buffer
    VkBuffer vertexBuffers[] = { this->vertex_buffer };
    GpuMapping vertex_mapping = this->model_vertex_metadata[index];
    GpuMapping index_mapping = this->model_index_metadata[index];

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(this->command_buffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(this->command_buffer, this->index_buffer, 0, VK_INDEX_TYPE_UINT32);

    uint32_t dynamicOffset[] = { (uint32_t)(index * sizeof(UniformBufferObject)) };
    VkDescriptorSet descriptor_set[] = { this->descriptor_sets[this->frame_index] };
    vkCmdBindDescriptorSets(
        this->command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout,
        0,
        1,
        descriptor_set,
        1,
        dynamicOffset
    );

    vkCmdDrawIndexed(
        this->command_buffer,
        (uint32_t) index_mapping.size,
        1,
        (uint32_t) index_mapping.offset,
        (uint32_t) vertex_mapping.offset,
        0
    );

    return HEAP_SUCCESS;
}

size_t VulkanArrayMeshHeap::get_mesh_count() {
    return this->model_index_metadata.size();
}

VkDescriptorPool *VulkanArrayMeshHeap::get_descriptor_pool() {
    return &this->descriptor_pool;
}

VkDescriptorSetLayout *VulkanArrayMeshHeap::get_descriptor_set_layout() {
    return &this->descriptor_set_layout;
}

void VulkanArrayMeshHeap::destroy() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device->get_device(), this->uniform_frame_buffers[i], nullptr);
        vkFreeMemory(device->get_device(), this->uniform_frame_buffers_memory[i], nullptr);
    }

    vkDestroyDescriptorPool(device->get_device(), this->descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device->get_device(), this->descriptor_set_layout, nullptr);

    vkDestroyBuffer(device->get_device(), this->index_staging_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->index_staging_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->index_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->index_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->vertex_staging_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->vertex_staging_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->vertex_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->vertex_buffer_memory, nullptr);
}