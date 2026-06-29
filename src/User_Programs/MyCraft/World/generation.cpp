#include "generation.hpp"

#include "std/math.hpp"
#include "std/math_types.hpp"
#include "world.hpp"
#include "terrain.hpp"
#include "Drivers/hpet/hpet.h"
#include "mesh.hpp"

namespace MyCraft {
    void GenerateChunks(const glm::ivec2 center, const int radius) {
        for (int x = -radius-1; x < radius+1; x++) {
            for (int z = -radius-1; z < radius+1; z++) {
                const int chunk_x = center.x + x;
                const int chunk_z = center.y + z;

                const int dist = std::max(std::abs(x), std::abs(z));
                const bool is_edge = dist > radius;

                bool found = false;
                // check if it exists
                for (int i = 0; i < world.size(); i++) {
                    auto chunk = &world[i];
                    if (chunk->chunk_x == chunk_x && chunk->chunk_z == chunk_z) {
                        chunk->is_edge = is_edge;
                        if (!chunk->has_terrain) {
                            Generate_terrain(chunk_x, chunk_z, chunk);
                            Generate_mesh(chunk);
                        }
                        if (chunk->has_terrain && !chunk->is_edge && !chunk->has_mesh) {
                        }
                        found = true;
                        break;
                    }
                }
                if (found)
                    continue;

                Chunk *chunk = new Chunk;
                chunk->is_edge = is_edge;

                Generate_terrain(chunk_x, chunk_z, chunk);
                //if (!is_edge)
                    Generate_mesh(chunk);
                world.push_back(*chunk);
                //delete chunk;
            }
        }
    }
}
