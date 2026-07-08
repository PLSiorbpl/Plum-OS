#include "generation.hpp"

#include "std/math.hpp"
#include "std/math_types.hpp"
#include "world.hpp"
#include "terrain.hpp"
#include "Drivers/hpet/hpet.h"

namespace MyCraft {
    void GenerateChunks(const glm::ivec2 center, const int radius) {
        for (int x = -radius-1; x <= radius+1; x++) {
            for (int z = -radius-1; z <= radius+1; z++) {
                const int chunk_x = center.x + x;
                const int chunk_z = center.y + z;

                const int dist = std::max(std::abs(x), std::abs(z));
                const bool is_edge = dist > radius;

                {
                    auto chunk = World::chunk_find(chunk_x, chunk_z);
                    if (chunk != nullptr) {
                        chunk->is_edge = is_edge;
                        if (!chunk->has_terrain) {
                            Generate_terrain(chunk);
                        }
                        if (chunk->has_mesh && chunk->is_edge) {
                            chunk->mesh.release();
                            chunk->has_mesh = false;
                        }
                        continue;
                    }
                }

                Chunk chunk;
                chunk.is_edge = is_edge;
                chunk.chunk_x = chunk_x;
                chunk.chunk_z = chunk_z;

                Generate_terrain(&chunk);

                World::world.push_back(std::move(chunk));
            }
        }
    }

    void RemoveChunks(const glm::ivec2 center, const int radius) {
        for (int i = 0; i < World::world.size();) {
            const auto &chunk = World::world[i];
            const int Chunk_X = chunk.chunk_x;
            const int Chunk_Z = chunk.chunk_z;

            const int dX = Chunk_X - center.x;
            const int dZ = Chunk_Z - center.y;

            const int dist = std::max(std::abs(dX), std::abs(dZ));

            if (dist > radius+1) {
                World::world[i].mesh.clear();
                World::world[i] = std::move(World::world.back());
                World::world.pop_back();
            } else {
                i++;
            }
        }
    }
}
