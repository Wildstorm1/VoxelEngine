#include <iostream>
#include <vector>
#include <algorithm>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "../includes/graphics/vulkan_image_private.h"
#include "../includes/graphics/vulkan_device_private.h"

using namespace std;

VulkanImage::VulkanImage() {
    this->swap_chain = {};
    this->swap_chain_images = {};
    this->swap_chain_image_views = {};
    this->swap_chain_image_format = {};
    this->swap_chain_extent = {};
    this->render_pass = {};
    this->pipeline_layout = {};
    this->graphics_pipeline = {};
    this->swap_chain_framebuffers = {};
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

        actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

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

VkResult create_image_views(
    VulkanDevice *device,
    vector<VkImage> *swap_chain_images,
    vector<VkImageView> *swap_chain_image_views,
    VkFormat *swap_chain_image_format
) {
    VkResult overall_result = VK_SUCCESS;
    swap_chain_image_views->resize(swap_chain_images->size());

    for (size_t i = 0; i < swap_chain_images->size(); i++) {
        VkImageViewCreateInfo createInfo {};

        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swap_chain_images->at(i);
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = *swap_chain_image_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device->get_device(), &createInfo, nullptr, &swap_chain_image_views->at(i));

        if (result != VK_SUCCESS && overall_result == VK_SUCCESS) {
            overall_result = result;
        }
    }

    return overall_result;
}

VulkanImageResult VulkanImage::init(
    SDL_Window *window,
    VulkanDevice *device
) {
    cout << "Starting vulkan image setup" << endl;

    VkResult swapchain_result = create_swapchain(
        device,
        window,
        &this->swap_chain,
        &this->swap_chain_images,
        &this->swap_chain_image_format,
        &this->swap_chain_extent
    );

    if (swapchain_result == VK_SUCCESS) {
        cout << "Successfully created vulkan swapchain" << endl;
    } else {
        cout << "Could not create swapchain" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkResult image_views = create_image_views(
        device,
        &this->swap_chain_images,
        &this->swap_chain_image_views,
        &this->swap_chain_image_format
    );

    if (image_views == VK_SUCCESS) {
        cout << "Successfully created vulkan image views" << endl;
    } else {
        cout << "Could not create an image view" << endl;
        return EXIT_FAILURE;
    }

    return VULKAN_IMAGE_INIT_SUCCESS;
}

void VulkanImage::destroy(
    VulkanDevice *device
) {
    for (auto image_view : this->swap_chain_image_views) {
        vkDestroyImageView(device->get_device(), image_view, nullptr);
    }

    vkDestroySwapchainKHR(device->get_device(), this->swap_chain, nullptr);
}

/*
VkResult createRenderPass(
    VkDevice device,
    VkRenderPass *render_pass,
    VkFormat *swap_chain_image_format
) {
    VkAttachmentDescription colorAttachment{};
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

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult renderPassResult = vkCreateRenderPass(device, &renderPassInfo, nullptr, render_pass);

    if (renderPassResult == VK_SUCCESS) {
        cout << "Successfully created vulkan render pass" << endl;
    } else {
        cout << "Could not create render pass" << endl;\
    }

    return renderPassResult;
}
*/

    /*
    ShaderDetails shaderDetails = loadShaders(this->device.device());
    VkPipelineShaderStageCreateInfo shaderStages[] = { shaderDetails.vertex_shader, shaderDetails.fragment_shader };

    vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) this->swap_chain_extent.width;
    viewport.height = (float) this->swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = this->swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    VkResult renderPassResult = createRenderPass(this->device.device(), &this->render_pass, &this->swap_chain_image_format);

    if (renderPassResult != VK_SUCCESS) {
        return EXIT_FAILURE;
    }

    VkResult layoutResult = vkCreatePipelineLayout(this->device.device(), &pipelineLayoutInfo, nullptr, &this->pipeline_layout);

    if (layoutResult == VK_SUCCESS) {
        cout << "Successfully created vulkan pipeline layout" << endl;
    } else {
        cout << "Could not create pipeline layout" << endl;
        return EXIT_FAILURE;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = this->pipeline_layout;
    pipelineInfo.renderPass = this->render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VkResult pipelineResult = vkCreateGraphicsPipelines(this->device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->graphics_pipeline);

    if (pipelineResult == VK_SUCCESS) {
        cout << "Successfully created vulkan pipeline" << endl;
    } else {
        cout << "Could not create pipeline" << endl;
        return EXIT_FAILURE;
    }

    vkDestroyShaderModule(this->device.device(), shaderDetails.fragment_shader_module, nullptr);
    vkDestroyShaderModule(this->device.device(), shaderDetails.vertex_shader_module, nullptr);
    */

/*
VkResult create_framebuffers(
    VkDevice device,
    VkRenderPass *render_pass,
    VkExtent2D *swap_chain_extent,
    vector<VkImageView> *swap_chain_image_views,
    vector<VkFramebuffer> *swap_chain_framebuffers
) {
    swap_chain_framebuffers->resize(swap_chain_image_views->size());

    for (size_t i = 0; i < swap_chain_image_views->size(); i++) {
        VkImageView attachments[] = {
            swap_chain_image_views->at(i)
        };

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = *render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swap_chain_extent->width;
        framebufferInfo.height = swap_chain_extent->height;
        framebufferInfo.layers = 1;

        VkResult framebufferResult = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swap_chain_framebuffers->at(i));

        if (framebufferResult != VK_SUCCESS) {
            cout << "Could not create framebuffer" << endl;
            return framebufferResult;
        } else {
            cout << "Successfully created a framebuffer" << endl;
        }
    }

    return VK_SUCCESS;
}

    VkResult framebuffersResult = create_framebuffers(
        this->device.device(),
        &this->render_pass,
        &this->swap_chain_extent,
        &this->swap_chain_image_views,
        &this->swap_chain_framebuffers
    );
*/

/*
        for (auto framebuffer : this->swap_chain_framebuffers) {
            vkDestroyFramebuffer(this->device.device(), framebuffer, nullptr);
        }

        vkDestroyPipeline(this->device.device(), this->graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(this->device.device(), this->pipeline_layout, nullptr);
        vkDestroyRenderPass(this->device.device(), this->render_pass, nullptr);
        */