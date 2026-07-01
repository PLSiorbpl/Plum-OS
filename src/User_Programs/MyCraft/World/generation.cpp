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
                for (auto &chunk : world) {
                    if (chunk.chunk_x == chunk_x && chunk.chunk_z == chunk_z) {
                        chunk.is_edge = is_edge;
                        if (!chunk.has_terrain) {
                            Generate_terrain(chunk_x, chunk_z, &chunk);
                            chunk.has_terrain = true;
                        }
                        if (chunk.has_terrain && !chunk.is_edge && !chunk.has_mesh) {
                            Generate_mesh(&chunk);
                            chunk.has_mesh = true;
                        }
                        found = true;
                        break;
                    }
                }
                if (found)
                    continue;

                Chunk chunk = {};
                chunk.is_edge = is_edge;

                Generate_terrain(chunk_x, chunk_z, &chunk);
                chunk.has_terrain = true;
                if (!is_edge) {
                    Generate_mesh(&chunk);
                    chunk.has_mesh = true;
                }
                world.push_back(std::move(chunk));
            }
        }
    }

    void RemoveChunks(const glm::ivec2 center, const int radius) {
        for (int i = 0; i < world.size();) {
            const auto &chunk = world[i];
            const int Chunk_X = chunk.chunk_x;
            const int Chunk_Z = chunk.chunk_z;

            const int dX = Chunk_X - center.x;
            const int dZ = Chunk_Z - center.y;

            const int dist = std::max(std::abs(dX), std::abs(dZ));

            if (dist > radius+1) {
                world[i].mesh.clear();
                world[i] = std::move(world.back());
                world.pop_back();
            } else {
                i++;
            }
        }
    }
}
