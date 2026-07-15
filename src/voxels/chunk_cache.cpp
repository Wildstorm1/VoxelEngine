#include <vector>
#include "../includes/voxels/chunk_cache.h"
#include "../includes/voxels/chunk.h"

ChunkCache::ChunkCache() : ChunkCache(0, 0) {}

size_t get_index(
    size_t horizontal_size,
    size_t depth,
    size_t x,
    size_t y,
    size_t z
) {
    return y * horizontal_size * horizontal_size + x * horizontal_size + z;
}

ChunkCache::ChunkCache(
    size_t world_horizontal_size,
    size_t depth
) {
    vector<Chunk*> chunks;
    this->horizontal_size = world_horizontal_size;
    this->depth = depth;

    chunks.resize(world_horizontal_size * world_horizontal_size * depth);

    for (size_t x = 0; x < this->horizontal_size; x++) {
        for (size_t z = 0; z < this->horizontal_size; z++) {
            for (size_t y = 0; y < this->depth; y++) {
                size_t index = get_index(this->horizontal_size, this->depth, x, y, z);
                chunks[index] = nullptr;
            }
        }
    }

    this->chunks = chunks;
}

bool ChunkCache::chunk_is_cached(
    size_t x,
    size_t y,
    size_t z
) {
    if (x >= this->horizontal_size || z >= this->horizontal_size || y >= this->depth) {
        return false;
    }

    size_t index = get_index(this->horizontal_size, this->depth, x, y, z);
    return this->chunks[index] != nullptr;
}

void ChunkCache::cache_chunk(
    size_t x,
    size_t y,
    size_t z,
    Chunk *chunk
) {
    size_t index = get_index(this->horizontal_size, this->depth, x, y, z);
    this->chunks[index] = chunk;
}

Chunk *ChunkCache::get_chunk(
    size_t x,
    size_t y,
    size_t z
) {
    size_t index = get_index(this->horizontal_size, this->depth, x, y, z);
    return this->chunks[index];
}

Chunk** ChunkCache::get_chunk_neighbors(
    size_t x,
    size_t y,
    size_t z
) {
    Chunk** neighbors = (Chunk**) malloc(6 * sizeof(Chunk*));
    size_t front_index = get_index(this->horizontal_size, this->depth, x + 1, y, z);
    size_t back_index = get_index(this->horizontal_size, this->depth, x - 1, y, z);
    size_t left_index = get_index(this->horizontal_size, this->depth, x, y, z - 1);
    size_t right_index = get_index(this->horizontal_size, this->depth, x, y, z + 1);
    size_t top_index = get_index(this->horizontal_size, this->depth, x, y + 1, z);
    size_t bottom_index = get_index(this->horizontal_size, this->depth, x, y - 1, z);

    neighbors[0] = this->chunks[front_index];
    neighbors[1] = this->chunks[back_index];
    neighbors[2] = this->chunks[left_index];
    neighbors[3] = this->chunks[right_index];
    neighbors[4] = this->chunks[top_index];
    neighbors[5] = this->chunks[bottom_index];
    return neighbors;
}

ChunkCache::~ChunkCache() {
    for (size_t x = 0; x < this->horizontal_size; x++) {
        for (size_t z = 0; z < this->horizontal_size; z++) {
            for (size_t y = 0; y < this->horizontal_size; y++) {
                size_t index = get_index(this->horizontal_size, this->depth, x, y, z);
                delete chunks[index];
            }
        }
    }
}
