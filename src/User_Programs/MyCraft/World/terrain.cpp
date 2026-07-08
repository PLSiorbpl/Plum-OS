#include "terrain.hpp"
#include "chunk.hpp"
#include "std/noise.hpp"

namespace MyCraft {
    std::perlin2d rand(127);

    void Generate_terrain(Chunk *chunk) {
        const auto chunk_x = chunk->chunk_x;
        const auto chunk_z = chunk->chunk_z;

        for (int x = 0; x < chunk->width; x++) {
            for (int z = 0; z < chunk->depth; z++) {

                const float world_x = chunk_x * Chunk::width + x;
                const float world_z = chunk_z * Chunk::depth + z;

                constexpr float scale = 0.05f;
                const int y = Chunk::height * rand.fget(world_x * scale, world_z * scale);

                for (int i = 0; i <= y; i++) {
                    chunk->set(x, i, z, Block(Type::Grass));
                }
            }
        }
        chunk->has_terrain = true;
    }
}
