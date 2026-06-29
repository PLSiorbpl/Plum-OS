#include "mesh.hpp"

namespace MyCraft {
    void Generate_mesh(Chunk *chunk) {
        int WorldOffsetX = chunk->chunk_x * chunk->width;
        int WorldOffsetZ = chunk->chunk_z * chunk->depth;

        for (int x = 0; x < chunk->width; x++) {
            for (int z = 0; z < chunk->depth; z++) {
                for (int y = 0; y < chunk->height; y++) {
                    if (chunk->get(x,y,z).id != 0) {

                        auto push = [&](const int x_, const int y_, const int z_) {
                            Chunk::Vertex v;
                            v.pos = glm::vec3(x_, y_, z_);
                            v.color = glm::vec3(0.0f, 0.5f, 1.0f);
                            chunk->mesh.push_back(v);
                        };

                        push(x+WorldOffsetX, y, z+WorldOffsetZ);
                        push(x+WorldOffsetX, y, z+WorldOffsetZ+1);
                        push(x+WorldOffsetX, y+1, z+WorldOffsetZ+1);
                    }
                }
            }
        }
    }
}
