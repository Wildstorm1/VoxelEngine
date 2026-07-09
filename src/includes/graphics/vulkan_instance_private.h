#pragma once

#include <vulkan/vulkan.h>

typedef int VulkanInstanceResult;

#define VULKAN_INSTANCE_INIT_SUCCESS 0
#define VULKAN_INSTANCE_INIT_FAILURE 1

using namespace std;

/*
 * Module wraps around vulkan instance creation and provides access
 * to that instance
 */
class VulkanInstance {
    private:
        VkInstance logical_instance;

    public:
        VulkanInstance();

        /*
         * Initializes a new instance of vulkan
         *
         * Should not be invoked more than once
         */
        VulkanInstanceResult init();

        /*
         * Returns a handle to the instance that was created
         *
         * Should not be invoked after destroy is invoked, nor should
         * any handles be used after destroy. Should not be called until
         * after init has been invoked
         */
        VkInstance instance();

        /*
         * Frees the instance
         *
         * Should only be called once. All handles are invalid after
         * invocation. Should not be invoked until after init has been invoked
         */
        void destroy();
};