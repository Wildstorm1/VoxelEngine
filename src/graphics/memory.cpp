#include <iostream>
#include <vulkan/vulkan.h>
#include "../includes/graphics/memory.h"
#include "../includes/graphics/vulkan_device_private.h"

uint32_t find_memory_type(
    VulkanDevice *device,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties
) {
    DeviceInfos* device_info = device->get_device_info();
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device_info->device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VkResult create_buffer(
    VulkanDevice *device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer *buffer,
    VkDeviceMemory *buffer_memory
) {
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult buffer_result = vkCreateBuffer(device->get_device(), &bufferInfo, nullptr, buffer);

    if (buffer_result == VK_SUCCESS) {
        cout << "Successfully created vulkan vertex buffer" << endl;
    } else {
        cout << "Could not create vertex buffer" << endl;
        return buffer_result;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device->get_device(), *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(device, memRequirements.memoryTypeBits, properties);

    VkResult buffer_memory_result = vkAllocateMemory(device->get_device(), &allocInfo, nullptr, buffer_memory);

    if (buffer_memory_result == VK_SUCCESS) {
        cout << "Successfully created vulkan buffer memory block" << endl;
    } else {
        cout << "Could not allocate device memory block" << endl;
        return buffer_result;
    }

    vkBindBufferMemory(device->get_device(), *buffer, *buffer_memory, 0);
    return VK_SUCCESS;
}