#pragma once

#include <vulkan/vulkan.h>
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
         * Marker for the beginning of a new frame
         * TODO: Decouple this from vulkan (probably need a list of frames passed in)
         */
        virtual MeshHeapResult begin_frame(size_t frame_index, VkCommandBuffer command_buffer) = 0;

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

        /*
         * Issues draws for the index'th mesh
         *
         * @param index the index of the mesh to draw
         */
        virtual MeshHeapResult draw_mesh(size_t index) = 0;

        /*
         * Returns the number of meshes currently in the heap
         */
        virtual size_t get_mesh_count() = 0;
};