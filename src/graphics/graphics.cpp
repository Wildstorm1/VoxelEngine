#include <iostream>
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

    return vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, swap_chain);
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

    VkResult swapchainResult = createSwapchain(&swapChainSupport, &device_infos, this->logical_device, this->surface, &this->swap_chain, this->window);

    if (swapchainResult == VK_SUCCESS) {
        cout << "Successfully created vulkan swapchain" << endl;
    } else {
        cout << "Could not create swapchain" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Sets up image related resources
 */
int Graphics::setup_image() {
    return 0;
}

/*
 * Gives ownership of window back to the caller
 */
SDL_Window *Graphics::release() {
    SDL_Window *tempWindow = this->window;

    if (this->window != nullptr) {
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