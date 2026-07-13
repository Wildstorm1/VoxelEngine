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
#include "../includes/graphics/frame.h"
#include "../includes/graphics/vulkan_frame.h"
#include "../includes/graphics/vulkan_instance_private.h"
#include "../includes/graphics/vulkan_image_private.h"

using namespace std;

Graphics::Graphics() {
    this->window = NULL;
    this->instance = {};
    this->image = {};
    this->device = {};
    this->frame_index = 0;
}

/*
 * Takes ownership over window until release
 */
int Graphics::init(
    SDL_Window *window
) {
    this->window = window;
    this->device = VulkanDevice::VulkanDevice();
    VulkanDeviceResult device_result = this->device.init(this->window);

    if (device_result != VULKAN_DEVICE_INIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    this->image = VulkanImage::VulkanImage();
    VulkanImageResult image_result = this->image.init(this->window, &this->device);

    if (image_result != VULKAN_IMAGE_INIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    DeviceInfos* device_info = this->device.get_device_info();

    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device_info->graphics_index.value();

    VkResult commandPoolResult = vkCreateCommandPool(this->device.get_device(), &poolInfo, nullptr, &this->command_pool);

    if (commandPoolResult != VK_SUCCESS) {
        cout << "Could not create command pool" << endl;
        return EXIT_FAILURE;
    } else {
        cout << "Successfully created a command pool" << endl;
    }

    this->frame_buffer.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < this->frame_buffer.size(); i++) {
        Frame *vulkan_frame = new VulkanFrame(&this->device, &this->image, this->command_pool, i);
        this->frame_buffer[i] = vulkan_frame;
        this->frame_buffer[i]->init();
    }

    return EXIT_SUCCESS;
}

/*
 * Sets up graphics pipeline related resources
 */
int Graphics::resize_window() {
    vkDeviceWaitIdle(this->device.get_device());
    this->image.destroy(&this->device);

    this->device.query_device_information();

    this->image = VulkanImage::VulkanImage();
    VulkanImageResult image_result = this->image.init(window, &this->device);

    if (image_result != VULKAN_IMAGE_INIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Begins a render pass
 */
void Graphics::begin_render_pass() {
    this->frame_buffer[this->frame_index]->begin_frame();
}

/*
 * Sets the camera for drawing
 */
void Graphics::set_camera(UniformBufferObject camera) {
    this->frame_buffer[this->frame_index]->set_camera(camera);
}

/*
 * Draws objects during a render pass
 */
void Graphics::draw_chunk(Chunk *chunk) {
    this->frame_buffer[this->frame_index]->draw_chunk(chunk);
}

/*
 * Ends a render pass
 */
void Graphics::end_render_pass() {
    this->frame_buffer[this->frame_index]->end_frame();
    this->frame_index = (this->frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

WindowSize Graphics::get_window_size() {
    VkExtent2D extent = this->image.get_swapchain_extent();

    WindowSize window_size = {};
    window_size.width = extent.width;
    window_size.height = extent.height;
    return window_size;
}

/*
 * Gives ownership of window back to the caller
 */
SDL_Window *Graphics::release() {
    SDL_Window *tempWindow = this->window;

    if (this->window != nullptr) {
        vkDeviceWaitIdle(this->device.get_device());

        // Free the framebuffers
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            this->frame_buffer[i]->destroy();
            delete this->frame_buffer[i];
        }

        vkDestroyCommandPool(this->device.get_device(), this->command_pool, nullptr);

        this->image.destroy(&this->device);
        this->device.destroy();
    }

    this->window = nullptr;
    return tempWindow;
}