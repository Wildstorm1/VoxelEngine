#include <iostream>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "../includes/graphics/vulkan_mesh_heap.h"
#include "../includes/graphics/frame.h"
#include "../includes/graphics/memory.h"

using namespace glm;

VulkanArrayMeshHeap::VulkanArrayMeshHeap(
    VulkanDevice *device
) {
    this->device = device;
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

    VkResult index_buffer_result = create_buffer(
        device,
        sizeof(uint32_t) * 4097152,
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
        sizeof(uint32_t) * 4097152,
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

        vkMapMemory(device->get_device(), this->uniform_frame_buffers_memory[i], 0, sizeof(UniformBufferObject), 0, &this->uniform_frame_buffers_cpu_mapped[i]);
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

VkDeviceMemory VulkanArrayMeshHeap::get_vertex_buffer_device_memory() {
    return this->vertex_buffer_memory;
}

VkBuffer VulkanArrayMeshHeap::get_vertex_buffer() {
    return this->vertex_buffer;
}

VkDeviceMemory VulkanArrayMeshHeap::get_vertex_staging_buffer_device_memory() {
    return this->vertex_staging_buffer_memory;
}

VkBuffer VulkanArrayMeshHeap::get_vertex_staging_buffer() {
    return this->vertex_staging_buffer;
}

VkDeviceMemory VulkanArrayMeshHeap::get_index_buffer_device_memory() {
    return this->index_buffer_memory;
}

VkBuffer VulkanArrayMeshHeap::get_index_buffer() {
    return this->index_buffer;
}

VkDeviceMemory VulkanArrayMeshHeap::get_index_staging_buffer_device_memory() {
    return this->index_staging_buffer_memory;
}

VkBuffer VulkanArrayMeshHeap::get_index_staging_buffer() {
    return this->index_staging_buffer;
}

VkDescriptorSet* VulkanArrayMeshHeap::get_frame_descriptor_set(size_t i) {
    return &this->descriptor_sets[i];
}

void* VulkanArrayMeshHeap::get_uniform_frame_buffer_cpu_mapped(size_t i) {
    return this->uniform_frame_buffers_cpu_mapped[i];
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