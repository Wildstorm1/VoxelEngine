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
         * Creates a CHUNK_SIZE_16 chunk
         */
        Chunk();

        /*
         * Create a new chunk of the given size
         * 
         * Must be one of the defined sizes
         */
        Chunk(size_t chunk_size);

        /*
         * Return the size of the chunk
         */
        size_t get_chunk_size();

        /*
         * Return the id of the voxel at x,y,z
         *
         * @param x the x coordinate in chunk space
         * @param y the y coordinate in chunk space
         * @param z the z coordinate in chunk space
         */
        uint16_t get_voxel_id(size_t x, size_t y, size_t z);

        /*
         * Set the id of the voxel at x,y,z
         *
         * @param x the x coordinate in chunk space
         * @param y the y coordinate in chunk space
         * @param z the z coordinate in chunk space
         * @param id the id of the voxel to put into the chunk
         */
        void set_voxel_id(size_t x, size_t y, size_t z, uint16_t id);

        ~Chunk();
};