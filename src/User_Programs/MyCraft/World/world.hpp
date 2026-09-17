#pragma once
#include "chunk.hpp"
#include "std/vector.hpp"
#include "std/hash_map.hpp"
#include "std/hash.hpp"

namespace MyCraft {
    namespace World {
        extern std::hash_map<glm::ivec2, Chunk, hash::hash<glm::ivec2>> world;
        //extern std::vector<Chunk> world;

        Chunk *chunk_find(int chunkX, int chunkZ);
    }
}