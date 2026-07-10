#pragma once

#include "./frame.h"

typedef int FrameResult;

#define FRAME_INIT_SUCCESS 0
#define FRAME_INIT_FAILURE 1

using namespace std;

/*
 * Module defines vulkan specific implementation for
 */
class VulkanFrame : Frame {
    private:

    public:
        VulkanFrame();

        FrameResult init();

        FrameResult begin_frame();

        FrameResult draw();

        FrameResult end_frame();

        void destroy();
};