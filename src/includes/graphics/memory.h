#pragma once

#include <vulkan/vulkan.h>
#include "./vulkan_device_private.h"

/*
 * Helper function to search through memory types
 */
uint32_t find_memory_type(
    VulkanDevice *device,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties
);

/*
 * Helper function which creates a new vulkan buffer
 */
VkResult create_buffer(
    VulkanDevice *device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer *buffer,
    VkDeviceMemory *buffer_memory
);