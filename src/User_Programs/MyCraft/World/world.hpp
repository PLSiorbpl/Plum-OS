#pragma once
#include "chunk.hpp"
#include "std/vector.hpp"

namespace MyCraft {
    namespace World {
        extern std::vector<Chunk> world;

        Chunk *chunk_find(int chunkX, int chunkZ);
    }
}