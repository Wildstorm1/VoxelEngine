#pragma once

#include <vulkan/vulkan.h>
#include "./vulkan_device_private.h"

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