#pragma once

#include <vector>

#define CHUNK_SIZE_8 8
#define CHUNK_SIZE_16 16
#define CHUNK_SIZE_32 32

using namespace std;

class Chunk {
    private:
        size_t chunk_size;
        vector<uint16_t> buffer;

    public:
        /*
         * Creates a 16x16x16 chunk
         */
        Chunk();

        /*
         * Create a new chunk of the given size, must be
         * one of the defined sizes
         */
        Chunk(size_t chunk_size);

        /*
         * Return the size of the chunk
         */
        size_t get_chunk_size();

        /*
         * Return the id of the voxel at x,y,z
         */
        uint16_t get_voxel_id(uint16_t x, uint16_t y, uint16_t z);

        /*
         * Set the id of the voxel at x,y,z
         */
        void set_voxel_id(uint16_t x, uint16_t y, uint16_t z, uint16_t id);

        ~Chunk();
};