#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define MAX_FRAMES_IN_FLIGHT 2

using namespace glm;

struct UniformBufferObject {
    mat4 model;
};

struct Camera {
    mat4 view;
    mat4 proj;
};