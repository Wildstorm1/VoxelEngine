#pragma once

#include <vector>
#include "./chunk.h"

using namespace std;

class ChunkCache {
    private:
        size_t horizontal_size;
        size_t depth;
        vector<Chunk*> chunks;

    public:
        /*
         * Creates a 0,0,0 chunk cache
         */
        ChunkCache();

        /*
         * Creates a chunk cache of the given world-size
         *
         * @param world_horizontal_size the x-z dimension of the world
         * @param depth the y depth of the world
         */
        ChunkCache(size_t world_horizontal_size, size_t depth);

        /*
         * Return true if the chunk is in the cache, false otherwise
         *
         * @param x the x chunk space offset of the chunk
         * @param y the y chunk space offset of the chunk
         * @param z the z chunk space offset of the chunk
         * @requires the x,y,z be within bounds of the chunk_space
         */
        bool chunk_is_cached(size_t x, size_t y, size_t z);

        /*
         * Cache the given chunk, takes ownership over the chunk pointer
         *
         * @param x the x chunk space offset of the chunk
         * @param y the y chunk space offset of the chunk
         * @param z the z chunk space offset of the chunk
         * @param chunk the chunk to cache
         * @requires the x,y,z be within bounds of the chunk_space
         */
        void cache_chunk(size_t x, size_t y, size_t z, Chunk *chunk);

        /*
         * Return the chunk at the given chunk space offset or nullptr
         * if the chunk is not cached
         *
         * @param x the x chunk space offset of the chunk
         * @param y the y chunk space offset of the chunk
         * @param z the z chunk space offset of the chunk
         * @requires the x,y,z be within bounds of the chunk_space
         */
        Chunk *get_chunk(size_t x, size_t y, size_t z);

        /*
         * Return an array of pointers to chunks neighboring the given
         * offset. neighbors: front, back, left, right, top, bottom
         *
         * @param x the x chunk space offset of the chunk
         * @param y the y chunk space offset of the chunk
         * @param z the z chunk space offset of the chunk
         * @requires that x,y,z are within the chunk space and that the
         * chunk exists in the cache
         */
        Chunk** get_chunk_neighbors(size_t x, size_t y, size_t z);

        /*
         * Destroys all of the chunks in the cache
         */
        ~ChunkCache();
};