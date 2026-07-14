#pragma once

#include <vector>
#include <glm/glm.hpp>

using namespace glm;
using namespace std;

struct Vertex {
    vec3 pos;
    vec3 color;
};

class Mesh {
    private:
        vector<Vertex> vertices;
        vector<uint32_t> indicies;

    public:
        /*
         * Creates an empty mesh
         */
        Mesh();

        /*
         * Creates a new mesh
         */
        Mesh(
            vector<Vertex> vertices,
            vector<uint32_t> indicies
        );

        /*
         * Return a pointer to the vertex array
         */
        vector<Vertex> *get_vertex_vector();

        /*
         * Return a pointer to the index array
         */
        vector<uint32_t> *get_index_vector();

        ~Mesh();
};