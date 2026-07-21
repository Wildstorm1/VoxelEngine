#include <iostream>
#include <array>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../includes/voxels/mesh.h"
#include "../includes/voxels/mesher.h"
#include "../includes/graphics/memory.h"
#include "../includes/graphics/structs.h"
#include "../includes/graphics/vulkan_frame.h"
#include "../includes/graphics/vulkan_swapchain.h"
#include "../includes/graphics/vulkan_render_pass.h"
#include "../includes/graphics/vulkan_chunk_pipeline.h"
#include "../includes/voxels/chunk.h"

using namespace std;

VulkanFrame::VulkanFrame(
    VulkanDevice *device,
    VulkanSwapchain *swapchain,
    VulkanRenderPass *render_pass,
    VulkanChunkPipeline *pipeline,
    VkCommandPool command_pool,
    size_t frame_index
) {
    this->command_pool = command_pool;
    this->device = device;
    this->swapchain = swapchain;
    this->render_pass = render_pass;
    this->frame_index = frame_index;
    this->pipeline = pipeline;
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
        this->swapchain->get_swapchain(),
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
    renderPassInfo.renderPass = this->render_pass->get_render_pass();
    renderPassInfo.framebuffer = this->render_pass->get_framebuffer(this->image_index);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = this->swapchain->get_swapchain_extent();

    array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(this->command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(this->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline->get_graphics_pipeline());

    // TODO: Does this extent re-fresh?
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(this->swapchain->get_swapchain_extent().width);
    viewport.height = static_cast<float>(this->swapchain->get_swapchain_extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(this->command_buffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = this->swapchain->get_swapchain_extent();
    vkCmdSetScissor(this->command_buffer, 0, 1, &scissor);

    return FRAME_SUCCESS;
}

FrameResult VulkanFrame::set_camera(
    Camera camera
) {
    //vkMapMemory(device->get_device(), this->uniform_buffer_memory, 0, sizeof(UniformBufferObject), 0, &this->uniform_buffer_cpu_memory);
    vkCmdPushConstants(
        this->command_buffer,
        this->pipeline->get_graphics_pipeline_layout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(Camera),
        &camera
    );
    //vkUnmapMemory(device->get_device(), this->uniform_buffer_memory);
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

    VkSemaphore signalSemaphores[] = { this->render_pass->get_framebuffer_semaphore(this->image_index) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(this->device->get_graphics_queue(), 1, &submitInfo, this->in_flight_fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { this->swapchain->get_swapchain() };
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

VkCommandBuffer VulkanFrame::get_command_buffer() {
    return this->command_buffer;
}

void VulkanFrame::destroy() {
    vkDestroySemaphore(this->device->get_device(), this->image_available_semaphore, nullptr);
    vkDestroyFence(this->device->get_device(), this->in_flight_fence, nullptr);
}