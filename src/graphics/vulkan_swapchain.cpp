#include <iostream>
#include <vector>
#include <algorithm>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "../includes/graphics/vulkan_swapchain.h"
#include "../includes/graphics/vulkan_device_private.h"

using namespace std;

VulkanSwapchain::VulkanSwapchain() {
    this->swap_chain = {};
    this->swap_chain_images = {};
    this->swap_chain_image_views = {};
    this->swap_chain_image_format = {};
    this->swap_chain_extent = {};
}

VkSurfaceFormatKHR choose_swap_surface_format(
    vector<VkSurfaceFormatKHR> &availableFormats
) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR choose_swap_present_mode(
    vector<VkPresentModeKHR> &availablePresentModes
) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(
    SDL_Window *window,
    VkSurfaceCapabilitiesKHR &capabilities
) {
    if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkResult create_swapchain(
    VulkanDevice *device,
    SDL_Window *window,
    VkSwapchainKHR *swap_chain,
    vector<VkImage> *swap_chain_images,
    VkFormat *swap_chain_image_format,
    VkExtent2D *swap_chain_extent
) {
    DeviceInfos* device_info = device->get_device_info();
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(device_info->formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(device_info->present_modes);
    VkExtent2D extent = choose_swap_extent(window, device_info->capabilities);
    uint32_t imageCount = device_info->capabilities.minImageCount + 1;

    if (device_info->capabilities.maxImageCount > 0 && imageCount > device_info->capabilities.maxImageCount) {
        imageCount = device_info->capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = device->get_surface();
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surface_format.format;
    swapchainInfo.imageColorSpace = surface_format.colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { device_info->graphics_index.value(), device_info->presentation_index.value() };

    if (device_info->graphics_index.value() != device_info->presentation_index.value()) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0; // Optional
        swapchainInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    swapchainInfo.preTransform = device_info->capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device->get_device(), &swapchainInfo, nullptr, swap_chain);

    if (result == VK_SUCCESS) {
        uint32_t image_count = 0;
        vkGetSwapchainImagesKHR(device->get_device(), *swap_chain, &image_count, nullptr);
        swap_chain_images->resize(image_count);
        vkGetSwapchainImagesKHR(device->get_device(), *swap_chain, &image_count, swap_chain_images->data());

        *swap_chain_image_format = surface_format.format;
        *swap_chain_extent = extent;
    }

    return result;
}

VkResult make_image_view(
    VulkanDevice *device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VkImageView *image_view
) {
    VkImageViewCreateInfo createInfo {};

    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    return vkCreateImageView(device->get_device(), &createInfo, nullptr, image_view);
}

VkResult create_image_views(
    VulkanDevice *device,
    vector<VkImage> *swap_chain_images,
    vector<VkImageView> *swap_chain_image_views,
    VkFormat *swap_chain_image_format
) {
    VkResult overall_result = VK_SUCCESS;
    swap_chain_image_views->resize(swap_chain_images->size());

    for (size_t i = 0; i < swap_chain_images->size(); i++) {
        VkResult result = make_image_view(
            device,
            swap_chain_images->at(i),
            *swap_chain_image_format,
            VK_IMAGE_ASPECT_COLOR_BIT,
            &swap_chain_image_views->at(i)
        );

        if (result != VK_SUCCESS && overall_result == VK_SUCCESS) {
            overall_result = result;
        }
    }

    return overall_result;
}

VulkanSwapchainResult VulkanSwapchain::init(
    SDL_Window *window,
    VulkanDevice *device
) {
    cout << "Starting vulkan swapchain setup" << endl;

    VkResult swapchain_result = create_swapchain(
        device, window, &this->swap_chain, &this->swap_chain_images, &this->swap_chain_image_format, &this->swap_chain_extent
    );

    if (swapchain_result == VK_SUCCESS) {
        cout << "Successfully created vulkan swapchain" << endl;
    } else {
        cout << "Could not create swapchain" << endl;
        return VULKAN_SWAPCHAIN_INIT_FAILURE;
    }

    VkResult image_views_result = create_image_views(
        device, &this->swap_chain_images, &this->swap_chain_image_views, &this->swap_chain_image_format
    );

    if (image_views_result == VK_SUCCESS) {
        cout << "Successfully created vulkan image views" << endl;
        return VULKAN_SWAPCHAIN_INIT_SUCCESS;
    } else {
        cout << "Could not create an image view" << endl;
        return VULKAN_SWAPCHAIN_INIT_FAILURE;
    }
}

VkSwapchainKHR VulkanSwapchain::get_swapchain() {
    return this->swap_chain;
}

VkExtent2D VulkanSwapchain::get_swapchain_extent() {
    return this->swap_chain_extent;
}

VkFormat VulkanSwapchain::get_swap_chain_image_format() {
    return this->swap_chain_image_format;
}

vector<VkImageView> VulkanSwapchain::get_swap_chain_image_views() {
    return this->swap_chain_image_views;
}

void VulkanSwapchain::destroy(
    VulkanDevice *device
) {
    for (auto image_view : this->swap_chain_image_views) {
        vkDestroyImageView(device->get_device(), image_view, nullptr);
    }

    vkDestroySwapchainKHR(device->get_device(), this->swap_chain, nullptr);
}