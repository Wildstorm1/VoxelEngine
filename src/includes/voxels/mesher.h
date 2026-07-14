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
         *
         * @param chunk the chunk to mesh
         * @param neighbors neighboring chunks used to calculate open faces. neighbors: front, back, left, right, top, bottom
         */
        Mesh* mesh_chunk_on_cpu(Chunk *chunk, Chunk *neighbors[]);
};