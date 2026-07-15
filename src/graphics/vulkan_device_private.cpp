#include <iostream>
#include <unordered_map>
#include <optional>
#include <vector>
#include <set>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "../includes/graphics/vulkan_instance_private.h"
#include "../includes/graphics/vulkan_device_private.h"

using namespace std;

const vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_MESH_SHADER_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
};

VulkanDevice::VulkanDevice() {
    this->instance = {};
    this->device_info = {};
    this->logical_device = {};
    this->graphics_queue = {};
    this->presentation_queue = {};
    this->surface = {};
}

bool device_supports_required_extensions(
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

void query_surface_support(
    DeviceInfos *device_infos,
    VkSurfaceKHR surface
) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_infos->device, surface, &device_infos->capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device_infos->device, surface, &formatCount, nullptr);
        
    if (formatCount != 0) {
        vector<VkSurfaceFormatKHR> formats_vec;
        formats_vec.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device_infos->device, surface, &formatCount, formats_vec.data());
        device_infos->formats = formats_vec;
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device_infos->device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        vector<VkPresentModeKHR> modes_vec;
        modes_vec.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device_infos->device, surface, &presentModeCount, modes_vec.data());
    }
}

void select_device_queues(
    DeviceInfos *device_infos,
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface
) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device_infos->device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device_infos->device, &queueFamilyCount, queueFamilies.data());

    int i = 0;

    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentationSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device_infos->device, i, surface, &presentationSupported);

        if (presentationSupported) {
            device_infos->presentation_index = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            device_infos->graphics_index = i;
        }

        i++;
    }
}

DeviceInfos* select_physical_device(
    VkInstance instance,
    VkSurfaceKHR surface
) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
        cout << "No Vulkan GPU's found" << endl;
        return {};
    }

    vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    unordered_map<VkPhysicalDevice, DeviceInfos*> device_scores;

    for (const auto& physical_device : devices) {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);

        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceFeatures(physical_device, &device_features);

        cout << "\t Found Device: " << device_properties.deviceName << endl;
        cout << "\t\t Device Type: " << device_properties.deviceType << endl;
        cout << "\t\t Device Index Limit: " << device_properties.limits.maxDrawIndexedIndexValue << endl;

        DeviceInfos* heap_infos = new DeviceInfos();
        device_scores[physical_device] = heap_infos;
        heap_infos->device = physical_device;
        heap_infos->properties = device_properties;
        heap_infos->features = device_features;
        heap_infos->device_score = 0;

        if (device_properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            device_scores[physical_device]->device_score = device_scores[physical_device]->device_score + 1000;
        }

        if (device_supports_required_extensions(physical_device)) {
            device_scores[physical_device]->device_score = device_scores[physical_device]->device_score + 100;
        }

        query_surface_support(device_scores[physical_device], surface);
        
        if (!device_scores[physical_device]->formats.empty() && !device_scores[physical_device]->present_modes.empty()) {
            device_scores[physical_device]->device_score = device_scores[physical_device]->device_score + 100;
        }

        select_device_queues(device_scores[physical_device], physical_device, surface);

        if (device_scores[physical_device]->graphics_index.has_value() && device_scores[physical_device]->presentation_index.has_value()) {
            device_scores[physical_device]->device_score = device_scores[physical_device]->device_score + 250;
        }

        cout << "\t\t Device Score: " << device_scores[physical_device]->device_score << endl;
    }

    DeviceInfos* best_device;
    uint32_t best_score = 0;

    for (const auto& [key, value] : device_scores) {
        if (value->device_score >= best_score) {
            best_device = value;
            best_score = value->device_score;
        }
    }

    for (const auto& [key, value] : device_scores) {
        if (best_device != value) {
            free(value);
        }
    }

    return best_device;
}

void fill_in_queue_info(
    DeviceInfos *device_infos
) {
    set<uint32_t> uniqueQueueFamilies = {
        device_infos->graphics_index.value(),
        device_infos->presentation_index.value()
    };

    float priority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &priority;

        device_infos->queue_infos.push_back(queueCreateInfo);
    }
}

VulkanDeviceResult VulkanDevice::init(SDL_Window *window) {
    this->instance = VulkanInstance::VulkanInstance();
    VulkanDeviceResult instance_result = this->instance.init();

    if (instance_result != VULKAN_DEVICE_INIT_SUCCESS) {
        return VULKAN_DEVICE_INIT_FAILURE;
    }

    bool windowCreated = SDL_Vulkan_CreateSurface(window, this->instance.instance(), nullptr, &this->surface);

    if (windowCreated) {
        cout << "Successfully created vulkan surface" << endl;
    } else {
        return VULKAN_DEVICE_INIT_FAILURE;
    }

    this->device_info = select_physical_device(this->instance.instance(), this->surface);

    if (this->device_info->device == VK_NULL_HANDLE) {
        cout << "Failed device selection" << endl;
        return VULKAN_DEVICE_INIT_FAILURE;
    } else {
        cout << "Selected device: " << this->device_info->properties.deviceName << endl;
    }

    fill_in_queue_info(this->device_info);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures {};
    descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = true;
    descriptorIndexingFeatures.descriptorBindingPartiallyBound = true;
    descriptorIndexingFeatures.pNext = nullptr;

    VkPhysicalDeviceFeatures deviceFeatures {};

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = this->device_info->queue_infos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(this->device_info->queue_infos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pNext = &descriptorIndexingFeatures;

    VkResult device_result = vkCreateDevice(this->device_info->device, &createInfo, nullptr, &this->logical_device);

    if (device_result == VK_SUCCESS) {
        cout << "Successfully created vulkan logical device" << endl;
    } else {
        cout << "Failed to create logical device" << endl;
        return VULKAN_DEVICE_INIT_FAILURE;
    }

    vkGetDeviceQueue(this->logical_device, this->device_info->graphics_index.value(), 0, &this->graphics_queue);
    vkGetDeviceQueue(this->logical_device, this->device_info->presentation_index.value(), 0, &this->presentation_queue);

    return VULKAN_DEVICE_INIT_SUCCESS;
}

VulkanDeviceResult VulkanDevice::query_device_information() {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(this->device_info->device, &device_properties);
    this->device_info->properties = device_properties;

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(this->device_info->device, &device_features);
    this->device_info->features = device_features;

    query_surface_support(this->device_info, this->surface);
    return VULKAN_DEVICE_QUERY_SUCCESS;
}

VkDevice VulkanDevice::get_device() {
    return this->logical_device;
}

DeviceInfos* VulkanDevice::get_device_info() {
    return this->device_info;
}

VkSurfaceKHR VulkanDevice::get_surface() {
    return this->surface;
}

VkQueue VulkanDevice::get_graphics_queue() {
    return this->graphics_queue;
}

VkQueue VulkanDevice::get_presentation_queue() {
    return this->presentation_queue;
}

void VulkanDevice::destroy() {
    vkDestroyDevice(this->logical_device, nullptr);
    vkDestroySurfaceKHR(this->instance.instance(), this->surface, nullptr);
    this->instance.destroy();
}