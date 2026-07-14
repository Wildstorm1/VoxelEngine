#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "../includes/voxels/mesh.h"
#include "../includes/graphics/memory.h"
#include "../includes/graphics/frame.h"
#include "../includes/graphics/vulkan_image_private.h"
#include "../includes/graphics/vulkan_device_private.h"

using namespace std;

struct ShaderDetails {
    VkPipelineShaderStageCreateInfo vertex_shader;
    VkShaderModule vertex_shader_module;
    VkPipelineShaderStageCreateInfo fragment_shader;
    VkShaderModule fragment_shader_module;
};

VulkanImage::VulkanImage() {
    this->swap_chain = {};
    this->swap_chain_images = {};
    this->swap_chain_image_views = {};
    this->render_framebuffer_semaphores = {};
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

VkResult create_image_views(
    VulkanDevice *device,
    vector<VkImage> *swap_chain_images,
    vector<VkImageView> *swap_chain_image_views,
    VkFormat *swap_chain_image_format
) {
    VkResult overall_result = VK_SUCCESS;
    swap_chain_image_views->resize(swap_chain_images->size());

    for (size_t i = 0; i < swap_chain_images->size(); i++) {
        VkResult result = create_image_view(
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

VkShaderModule createShaderModule(
    VulkanDevice *device,
    const std::vector<char>& code
) {
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device->get_device(), &createInfo, nullptr, &shaderModule);

    if (result != VK_SUCCESS) {
        throw runtime_error("failed to create shader module");
    }

    return shaderModule;
}

static vector<char> readFile(
    const string &filename
) {
    ifstream file(filename, ios::ate | ios::binary);

    if (!file.is_open()) {
        throw runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

ShaderDetails loadShaders(
    VulkanDevice *device
) {
    // Would be more appropriate to read from a final distribution destination
    vector<char> vertShaderCode = readFile("../shaders/vert.spv");
    vector<char> fragShaderCode = readFile("../shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    ShaderDetails details = {};
    details.vertex_shader = vertShaderStageInfo;
    details.vertex_shader_module = vertShaderModule;
    details.fragment_shader = fragShaderStageInfo;
    details.fragment_shader_module = fragShaderModule;

    return details;
}

VkResult create_descriptor_set_layout(
    VulkanDevice *device,
    VkDescriptorSetLayout *descriptor_set_layout
) {
    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    return vkCreateDescriptorSetLayout(device->get_device(), &layoutInfo, nullptr, descriptor_set_layout);
}

VkResult create_descriptor_pool(
    VulkanDevice *device,
    VkDescriptorPool *descriptor_pool
) {
    // Allocate 1 per frame, but the size is ambiguous (magic 2's)
    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    return vkCreateDescriptorPool(device->get_device(), &poolInfo, nullptr, descriptor_pool);
}

VulkanImageResult setup_draw_pipeline(
    VulkanDevice *device,
    VkDescriptorSetLayout *descriptor_set_layout,
    VkDescriptorPool *descriptor_pool,
    VkExtent2D *swap_chain_extent,
    VkRenderPass *render_pass,
    VkPipelineLayout *pipeline_layout,
    VkPipeline *graphics_pipeline
) {
    ShaderDetails shaderDetails = loadShaders(device);
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        shaderDetails.vertex_shader, shaderDetails.fragment_shader
    };

    vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkVertexInputBindingDescription bindingDescription {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    array<VkVertexInputAttributeDescription, 2> attributeDescriptions {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swap_chain_extent->width;
    viewport.height = (float) swap_chain_extent->height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = *swap_chain_extent;

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
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

    VkResult descriptorSetLayoutResult = create_descriptor_set_layout(device, descriptor_set_layout);

    if (descriptorSetLayoutResult == VK_SUCCESS) {
        cout << "Successfully created vulkan descriptor set layout" << endl;
    } else {
        cout << "Could not create descriptor set layout" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkResult descriptorSetPoolResult = create_descriptor_pool(device, descriptor_pool);

    if (descriptorSetPoolResult == VK_SUCCESS) {
        cout << "Successfully created vulkan descriptor pool" << endl;
    } else {
        cout << "Could not create descriptor pool" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = descriptor_set_layout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    VkResult layoutResult = vkCreatePipelineLayout(device->get_device(), &pipelineLayoutInfo, nullptr, pipeline_layout);

    if (layoutResult == VK_SUCCESS) {
        cout << "Successfully created vulkan pipeline layout" << endl;
    } else {
        cout << "Could not create pipeline layout" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

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
    pipelineInfo.layout = *pipeline_layout;
    pipelineInfo.renderPass = *render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional
    pipelineInfo.pDepthStencilState = &depthStencil;

    VkResult pipelineResult = vkCreateGraphicsPipelines(device->get_device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphics_pipeline);

    if (pipelineResult == VK_SUCCESS) {
        cout << "Successfully created vulkan pipeline" << endl;
    } else {
        cout << "Could not create pipeline" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    vkDestroyShaderModule(device->get_device(), shaderDetails.fragment_shader_module, nullptr);
    vkDestroyShaderModule(device->get_device(), shaderDetails.vertex_shader_module, nullptr);
    return VULKAN_IMAGE_INIT_SUCCESS;
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

VulkanImageResult VulkanImage::init(
    SDL_Window *window,
    VulkanDevice *device
) {
    cout << "Starting vulkan image setup" << endl;

    VkResult swapchain_result = create_swapchain(
        device, window, &this->swap_chain, &this->swap_chain_images, &this->swap_chain_image_format, &this->swap_chain_extent
    );

    if (swapchain_result == VK_SUCCESS) {
        cout << "Successfully created vulkan swapchain" << endl;
    } else {
        cout << "Could not create swapchain" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkResult image_views_result = create_image_views(
        device, &this->swap_chain_images, &this->swap_chain_image_views, &this->swap_chain_image_format
    );

    if (image_views_result == VK_SUCCESS) {
        cout << "Successfully created vulkan image views" << endl;
    } else {
        cout << "Could not create an image view" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VkFormat depthFormat = find_depth_format(device);
    create_image(
        device,
        this->swap_chain_extent.width,
        this->swap_chain_extent.height,
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
        device, &this->swap_chain_image_format, &this->render_pass, depthFormat
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
        &this->swap_chain_extent,
        &this->swap_chain_image_views,
        &this->swap_chain_framebuffers,
        this->depth_image_view
    );

    if (framebuffers_result == VK_SUCCESS) {
        cout << "Successfully created vulkan framebuffers" << endl;
    } else {
        cout << "Could not create a framebuffer" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

    VulkanImageResult draw_pipeline = setup_draw_pipeline(
        device,
        &this->descriptor_set_layout,
        &this->descriptor_pool,
        &this->swap_chain_extent,
        &this->render_pass,
        &this->pipeline_layout,
        &this->graphics_pipeline
    );

    if (draw_pipeline == VULKAN_IMAGE_INIT_SUCCESS) {
        cout << "Successfully created vulkan pipeline resources" << endl;
    } else {
        cout << "Could not create the pipeline" << endl;
        return VULKAN_IMAGE_INIT_FAILURE;
    }

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
            sizeof(UniformBufferObject),
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
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        vkUpdateDescriptorSets(device->get_device(), 1, &descriptorWrite, 0, nullptr);
    }

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    this->render_framebuffer_semaphores.resize(this->swap_chain_image_views.size());

    for (size_t i = 0; i < this->swap_chain_image_views.size(); i++) {
        if (
            vkCreateSemaphore(device->get_device(), &semaphoreInfo, nullptr, &this->render_framebuffer_semaphores[i]) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create sync objects!");
        }
    }

    return draw_pipeline;
}

VkSwapchainKHR VulkanImage::get_swapchain() {
    return this->swap_chain;
}

VkExtent2D VulkanImage::get_swapchain_extent() {
    return this->swap_chain_extent;
}

VkRenderPass VulkanImage::get_render_pass() {
    return this->render_pass;
}

VkSemaphore VulkanImage::get_framebuffer_semaphore(size_t i) {
    return this->render_framebuffer_semaphores[i];
}

VkFramebuffer VulkanImage::get_framebuffer(size_t i) {
    return this->swap_chain_framebuffers[i];
}

VkPipelineLayout VulkanImage::get_graphics_pipeline_layout() {
    return this->pipeline_layout;
}

VkPipeline VulkanImage::get_graphics_pipeline() {
    return this->graphics_pipeline;
}

VkDeviceMemory VulkanImage::get_vertex_buffer_device_memory() {
    return this->vertex_buffer_memory;
}

VkBuffer VulkanImage::get_vertex_buffer() {
    return this->vertex_buffer;
}

VkDeviceMemory VulkanImage::get_vertex_staging_buffer_device_memory() {
    return this->vertex_staging_buffer_memory;
}

VkBuffer VulkanImage::get_vertex_staging_buffer() {
    return this->vertex_staging_buffer;
}

VkDeviceMemory VulkanImage::get_index_buffer_device_memory() {
    return this->index_buffer_memory;
}

VkBuffer VulkanImage::get_index_buffer() {
    return this->index_buffer;
}

VkDeviceMemory VulkanImage::get_index_staging_buffer_device_memory() {
    return this->index_staging_buffer_memory;
}

VkBuffer VulkanImage::get_index_staging_buffer() {
    return this->index_staging_buffer;
}

VkDescriptorSet* VulkanImage::get_frame_descriptor_set(size_t i) {
    return &this->descriptor_sets[i];
}

void* VulkanImage::get_uniform_frame_buffer_cpu_mapped(size_t i) {
    return this->uniform_frame_buffers_cpu_mapped[i];
}

void VulkanImage::destroy(
    VulkanDevice *device
) {
    vkDeviceWaitIdle(device->get_device());

    for (size_t i = 0; i < this->swap_chain_image_views.size(); i++) {
        vkDestroySemaphore(device->get_device(), this->render_framebuffer_semaphores[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device->get_device(), this->uniform_frame_buffers[i], nullptr);
        vkFreeMemory(device->get_device(), this->uniform_frame_buffers_memory[i], nullptr);
    }

    vkDestroyImage(device->get_device(), this->depth_image, nullptr);
    vkFreeMemory(device->get_device(), this->depth_image_memory, nullptr);
    vkDestroyImageView(device->get_device(), this->depth_image_view, nullptr);

    vkDestroyBuffer(device->get_device(), this->index_staging_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->index_staging_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->index_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->index_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->vertex_staging_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->vertex_staging_buffer_memory, nullptr);
    vkDestroyBuffer(device->get_device(), this->vertex_buffer, nullptr);
    vkFreeMemory(device->get_device(), this->vertex_buffer_memory, nullptr);

    vkDestroyPipeline(device->get_device(), this->graphics_pipeline, nullptr);
    vkDestroyDescriptorPool(device->get_device(), this->descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device->get_device(), this->descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(device->get_device(), this->pipeline_layout, nullptr);

    for (auto framebuffer : this->swap_chain_framebuffers) {
        vkDestroyFramebuffer(device->get_device(), framebuffer, nullptr);
    }

    vkDestroyRenderPass(device->get_device(), this->render_pass, nullptr);

    for (auto image_view : this->swap_chain_image_views) {
        vkDestroyImageView(device->get_device(), image_view, nullptr);
    }

    vkDestroySwapchainKHR(device->get_device(), this->swap_chain, nullptr);
}