#include "mesh.hpp"

#include "world.hpp"

namespace MyCraft {
    void Generate_mesh(Chunk *chunk) {
        const float WorldOffsetX = chunk->chunk_x * Chunk::width;
        const float WorldOffsetZ = chunk->chunk_z * Chunk::depth;

        const Chunk* cxp = World::chunk_find(WorldOffsetX+1, WorldOffsetZ);
        const Chunk* cxn = World::chunk_find(WorldOffsetX-1, WorldOffsetZ);
        const Chunk* czp = World::chunk_find(WorldOffsetX, WorldOffsetZ+1);
        const Chunk* czn = World::chunk_find(WorldOffsetX, WorldOffsetZ-1);

        for (int x = 0; x < Chunk::width; x++) {
            for (int z = 0; z < Chunk::depth; z++) {
                for (int y = 0; y < Chunk::height; y++) {
                    const auto block = chunk->get(x, y, z);
                    if (block.id != 0) {
                        const glm::vec3 w = {WorldOffsetX + x, static_cast<float>(y), WorldOffsetZ + z};

                        // X+
                        if (x+1 < Chunk::width) {
                            if (chunk->get(x+1, y, z).id == 0)
                                FaceX(chunk, w, 1);
                        } else if (cxp && cxp->get(0, y, z).id == 0)
                            FaceX(chunk, w, 1);

                        // X-
                        if (x-1 > 0) {
                            if (chunk->get(x-1, y, z).id == 0)
                                FaceX(chunk, w, -1);
                        } else if (cxn && cxn->get(Chunk::width-1, y, z).id == 0)
                            FaceX(chunk, w, -1);

                        // Y+/-
                        if (y+1 >= Chunk::height || chunk->get(x, y+1, z).id == 0)
                            FaceY(chunk, w, 1);
                        if (y+1 < 0 || chunk->get(x, y-1, z).id == 0)
                            FaceY(chunk, w, -1);

                        // Z+
                        if (z+1 < Chunk::depth) {
                            if (chunk->get(x, y, z+1).id == 0)
                                FaceZ(chunk, w, 1);
                        } else if (czp && czp->get(x, y, 0).id == 0)
                            FaceZ(chunk, w, 1);

                        // Z-
                        if (z-1 > 0) {
                            if (chunk->get(x, y, z-1).id == 0)
                                FaceZ(chunk, w, -1);
                        } else if (czn && czn->get(x, y, Chunk::depth-1).id == 0)
                            FaceZ(chunk, w, -1);
                    }
                }
            }
        }
    }

    void FaceX(Chunk *chunk, const glm::vec3& w, const int dir) {
        constexpr float size = 1.0f;
        const float color = dir == 1 ? 1.0f : 0.5f;
        const float z1 = w.z;
        const float y1 = w.y;
        const float z2 = w.z + size;
        const float y2 = w.y + size;

        const float x = w.x + (dir > 0 ? size : 0.0f); // X+/X-

        auto push = [&](const float x_, const float y_, const float z_) {
            Chunk::Vertex v;
            v.pos = glm::vec3(x_, y_, z_);
            v.color = glm::vec3(color, 0.0f, 0.0f);
            chunk->mesh.push_back(v);
        };

        // First Triangle
        push(x, y1, z1);
        push(x, y1, z2);
        push(x, y2, z2);

        // Second Triangle
        push(x, y1, z1);
        push(x, y2, z2);
        push(x, y2, z1);
    }

    void FaceY(Chunk *chunk, const glm::vec3& w, const int dir) {
        constexpr float size = 1.0f;
        const float color = dir == 1 ? 1.0f : 0.5f;
        const float x1 = w.x;
        const float z1 = w.z;
        const float x2 = w.x + size;
        const float z2 = w.z + size;

        const float y = w.y + (dir > 0 ? size : 0.0f); // Y+/Y-

        auto push = [&](const float x_, const float y_, const float z_) {
            Chunk::Vertex v;
            v.pos = glm::vec3(x_, y_, z_);
            v.color = glm::vec3(0.0f, color, 0.0f);
            chunk->mesh.push_back(v);
        };

        // First Triangle
        push(x1, y, z1);
        push(x2, y, z1);
        push(x2, y, z2);

        // Second Triangle
        push(x1, y, z1);
        push(x2, y, z2);
        push(x1, y, z2);
    }

    void FaceZ(Chunk *chunk, const glm::vec3& w, const int dir) {
        constexpr float size = 1.0f;
        const float color = dir == 1 ? 1.0f : 0.5f;
        const float x1 = w.x;
        const float y1 = w.y;
        const float x2 = w.x + size;
        const float y2 = w.y + size;

        const float z = w.z + (dir > 0 ? size : 0.0f); // Z+/Z-

        auto push = [&](const float x_, const float y_, const float z_) {
            Chunk::Vertex v;
            v.pos = glm::vec3(x_, y_, z_);
            v.color = glm::vec3(0.0f, 0.0f, color);
            chunk->mesh.push_back(v);
        };

        // First Triangle
        push(x1, y1, z);
        push(x2, y1, z);
        push(x2, y2, z);

        // Second Triangle
        push(x1, y1, z);
        push(x2, y2, z);
        push(x1, y2, z);
    }
}
