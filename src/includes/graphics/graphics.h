#pragma once

#include <optional>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "./structs.h"
#include "./vulkan_frame.h"
#include "./vulkan_mesh_heap.h"
#include "./vulkan_instance_private.h"
#include "./vulkan_image_private.h"
#include "./vulkan_device_private.h"
#include "./vulkan_chunk_pipeline.h"
#include "../voxels/mesh.h"

using namespace std;

struct WindowSize {
    uint32_t width;
    uint32_t height;
};

class Graphics {
    private:
        VulkanInstance instance;
        VulkanDevice device;
        VulkanImage *image;
        VulkanArrayMeshHeap *heap;
        VulkanChunkPipeline *pipeline;

        SDL_Window *window;
        VkCommandPool command_pool;
        vector<VulkanFrame*> frame_buffer;
        uint32_t frame_index;

    public:
        Graphics();

        /*
         * Takes ownership over window until release
         */
        int init(SDL_Window *window);

        /*
         * Sets up graphics pipeline related resources
         */
        int resize_window();

        /*
         * Forward an inner command
         */
        void begin_hacky_frame();

        /*
         * Begins a render pass
         */
        void begin_render_pass();

        /*
         * Sets the camera for drawing
         */
        void set_camera(Camera camera);

        /*
         * Loads a mesh onto the gpu
         */
        void queue_mesh_to_gpu(UniformBufferObject *model, Mesh mesh);

        /*
         * Transfers models from the cpu to gpu memory
         */
        void push_models_to_gpu();

        /*
         * Draws objects during a render pass
         */
        void draw_mesh(size_t index);

        /*
         * Return the number of meshes stored in the heap
         */
        size_t get_mesh_count();

        /*
         * Ends a render pass
         */
        void end_render_pass();
        WindowSize get_window_size();
        SDL_Window *release();
};