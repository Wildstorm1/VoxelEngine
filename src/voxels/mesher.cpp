#include "../includes/voxels/mesh.h"
#include "../includes/voxels/chunk.h"
#include "../includes/voxels/mesher.h"

using namespace std;

Mesher::Mesher() {}

void push_face(
    vec3 voxel_offset,
    vec3 vertex_0,
    vec3 vertex_1,
    vec3 vertex_2,
    vec3 vertex_3,
    vector<Vertex> *vertices,
    vector<uint32_t> *indicies
) {
    uint32_t index = (uint32_t) vertices->size();
    vertices->push_back({vertex_0, {1.0f, 0.0f, 0.0f}});
    vertices->push_back({vertex_1, {0.0f, 1.0f, 0.0f}});
    vertices->push_back({vertex_2, {0.0f, 0.0f, 1.0f}});
    vertices->push_back({vertex_3, {1.0f, 1.0f, 1.0f}});
    indicies->push_back(index);
    indicies->push_back(index + 1);
    indicies->push_back(index + 2);
    indicies->push_back(index + 2);
    indicies->push_back(index + 1);
    indicies->push_back(index + 3);
}

void push_right_face(
    vec3 voxel_offset,
    vector<Vertex> *vertices,
    vector<uint32_t> *indicies
) {
    uint32_t index = (uint32_t) vertices->size();
    vec3 v0 = voxel_offset + vec3(0.0f, 0.0f, 1.0f);
    vec3 v1 = voxel_offset + vec3(1.0f, 0.0f, 1.0f);
    vec3 v2 = voxel_offset + vec3(0.0f, 1.0f, 1.0f);
    vec3 v3 = voxel_offset + vec3(1.0f, 1.0f, 1.0f);
    push_face(voxel_offset, v0, v1, v2, v3, vertices, indicies);
}

void push_front_face(
    vec3 voxel_offset,
    vector<Vertex> *vertices,
    vector<uint32_t> *indicies
) {
    uint32_t index = (uint32_t) vertices->size();
    vec3 v0 = voxel_offset + vec3(1.0f, 0.0f, 0.0f);
    vec3 v1 = voxel_offset + vec3(1.0f, 1.0f, 0.0f);
    vec3 v2 = voxel_offset + vec3(1.0f, 0.0f, 1.0f);
    vec3 v3 = voxel_offset + vec3(1.0f, 1.0f, 1.0f);
    push_face(voxel_offset, v0, v1, v2, v3, vertices, indicies);
}

void push_bottom_face(
    vec3 voxel_offset,
    vector<Vertex> *vertices,
    vector<uint32_t> *indicies
) {
    uint32_t index = (uint32_t) vertices->size();
    vec3 v0 = voxel_offset + vec3(0.0f, 0.0f, 0.0f);
    vec3 v1 = voxel_offset + vec3(1.0f, 0.0f, 0.0f);
    vec3 v2 = voxel_offset + vec3(0.0f, 0.0f, 1.0f);
    vec3 v3 = voxel_offset + vec3(1.0f, 0.0f, 1.0f);
    push_face(voxel_offset, v0, v1, v2, v3, vertices, indicies);
}

void push_left_face(
    vec3 voxel_offset,
    vector<Vertex> *vertices,
    vector<uint32_t> *indicies
) {
    uint32_t index = (uint32_t) vertices->size();
    vec3 v0 = voxel_offset + vec3(0.0f, 0.0f, 0.0f);
    vec3 v1 = voxel_offset + vec3(0.0f, 1.0f, 0.0f);
    vec3 v2 = voxel_offset + vec3(1.0f, 0.0f, 0.0f);
    vec3 v3 = voxel_offset + vec3(1.0f, 1.0f, 0.0f);
    push_face(voxel_offset, v0, v1, v2, v3, vertices, indicies);
}

void push_back_face(
    vec3 voxel_offset,
    vector<Vertex> *vertices,
    vector<uint32_t> *indicies
) {
    uint32_t index = (uint32_t) vertices->size();
    vec3 v0 = voxel_offset + vec3(0.0f, 0.0f, 0.0f);
    vec3 v1 = voxel_offset + vec3(0.0f, 0.0f, 1.0f);
    vec3 v2 = voxel_offset + vec3(0.0f, 1.0f, 0.0f);
    vec3 v3 = voxel_offset + vec3(0.0f, 1.0f, 1.0f);
    push_face(voxel_offset, v0, v1, v2, v3, vertices, indicies);
}

void push_top_face(
    vec3 voxel_offset,
    vector<Vertex> *vertices,
    vector<uint32_t> *indicies
) {
    uint32_t index = (uint32_t) vertices->size();
    vec3 v0 = voxel_offset + vec3(0.0f, 1.0f, 0.0f);
    vec3 v1 = voxel_offset + vec3(0.0f, 1.0f, 1.0f);
    vec3 v2 = voxel_offset + vec3(1.0f, 1.0f, 0.0f);
    vec3 v3 = voxel_offset + vec3(1.0f, 1.0f, 1.0f);
    push_face(voxel_offset, v0, v1, v2, v3, vertices, indicies);
}

Mesh* Mesher::mesh_chunk_on_cpu(Chunk *chunk, Chunk *neighbors[]) {
    vector<uint32_t> indices;
    vector<Vertex> vertices;
    size_t chunk_size = chunk->get_chunk_size();

    for (size_t x = 0; x < chunk_size; x++) {
        for (size_t y = 0; y < chunk_size; y++) {
            for (size_t z = 0; z < chunk_size; z++) {
                if (chunk->get_voxel_id(x, y, z) != 0) {
                    vec3 v0 = {x, y, z};

                    /*
                    push_right_face(v0, &vertices, &indices);
                    push_left_face(v0, &vertices, &indices);
                    push_front_face(v0, &vertices, &indices);
                    push_back_face(v0, &vertices, &indices);
                    push_top_face(v0, &vertices, &indices);
                    push_bottom_face(v0, &vertices, &indices);
                    */

                    // Right face: +z
                    if (z < chunk_size - 1) {
                        if (chunk->get_voxel_id(x, y, z + 1) == 0) {
                            push_right_face(v0, &vertices, &indices);
                        }
                    } else {
                        Chunk *neighbor = neighbors[3]; // right chunk

                        if (neighbor->get_voxel_id(x, y, 0) == 0) {
                            push_right_face(v0, &vertices, &indices);
                        }
                    }

                    // Left face: -z
                    if (z > 0) {
                        if (chunk->get_voxel_id(x, y, z - 1) == 0) {
                            push_left_face(v0, &vertices, &indices);
                        }
                    } else {
                        Chunk *neighbor = neighbors[2]; // left chunk

                        if (neighbor->get_voxel_id(x, y, chunk_size - 1) == 0) {
                            push_left_face(v0, &vertices, &indices);
                        }
                    }

                    // Front face: +x
                    if (x < chunk_size - 1) {
                        if (chunk->get_voxel_id(x + 1, y, z) == 0) {
                            push_front_face(v0, &vertices, &indices);
                        }
                    } else {
                        Chunk *neighbor = neighbors[0]; // front chunk

                        if (neighbor->get_voxel_id(0, y, z) == 0) {
                            push_front_face(v0, &vertices, &indices);
                        }
                    }

                    // Back face: -x
                    if (x > 0) {
                        if (chunk->get_voxel_id(x - 1, y, z) == 0) {
                            push_back_face(v0, &vertices, &indices);
                        }
                    } else {
                        Chunk *neighbor = neighbors[1]; // back chunk

                        if (neighbor->get_voxel_id(chunk_size - 1, y, z) == 0) {
                            push_back_face(v0, &vertices, &indices);
                        }
                    }

                    // Top face: +y
                    if (y < chunk_size - 1) {
                        if (chunk->get_voxel_id(x, y + 1, z) == 0) {
                            push_top_face(v0, &vertices, &indices);
                        }
                    } else {
                        Chunk *neighbor = neighbors[4]; // top chunk

                        if (neighbor->get_voxel_id(x, 0, z) == 0) {
                            push_top_face(v0, &vertices, &indices);
                        }
                    }

                    // Bottom face: -y
                    if (y > 0) {
                        if (chunk->get_voxel_id(x, y - 1, z) == 0) {
                            push_bottom_face(v0, &vertices, &indices);
                        }
                    } else {
                        Chunk *neighbor = neighbors[5]; // bottom chunk

                        if (neighbor->get_voxel_id(x, chunk_size - 1, z) == 0) {
                            push_bottom_face(v0, &vertices, &indices);
                        }
                    }
                }
            }
        }
    }

    return new Mesh(vertices, indices);
}
