#include <vulkan/vulkan.h>
#include "../includes/graphics/vulkan_frame.h"

using namespace std;

VulkanFrame::VulkanFrame() {}

FrameResult VulkanFrame::init() {
    return FRAME_SUCCESS;
}

FrameResult VulkanFrame::begin_frame() {
    return FRAME_SUCCESS;
}

FrameResult VulkanFrame::draw() {
    return FRAME_SUCCESS;
}

FrameResult VulkanFrame::end_frame() {
    return FRAME_SUCCESS;
}

void VulkanFrame::destroy() {}