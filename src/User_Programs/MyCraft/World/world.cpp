#include "world.hpp"

#include "std/vector.hpp"

namespace MyCraft {
    namespace World {
        std::hash_map<glm::ivec2, Chunk, hash::hash<glm::ivec2>> world;

        Chunk *chunk_find(const int chunkX, const int chunkZ) {
            //for (auto &c : world) {
            //    if (c.chunk_x == chunkX && c.chunk_z == chunkZ) {
            //        return &c;
            //    }
            //}
            return world.find({chunkX, chunkZ});
        }
    }
}
