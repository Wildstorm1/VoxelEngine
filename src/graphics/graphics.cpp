#include <iostream>
#include <fstream>
#include <optional>
#include <vector>
#include <algorithm>
#include <set>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "../includes/graphics/graphics.h"

using namespace std;

struct QueueFamilyIndicies {
    optional<uint32_t> graphics_index;
    optional<uint32_t> presentation_index;
};

struct DeviceInfo {
    QueueFamilyIndicies indicies;
    VkPhysicalDevice device;
};

struct QueuesInfo {
    vector<VkDeviceQueueCreateInfo> infos;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    vector<VkSurfaceFormatKHR> formats;
    vector<VkPresentModeKHR> presentModes;
};

struct ShaderDetails {
    VkPipelineShaderStageCreateInfo vertex_shader;
    VkShaderModule vertex_shader_module;
    VkPipelineShaderStageCreateInfo fragment_shader;
    VkShaderModule fragment_shader_module;
};

const vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_MESH_SHADER_EXTENSION_NAME
};

Graphics::Graphics() {
    this->window = NULL;
    this->instance = {};
    this->surface = {};
    this->logical_device = {};
    this->graphics_queue = {};
    this->presentation_queue = {};
}

bool checkValidationLayerSupport(
    vector<const char*> *validationLayers,
    vector<VkLayerProperties> *availableLayers
) {
    for (const char* layerName : *validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : *availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

/*
 * Takes ownership over window until release
 */
int Graphics::init(
    SDL_Window *window
) {
    this->window = window;
    cout << "Starting vulkan setup" << endl;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SettleAndSurvive";
    appInfo.pEngineName = "Presto";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    Uint32 extension_count;
    char const* const* extension_names = SDL_Vulkan_GetInstanceExtensions(&extension_count);

    if (extension_names == NULL) {
        cout << "No extensions found for SDL windowing" << endl;

        return EXIT_FAILURE;
    }

    uint32_t supported_extension_count = 0;
    vector<VkExtensionProperties> extensions(supported_extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, extensions.data());

    for (const auto& extension : extensions) {
        cout << '\t' << extension.extensionName << '\n';
    }

    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    if (enableValidationLayers && !checkValidationLayerSupport(&validationLayers, &availableLayers)) {
        cout << "validation layers requested, but not available!" << endl;

        return EXIT_FAILURE;
    }

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = extension_count;
    instanceInfo.ppEnabledExtensionNames = extension_names;

    if (enableValidationLayers) {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
        cout << "validation layers configured!" << endl;
    } else {
        instanceInfo.enabledLayerCount = 0;
        cout << "validation layers skipped!" << endl;
    }

    VkResult result = vkCreateInstance(&instanceInfo, 0, &this->instance);

    if (result == VK_SUCCESS) {
        cout << "Successfully created vulkan instance" << endl;
    } else {
        return EXIT_FAILURE;
    }

    bool windowCreated = SDL_Vulkan_CreateSurface(window, this->instance, nullptr, &this->surface);

    if (windowCreated) {
        cout << "Successfully created vulkan surface" << endl;
    } else {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool checkDeviceExtensionsSupport(
    VkPhysicalDevice device
) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    vector<VkSurfaceFormatKHR> &availableFormats
) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    vector<VkPresentModeKHR> &availablePresentModes
) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(
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

SwapChainSupportDetails querySwapChainSupport(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

QueueFamilyIndicies getSuitableDeviceQueues(
    SwapChainSupportDetails *swapChainDetails,
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    bool extensionsSupported = checkDeviceExtensionsSupport(device);
    
    bool hasAdequateSwapChain = false;

    if (extensionsSupported) {
        *swapChainDetails = querySwapChainSupport(device, surface);
        hasAdequateSwapChain = !swapChainDetails->formats.empty() && !swapChainDetails->presentModes.empty();
    }

    bool isSuitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && extensionsSupported && hasAdequateSwapChain;

    if (isSuitable) {
        cout << "Suitable Device: " << deviceProperties.deviceName << endl;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    QueueFamilyIndicies indexes = {};

    int i = 0;

    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentationSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupported);

        if (presentationSupported) {
            indexes.presentation_index = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indexes.graphics_index = i;
        }

        i++;
    }

    return indexes;
}

DeviceInfo selectPhysicalDevice(
    SwapChainSupportDetails *swapChainDetails,
    VkInstance instance,
    VkSurfaceKHR surface
) {
    DeviceInfo infos;
    infos.device = VK_NULL_HANDLE;

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
        cout << "No Vulkan GPU's found" << endl;
        return infos;
    }

    vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto& device : devices) {
        infos.indicies = getSuitableDeviceQueues(swapChainDetails, device, surface);

        if (infos.indicies.graphics_index.has_value() && infos.indicies.presentation_index.has_value()) {
            infos.device = device;
            break;
        }
    }

    if (infos.device == VK_NULL_HANDLE || !infos.indicies.graphics_index.has_value() || !infos.indicies.presentation_index.has_value()) {
        cout << "Failed to find a suitable GPU!" << endl;

        infos = {};
        infos.device = VK_NULL_HANDLE;

        return infos;
    }

    return infos;
}

QueuesInfo selectQueues(
    DeviceInfo device_infos
) {
    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    set<uint32_t> uniqueQueueFamilies = {
        device_infos.indicies.graphics_index.value(),
        device_infos.indicies.presentation_index.value()
    };

    float priority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &priority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    QueuesInfo queueInfos;
    queueInfos.infos = queueCreateInfos;
    return queueInfos;
}

VkResult createSwapchain(
    SwapChainSupportDetails *swapChainDetails,
    DeviceInfo *device_infos,
    VkDevice device,
    VkSurfaceKHR surface,
    VkSwapchainKHR *swap_chain,
    vector<VkImage> *swap_chain_images,
    VkFormat *swap_chain_image_format,
    VkExtent2D *swap_chain_extent,
    SDL_Window *window
) {
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails->formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails->presentModes);
    VkExtent2D extent = chooseSwapExtent(window, swapChainDetails->capabilities);
    uint32_t imageCount = swapChainDetails->capabilities.minImageCount + 1;

    if (swapChainDetails->capabilities.maxImageCount > 0 && imageCount > swapChainDetails->capabilities.maxImageCount) {
        imageCount = swapChainDetails->capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { device_infos->indicies.graphics_index.value(), device_infos->indicies.presentation_index.value() };

    if (device_infos->indicies.graphics_index.value() != device_infos->indicies.presentation_index.value()) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0; // Optional
        swapchainInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    swapchainInfo.preTransform = swapChainDetails->capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, swap_chain);

    if (result == VK_SUCCESS) {
        uint32_t image_count = 0;
        vkGetSwapchainImagesKHR(device, *swap_chain, &image_count, nullptr);
        swap_chain_images->resize(image_count);
        vkGetSwapchainImagesKHR(device, *swap_chain, &image_count, swap_chain_images->data());

        *swap_chain_image_format = surfaceFormat.format;
        *swap_chain_extent = extent;
    }

    return result;
}

VkResult createImageViews(
    VkDevice device,
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

        VkResult result = vkCreateImageView(device, &createInfo, nullptr, &swap_chain_image_views->at(i));

        if (result != VK_SUCCESS && overall_result == VK_SUCCESS) {
            overall_result = result;
        }
    }

    return overall_result;
}

/*
 * Takes ownership over window until release
 */
int Graphics::select_device() {
    SwapChainSupportDetails swapChainSupport = {};
    DeviceInfo device_infos = selectPhysicalDevice(&swapChainSupport, this->instance, this->surface);

    if (device_infos.device == VK_NULL_HANDLE) {
        cout << "Failed device selection" << endl;
        return EXIT_FAILURE;
    }

    QueuesInfo queue_infos = selectQueues(device_infos);

    VkPhysicalDeviceFeatures deviceFeatures {};

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queue_infos.infos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.infos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult result = vkCreateDevice(device_infos.device, &createInfo, nullptr, &this->logical_device);

    if (result == VK_SUCCESS) {
        cout << "Successfully created vulkan logical device" << endl;
    } else {
        cout << "Failed to create logical device" << endl;
        return EXIT_FAILURE;
    }

    vkGetDeviceQueue(this->logical_device, device_infos.indicies.graphics_index.value(), 0, &this->graphics_queue);
    vkGetDeviceQueue(this->logical_device, device_infos.indicies.presentation_index.value(), 0, &this->presentation_queue);

    VkResult swapchainResult = createSwapchain(
        &swapChainSupport,
        &device_infos,
        this->logical_device,
        this->surface,
        &this->swap_chain,
        &this->swap_chain_images,
        &this->swap_chain_image_format,
        &this->swap_chain_extent,
        this->window
    );

    if (swapchainResult == VK_SUCCESS) {
        cout << "Successfully created vulkan swapchain" << endl;
    } else {
        cout << "Could not create swapchain" << endl;
        return EXIT_FAILURE;
    }

    VkResult imageViews = createImageViews(
        this->logical_device,
        &this->swap_chain_images,
        &this->swap_chain_image_views,
        &this->swap_chain_image_format
    );

    if (imageViews == VK_SUCCESS) {
        cout << "Successfully created vulkan image views" << endl;
    } else {
        cout << "Could not create an image view" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

VkShaderModule createShaderModule(
    VkDevice device,
    const std::vector<char>& code
) {
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);

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
    VkDevice device
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

/*
 * Sets up image related resources
 */
int Graphics::setup_pipeline() {
    ShaderDetails shaderDetails = loadShaders(this->logical_device);
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

    VkResult renderPassResult = createRenderPass(this->logical_device, &this->render_pass, &this->swap_chain_image_format);

    if (renderPassResult != VK_SUCCESS) {
        return EXIT_FAILURE;
    }

    VkResult layoutResult = vkCreatePipelineLayout(this->logical_device, &pipelineLayoutInfo, nullptr, &this->pipeline_layout);

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

    VkResult pipelineResult = vkCreateGraphicsPipelines(this->logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->graphics_pipeline);

    if (pipelineResult == VK_SUCCESS) {
        cout << "Successfully created vulkan pipeline" << endl;
    } else {
        cout << "Could not create pipeline" << endl;
        return EXIT_FAILURE;
    }

    vkDestroyShaderModule(this->logical_device, shaderDetails.fragment_shader_module, nullptr);
    vkDestroyShaderModule(this->logical_device, shaderDetails.vertex_shader_module, nullptr);

    return EXIT_SUCCESS;
}

/*
 * Gives ownership of window back to the caller
 */
SDL_Window *Graphics::release() {
    SDL_Window *tempWindow = this->window;

    if (this->window != nullptr) {
        vkDestroyPipeline(this->logical_device, this->graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(this->logical_device, this->pipeline_layout, nullptr);
        vkDestroyRenderPass(this->logical_device, this->render_pass, nullptr);

        for (auto image_view : this->swap_chain_image_views) {
            vkDestroyImageView(this->logical_device, image_view, nullptr);
        }

        vkDestroySwapchainKHR(this->logical_device, this->swap_chain, nullptr);
        vkDestroyDevice(this->logical_device, nullptr);
        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
        vkDestroyInstance(this->instance, nullptr);

        this->instance = {};
        this->logical_device = {};
        this->graphics_queue = {};
        this->presentation_queue = {};
        this->surface = {};
        this->swap_chain = {};
    }

    this->window = nullptr;
    return tempWindow;
}