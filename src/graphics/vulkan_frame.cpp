#include <iostream>
#include <array>
#include <vulkan/vulkan.h>
#include "../includes/graphics/vulkan_frame.h"

using namespace std;

VulkanFrame::VulkanFrame(
    VulkanDevice *device,
    VulkanImage *image,
    VkCommandPool command_pool
) {
    this->command_pool = command_pool;
    this->device = device;
    this->image = image;
}

FrameResult VulkanFrame::init() {
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkResult commandBuffer = vkAllocateCommandBuffers(this->device->get_device(), &allocInfo, &this->command_buffer);

    if (commandBuffer != VK_SUCCESS) {
        cout << "Could not create command buffer" << endl;
        return FRAME_INIT_FAILURE;
    } else {
        cout << "Successfully created command buffer" << endl;
    }

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Start signaled so the first frame is considered ready to proceed
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (
        vkCreateSemaphore(this->device->get_device(), &semaphoreInfo, nullptr, &this->image_available_semaphore) != VK_SUCCESS ||
        vkCreateFence(this->device->get_device(), &fenceInfo, nullptr, &this->in_flight_fence) != VK_SUCCESS
    ) {
        throw std::runtime_error("Failed to create sync objects!");
    }

    return FRAME_SUCCESS;
}

FrameResult VulkanFrame::begin_frame() {
    // Wait for previous use of frame resources before using them again
    vkWaitForFences(this->device->get_device(), 1, &this->in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(this->device->get_device(), 1, &this->in_flight_fence);

    VkResult next_image_result = vkAcquireNextImageKHR(
        this->device->get_device(),
        this->image->get_swapchain(),
        UINT64_MAX,
        this->image_available_semaphore,
        VK_NULL_HANDLE,
        &this->image_index
    );

    if (next_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
        cout << "Image swapchain is out of date [next image]" << endl;
        //this->resize_window();
    }
 
    vkResetCommandBuffer(this->command_buffer, 0);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(this->command_buffer, &beginInfo) != VK_SUCCESS) {
        throw runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->image->get_render_pass();
    renderPassInfo.framebuffer = this->image->get_framebuffer(this->image_index);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = this->image->get_swapchain_extent();

    VkClearValue clearColor = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(this->command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(this->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->image->get_graphics_pipeline());

    // TODO: Does this extent re-fresh?
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(this->image->get_swapchain_extent().width);
    viewport.height = static_cast<float>(this->image->get_swapchain_extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(this->command_buffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = this->image->get_swapchain_extent();
    vkCmdSetScissor(this->command_buffer, 0, 1, &scissor);

    return FRAME_SUCCESS;
}

void copy_vertex_cpu_staging_buffer_to_gpu(
    VulkanDevice *device,
    VulkanImage *image,
    VkCommandPool command_pool
) {
    const vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    // Copy into staging buffer
    void* data;
    vkMapMemory(device->get_device(), image->get_vertex_staging_buffer_device_memory(), 0, sizeof(Vertex) * 4, 0, &data);
    memcpy(data, vertices.data(), (size_t) sizeof(Vertex) * 4);
    vkUnmapMemory(device->get_device(), image->get_vertex_staging_buffer_device_memory());

    // Transfer from staging buffer to gpu buffer
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device->get_device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = (size_t) sizeof(Vertex) * 4;
    vkCmdCopyBuffer(commandBuffer, image->get_vertex_staging_buffer(),image->get_vertex_buffer(), 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    // Submit the copy for processing
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device->get_graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->get_graphics_queue());

    // Free the command buffer
    vkFreeCommandBuffers(device->get_device(), command_pool, 1, &commandBuffer);
}

void copy_index_cpu_staging_buffer_to_gpu(
    VulkanDevice *device,
    VulkanImage *image,
    VkCommandPool command_pool
) {
    const vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    // Copy into staging buffer
    void* data;
    vkMapMemory(device->get_device(), image->get_index_staging_buffer_device_memory(), 0, sizeof(uint16_t) * 6, 0, &data);
    memcpy(data, indices.data(), (size_t) sizeof(uint16_t) * 6);
    vkUnmapMemory(device->get_device(), image->get_index_staging_buffer_device_memory());

    // Transfer from staging buffer to gpu buffer
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device->get_device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = (size_t) sizeof(uint16_t) * 6;
    vkCmdCopyBuffer(commandBuffer, image->get_index_staging_buffer(), image->get_index_buffer(), 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    // Submit the copy for processing
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device->get_graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->get_graphics_queue());

    // Free the command buffer
    vkFreeCommandBuffers(device->get_device(), command_pool, 1, &commandBuffer);
}

FrameResult VulkanFrame::draw() {
    copy_vertex_cpu_staging_buffer_to_gpu(this->device, this->image, this->command_pool);
    copy_index_cpu_staging_buffer_to_gpu(this->device, this->image, this->command_pool);

    // Draw what is in the gpu buffer
    VkBuffer vertexBuffers[] = { this->image->get_vertex_buffer() };
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(this->command_buffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(this->command_buffer, this->image->get_index_buffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(this->command_buffer, 6, 1, 0, 0, 0);
    return FRAME_SUCCESS;
}

FrameResult VulkanFrame::end_frame() {
    vkCmdEndRenderPass(this->command_buffer);

    if (vkEndCommandBuffer(this->command_buffer) != VK_SUCCESS) {
        throw runtime_error("Failed to record command buffer!");
    }

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { this->image_available_semaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->command_buffer;

    VkSemaphore signalSemaphores[] = { this->image->get_framebuffer_semaphore(this->image_index) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(this->device->get_graphics_queue(), 1, &submitInfo, this->in_flight_fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { this->image->get_swapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &this->image_index;
    presentInfo.pResults = nullptr; // Optional

    VkResult present_image_result = vkQueuePresentKHR(this->device->get_presentation_queue(), &presentInfo);

    if (present_image_result == VK_ERROR_OUT_OF_DATE_KHR || present_image_result == VK_SUBOPTIMAL_KHR) {
        cout << "Image swapchain is out of date [queue]" << endl;
        //this->resize_window();
    }

    return FRAME_SUCCESS;
}

void VulkanFrame::destroy() {
    vkDestroySemaphore(this->device->get_device(), this->image_available_semaphore, nullptr);
    vkDestroyFence(this->device->get_device(), this->in_flight_fence, nullptr);
}