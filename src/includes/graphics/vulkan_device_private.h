#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "./vulkan_instance_private.h"

typedef int VulkanDeviceResult;

#define VULKAN_DEVICE_INIT_SUCCESS 0
#define VULKAN_DEVICE_INIT_FAILURE 1

using namespace std;

struct DeviceInfos {
    vector<VkSurfaceFormatKHR> formats;
    vector<VkPresentModeKHR> present_modes;
    vector<VkDeviceQueueCreateInfo> queue_infos;
    vector<uint32_t> testing;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkSurfaceCapabilitiesKHR capabilities;
    VkPhysicalDevice device;
    optional<uint32_t> graphics_index;
    optional<uint32_t> presentation_index;
    uint32_t device_score;
};

/*
 * Module wraps around vulkan device creation and selection details
 * and provides access to the 
 */
class VulkanDevice {
    private:
        VulkanInstance instance;
        DeviceInfos* device_info;
        VkSurfaceKHR surface;
        VkDevice logical_device;
        VkQueue graphics_queue;
        VkQueue presentation_queue;

    public:
        VulkanDevice();

        /*
         * Initializes a new vulkan driven gpu device
         *
         * Should not be invoked more than once
         */
        VulkanDeviceResult init(SDL_Window *window);

        /*
         * Returns a handle to the device that was created
         *
         * Should not be invoked after destroy is invoked, nor should
         * any handles be used after destroy. Should not be called until
         * after init has been invoked
         */
        VkDevice get_device();

        /*
         * Returns device related information
         *
         * Should not be invoked after destroy, nor should any handles be used
         * after destroy
         */
        DeviceInfos* get_device_info();

        /*
         * Returns the created vulkan surface
         *
         * Should not be invoked after destroy, nor should any handles be used
         * after destroy
         */
        VkSurfaceKHR get_surface();

        /*
         * Frees the device
         *
         * Should only be called once. All handles are invalid after
         * invocation. Should not be invoked until after init has been invoked
         */
        void destroy();
};