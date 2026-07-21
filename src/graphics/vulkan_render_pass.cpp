#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "../includes/voxels/mesh.h"
#include "../includes/graphics/memory.h"
#include "../includes/graphics/vulkan_frame.h"
#include "../includes/graphics/vulkan_swapchain.h"
#include "../includes/graphics/vulkan_render_pass.h"
#include "../includes/graphics/vulkan_device_private.h"

using namespace std;

VulkanRenderPass::VulkanRenderPass() {
    this->render_framebuffer_semaphores = {};
    this->render_pass = {};
    this->swap_chain_framebuffers = {};
}

VkResult create_image_view(
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

VkResult create_render_pass(
    VulkanDevice *device,
    VkFormat *swap_chain_image_format,
    VkRenderPass *render_pass,
    VkFormat depth_image_format
) {
    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = depth_image_format;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = *swap_chain_image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    VkSubpassDependency dependency {};
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult renderPassResult = vkCreateRenderPass(device->get_device(), &renderPassInfo, nullptr, render_pass);

    if (renderPassResult == VK_SUCCESS) {
        cout << "Successfully created vulkan render pass" << endl;
    } else {
        cout << "Could not create render pass" << endl;\
    }

    return renderPassResult;
}

VkResult create_framebuffers(
    VulkanDevice *device,
    VkRenderPass *render_pass,
    VkExtent2D *swap_chain_extent,
    vector<VkImageView> *swap_chain_image_views,
    vector<VkFramebuffer> *swap_chain_framebuffers,
    VkImageView depth_image_view
) {
    swap_chain_framebuffers->resize(swap_chain_image_views->size());

    for (size_t i = 0; i < swap_chain_image_views->size(); i++) {
        array<VkImageView, 2> attachments = {
            swap_chain_image_views->at(i),
            depth_image_view
        };

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = *render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swap_chain_extent->width;
        framebufferInfo.height = swap_chain_extent->height;
        framebufferInfo.layers = 1;

        VkResult framebufferResult = vkCreateFramebuffer(device->get_device(), &framebufferInfo, nullptr, &swap_chain_framebuffers->at(i));

        if (framebufferResult != VK_SUCCESS) {
            cout << "Could not create framebuffer" << endl;
            return framebufferResult;
        } else {
            cout << "Successfully created a framebuffer" << endl;
        }
    }

    return VK_SUCCESS;
}

VkFormat find_supported_format(
    VulkanDevice *device,
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features
) {
    DeviceInfos *device_info = device->get_device_info();

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device_info->device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

VkFormat find_depth_format(
    VulkanDevice *device
) {
    return find_supported_format(
        device,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool has_stencil_component(
    VkFormat format
) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void create_image(
    VulkanDevice *device,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory
) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device->get_device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device->get_device(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(device, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device->get_device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device->get_device(), image, imageMemory, 0);
}

VulkanRenderPassResult VulkanRenderPass::init(
    SDL_Window *window,
    VulkanDevice *device,
    VulkanSwapchain *swapchain
) {
    cout << "Starting vulkan image setup" << endl;

    VkExtent2D swap_chain_extent = swapchain->get_swapchain_extent();
    VkFormat swap_chain_image_format = swapchain->get_swap_chain_image_format();
    vector<VkImageView> swap_chain_image_views = swapchain->get_swap_chain_image_views();

    VkFormat depthFormat = find_depth_format(device);
    create_image(
        device,
        swap_chain_extent.width,
        swap_chain_extent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        this->depth_image,
        this->depth_image_memory
    );

    VkResult depth_image_view_result = create_image_view(
        device,
        this->depth_image,
        depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &this->depth_image_view
    );

    if (depth_image_view_result == VK_SUCCESS) {
        cout << "Successfully created vulkan depth image" << endl;
    } else {
        cout << "Could not create depth image" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkResult render_pass_result = create_render_pass(
        device, &swap_chain_image_format, &this->render_pass, depthFormat
    );

    if (render_pass_result == VK_SUCCESS) {
        cout << "Successfully created vulkan render pass" << endl;
    } else {
        cout << "Could not create a render pass" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkResult framebuffers_result = create_framebuffers(
        device,
        &this->render_pass,
        &swap_chain_extent,
        &swap_chain_image_views,
        &this->swap_chain_framebuffers,
        this->depth_image_view
    );

    if (framebuffers_result == VK_SUCCESS) {
        cout << "Successfully created vulkan framebuffers" << endl;
    } else {
        cout << "Could not create a framebuffer" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    this->render_framebuffer_semaphores.resize(swap_chain_image_views.size());

    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        if (
            vkCreateSemaphore(device->get_device(), &semaphoreInfo, nullptr, &this->render_framebuffer_semaphores[i]) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create sync objects!");
        }
    }

    return framebuffers_result;
}

VkRenderPass VulkanRenderPass::get_render_pass() {
    return this->render_pass;
}

VkSemaphore VulkanRenderPass::get_framebuffer_semaphore(size_t i) {
    return this->render_framebuffer_semaphores[i];
}

VkFramebuffer VulkanRenderPass::get_framebuffer(size_t i) {
    return this->swap_chain_framebuffers[i];
}

void VulkanRenderPass::destroy(
    VulkanDevice *device
) {
    for (size_t i = 0; i < this->render_framebuffer_semaphores.size(); i++) {
        vkDestroySemaphore(device->get_device(), this->render_framebuffer_semaphores[i], nullptr);
    }

    vkDestroyImage(device->get_device(), this->depth_image, nullptr);
    vkFreeMemory(device->get_device(), this->depth_image_memory, nullptr);
    vkDestroyImageView(device->get_device(), this->depth_image_view, nullptr);

    for (auto framebuffer : this->swap_chain_framebuffers) {
        vkDestroyFramebuffer(device->get_device(), framebuffer, nullptr);
    }

    vkDestroyRenderPass(device->get_device(), this->render_pass, nullptr);
}