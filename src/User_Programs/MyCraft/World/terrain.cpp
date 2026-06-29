#include "terrain.hpp"
#include "chunk.hpp"

namespace MyCraft {
    void Generate_terrain(int chunk_x, int chunk_z, Chunk *chunk) {
        chunk->chunk_x = chunk_x;
        chunk->chunk_z = chunk_z;
        chunk->has_terrain = true;

        for (int x = 0; x < chunk->width; x++) {
            for (int z = 0; z < chunk->depth; z++) {
                for (int y = 0; y < chunk->height; y++) {
                    if (y == 0)
                        chunk->set(x, y, z, Chunk::Block(1, 0));
                    else
                        chunk->set(x, y, z, Chunk::Block(0, 0));
                }
            }
        }
    }
}
