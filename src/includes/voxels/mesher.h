#pragma once

#include "./mesh.h"
#include "./chunk.h"

using namespace std;

class Mesher {
    private:

    public:
        Mesher();

        /*
         * Return a mesh for the given chunk and its neighbors
         */
        Mesh* mesh_chunk_on_cpu(Chunk *chunk, Chunk *neighbors[]);
};