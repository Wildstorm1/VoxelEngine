#include <vector>
#include <stdexcept>
#include "../includes/voxels/chunk.h"

using namespace std;

Chunk::Chunk() : Chunk(CHUNK_SIZE_16) {}

Chunk::Chunk(size_t chunk_size) {
    if (chunk_size != CHUNK_SIZE_8 && chunk_size != CHUNK_SIZE_16 && chunk_size != CHUNK_SIZE_32) {
        throw runtime_error("Invalid chunk size given");
    }

    vector<uint16_t> chunk_buffer;
    chunk_buffer.resize(chunk_size * chunk_size * chunk_size * sizeof(uint16_t));
    this->buffer = chunk_buffer;
    this->chunk_size = chunk_size;
}

size_t Chunk::get_chunk_size() {
    return this->chunk_size;
}

size_t get_index(size_t chunk_size, size_t x, size_t y, size_t z) {
    return chunk_size * chunk_size * x + chunk_size * y + z;
}

uint16_t Chunk::get_voxel_id(size_t x, size_t y, size_t z) {
    size_t index = get_index(this->chunk_size, x, y, z);
    return this->buffer[index];
}

void Chunk::set_voxel_id(size_t x, size_t y, size_t z, uint16_t id) {
    size_t index = get_index(this->chunk_size, x, y, z);
    this->buffer[index] = id;
}

Chunk::~Chunk() {
    this->buffer = {};
}