#include <vector>
#include "../includes/voxels/mesh.h"

using namespace std;

Mesh::Mesh() {
    vector<Vertex> vertex_list;
    vector<uint32_t> index_list;
    this->vertices = vertex_list;
    this->indicies = index_list;
}

Mesh::Mesh(
    vector<Vertex> vertices,
    vector<uint32_t> indicies
) {
    this->vertices = vertices;
    this->indicies = indicies;
}

vector<Vertex> *Mesh::get_vertex_vector() {
    return &this->vertices;
}

vector<uint32_t> *Mesh::get_index_vector() {
    return &this->indicies;
}

Mesh::~Mesh() {
    this->vertices = {};
    this->indicies = {};
}