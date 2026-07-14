#pragma once

#include <glm/glm.hpp>
#include "../voxels/mesh.h"

typedef int FrameResult;

#define FRAME_SUCCESS 0
#define FRAME_FAILURE 1
#define MAX_FRAMES_IN_FLIGHT 2

using namespace std;
using namespace glm;

struct UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
};

/*
 * Module defines an interface for the rendering engine
 * abstracting away which underlying driver api is being used
 */
class Frame {
    public:
        /*
         * Initializes a new frame
         *
         * Should not be invoked more than once
         */
        virtual FrameResult init() = 0;

        /*
         * Begins the frame for drawing
         */
        virtual FrameResult begin_frame() = 0;

        /*
         * Draw geometry to the frame using the following camera
         */
        virtual FrameResult set_camera(UniformBufferObject camera) = 0;

        /*
         * Draw geometry to the frame
         */
        virtual FrameResult draw_mesh(Mesh *mesh) = 0;

        /*
         * Finish drawing the frame
         */
        virtual FrameResult end_frame() = 0;

        /*
         * Frees the frame resources
         *
         * Should only be called once. All handles are invalid after
         * invocation. Should not be invoked until after init has been invoked
         */
        virtual void destroy() = 0;
};