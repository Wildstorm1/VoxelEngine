#pragma once

#include <glm/glm.hpp>
#include "../voxels/mesh.h"

typedef int MeshHeapResult;

#define HEAP_SUCCESS 0
#define HEAP_FAILURE 1

using namespace glm;

/*
 * Module defines an interface for a heap of mesh information which
 * is copied and consumed onto the gpu
 */
class MeshHeap {
    public:
        /*
         * Buffers a model in cpu memory for later copying to the gpu
         *
         * @param model the model matrix to apply to the mesh
         * @param mesh the mesh to draw
         */
        virtual MeshHeapResult buffer_model_on_cpu(mat4 model, Mesh mesh) = 0;

        /*
         * Copies cpu cached data to the gpu
         */
        virtual MeshHeapResult write_models_to_gpu() = 0;
};