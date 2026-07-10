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
#include "../includes/graphics/vulkan_instance_private.h"
#include "../includes/graphics/vulkan_image_private.h"

using namespace std;

const int MAX_FRAMES_IN_FLIGHT = 2;

Graphics::Graphics() {
    this->window = NULL;
    this->instance = {};
    this->image = {};
    this->device = {};
    this->frame_index = 0;
    this->image_index = 0;
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

    return EXIT_SUCCESS;
}

/*
 * Sets up image related resources
 */
int Graphics::setup_draw_resources() {
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

    this->command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    this->image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->render_finished_semaphores.resize(this->image.get_number_of_images());
    this->in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) this->command_buffers.size();

    VkResult commandBuffer = vkAllocateCommandBuffers(this->device.get_device(), &allocInfo, this->command_buffers.data());

    if (commandBuffer != VK_SUCCESS) {
        cout << "Could not create command buffers" << endl;
        return EXIT_FAILURE;
    } else {
        cout << "Successfully created command buffers" << endl;
    }

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Start signaled so the first frame is considered ready to proceed
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (
            vkCreateSemaphore(this->device.get_device(), &semaphoreInfo, nullptr, &this->image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(this->device.get_device(), &fenceInfo, nullptr, &this->in_flight_fences[i]) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create sync objects!");
        }
    }

    for (size_t i = 0; i < this->image.get_number_of_images(); i++) {
        if (
            vkCreateSemaphore(this->device.get_device(), &semaphoreInfo, nullptr, &this->render_finished_semaphores[i]) != VK_SUCCESS
        ) {
            throw std::runtime_error("Failed to create sync objects!");
        }
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
    // Wait for previous use of frame resources before using them again
    vkWaitForFences(this->device.get_device(), 1, &this->in_flight_fences[this->frame_index], VK_TRUE, UINT64_MAX);
    vkResetFences(this->device.get_device(), 1, &this->in_flight_fences[this->frame_index]);

    VkResult next_image_result = vkAcquireNextImageKHR(
        this->device.get_device(),
        this->image.get_swapchain(),
        UINT64_MAX,
        this->image_available_semaphores[this->frame_index],
        VK_NULL_HANDLE,
        &this->image_index
    );

    if (next_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
        cout << "Image swapchain is out of date [next image]" << endl;
        //this->resize_window();
    }

    vkResetCommandBuffer(this->command_buffers[this->frame_index], 0);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(this->command_buffers[this->frame_index], &beginInfo) != VK_SUCCESS) {
        throw runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->image.get_render_pass();
    renderPassInfo.framebuffer = this->image.get_framebuffer(this->image_index);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = this->image.get_swapchain_extent();

    VkClearValue clearColor = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(this->command_buffers[this->frame_index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(this->command_buffers[this->frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, this->image.get_graphics_pipeline());

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(this->image.get_swapchain_extent().width);
    viewport.height = static_cast<float>(this->image.get_swapchain_extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(this->command_buffers[this->frame_index], 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = this->image.get_swapchain_extent();
    vkCmdSetScissor(this->command_buffers[this->frame_index], 0, 1, &scissor);
}

/*
 * Draws objects during a render pass
 */
void Graphics::draw() {
    vkCmdDraw(this->command_buffers[this->frame_index], 3, 1, 0, 0);
}

/*
 * Ends a render pass
 */
void Graphics::end_render_pass() {
    vkCmdEndRenderPass(this->command_buffers[this->frame_index]);

    if (vkEndCommandBuffer(this->command_buffers[this->frame_index]) != VK_SUCCESS) {
        throw runtime_error("Failed to record command buffer!");
    }

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { this->image_available_semaphores[this->frame_index] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->command_buffers[this->frame_index];

    VkSemaphore signalSemaphores[] = { this->render_finished_semaphores[this->image_index] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(this->device.get_graphics_queue(), 1, &submitInfo, this->in_flight_fences[this->frame_index]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { this->image.get_swapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &this->image_index;
    presentInfo.pResults = nullptr; // Optional

    VkResult present_image_result = vkQueuePresentKHR(this->device.get_presentation_queue(), &presentInfo);

    if (present_image_result == VK_ERROR_OUT_OF_DATE_KHR || present_image_result == VK_SUBOPTIMAL_KHR) {
        cout << "Image swapchain is out of date [queue]" << endl;
        //this->resize_window();
    }

    this->frame_index = (this->frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
 * Gives ownership of window back to the caller
 */
SDL_Window *Graphics::release() {
    SDL_Window *tempWindow = this->window;

    if (this->window != nullptr) {
        vkDeviceWaitIdle(this->device.get_device());

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(this->device.get_device(), this->image_available_semaphores[i], nullptr);
            vkDestroyFence(this->device.get_device(), this->in_flight_fences[i], nullptr);
        }

        for (size_t i = 0; i < this->image.get_number_of_images(); i++) {
            vkDestroySemaphore(this->device.get_device(), this->render_finished_semaphores[i], nullptr);
        }

        vkDestroyCommandPool(this->device.get_device(), this->command_pool, nullptr);

        this->image.destroy(&this->device);
        this->device.destroy();
    }

    this->window = nullptr;
    return tempWindow;
}