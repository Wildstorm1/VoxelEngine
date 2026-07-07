#include <iostream>
#include <optional>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "../includes/graphics/graphics.h"

using namespace std;

Graphics::Graphics() {
    this->window = NULL;
    this->instance = {};
    this->logical_device = {};
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
int Graphics::init(SDL_Window *window) {
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

    return EXIT_SUCCESS;
}

int getSuitableDeviceGraphicsQueue(
    VkPhysicalDevice device
) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    bool isSuitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    if (isSuitable) {
        cout << "Suitable Device: " << deviceProperties.deviceName << endl;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int index = -1;
    int i = 0;

    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            index = i;
        }

        i++;
    }

    return index;
}

/*
 * Takes ownership over window until release
 */
int Graphics::select_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(this->instance, &device_count, nullptr);

    if (device_count == 0) {
        cout << "No Vulkan GPU's found" << endl;
        return EXIT_FAILURE;
    }

    vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(this->instance, &device_count, devices.data());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    int graphics_index = -1;

    for (const auto& device : devices) {
        graphics_index = getSuitableDeviceGraphicsQueue(device);

        if (graphics_index != -1) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE || graphics_index == -1) {
        cout << "Failed to find a suitable GPU!" << endl;
        return EXIT_FAILURE;
    }

    float priority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphics_index;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures deviceFeatures {};

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &this->logical_device);

    if (result == VK_SUCCESS) {
        cout << "Successfully created vulkan logical device" << endl;
    } else {
        return EXIT_FAILURE;
    }

    vkGetDeviceQueue(this->logical_device, graphics_index, 0, &this->graphics_queue);

    return EXIT_SUCCESS;
}

/*
 * Gives ownership of window back to the caller
 */
SDL_Window *Graphics::release() {
    SDL_Window *tempWindow = this->window;

    if (this->window != nullptr) {
        vkDestroyDevice(this->logical_device, nullptr);
        vkDestroyInstance(this->instance, nullptr);

        this->instance = {};
        this->logical_device = {};
    }

    this->window = nullptr;
    return tempWindow;
}