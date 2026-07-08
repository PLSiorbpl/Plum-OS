#include "mesh.hpp"

#include "world.hpp"

namespace MyCraft {
    void Generate_mesh(Chunk *chunk) {
        const int WorldOffsetX = chunk->chunk_x * Chunk::width;
        const int WorldOffsetZ = chunk->chunk_z * Chunk::depth;

        const Chunk* cxp = World::chunk_find(chunk->chunk_x+1, chunk->chunk_z);
        const Chunk* cxn = World::chunk_find(chunk->chunk_x-1, chunk->chunk_z);
        const Chunk* czp = World::chunk_find(chunk->chunk_x, chunk->chunk_z+1);
        const Chunk* czn = World::chunk_find(chunk->chunk_x, chunk->chunk_z-1);

        if (!cxp || !cxn || !czp || !czn) return;

        for (int x = 0; x < Chunk::width; x++) {
            for (int z = 0; z < Chunk::depth; z++) {
                for (int y = 0; y < Chunk::height; y++) {
                    const auto block = chunk->get(x, y, z);
                    if (block.type != Type::Air) {
                        const glm::vec3 w = {static_cast<float>(WorldOffsetX + x), static_cast<float>(y), static_cast<float>(WorldOffsetZ + z)};

                        // X+
                        if (x+1 < Chunk::width) {
                            if (chunk->get(x+1, y, z).type == Type::Air)
                                FaceX(chunk, w, 1);
                        } else if (cxp && cxp->get(0, y, z).type == Type::Air)
                            FaceX(chunk, w, 1);

                        // X-
                        if (x-1 >= 0) {
                            if (chunk->get(x-1, y, z).type == Type::Air)
                                FaceX(chunk, w, -1);
                        } else if (cxn && cxn->get(Chunk::width-1, y, z).type == Type::Air)
                            FaceX(chunk, w, -1);

                        // Y+/-
                        if (y+1 >= Chunk::height || chunk->get(x, y+1, z).type == Type::Air)
                            FaceY(chunk, w, 1);
                        if (y-1 < 0 || chunk->get(x, y-1, z).type == Type::Air)
                            FaceY(chunk, w, -1);

                        // Z+
                        if (z+1 < Chunk::depth) {
                            if (chunk->get(x, y, z+1).type == Type::Air)
                                FaceZ(chunk, w, 1);
                        } else if (czp && czp->get(x, y, 0).type == Type::Air)
                            FaceZ(chunk, w, 1);

                        // Z-
                        if (z-1 >= 0) {
                            if (chunk->get(x, y, z-1).type == Type::Air)
                                FaceZ(chunk, w, -1);
                        } else if (czn && czn->get(x, y, Chunk::depth-1).type == Type::Air)
                            FaceZ(chunk, w, -1);
                    }
                }
            }
        }
        chunk->has_mesh = true;
    }

    void FaceX(Chunk *chunk, const glm::vec3& w, const int dir) {
        constexpr float size = 1.0f;
        const float z1 = w.z;
        const float y1 = w.y;
        const float z2 = w.z + size;
        const float y2 = w.y + size;

        const float x = w.x + (dir > 0 ? size : 0.0f); // X+/X-

        auto push = [&](const float x_, const float y_, const float z_, const float uv_x, const float uv_y) {
            Vertex v;
            v.pos = glm::vec3(x_, y_, z_);
            v.uv = glm::vec2(uv_x+2.0f, uv_y+2.0f);
            chunk->mesh.push_back(v);
        };

        // First Triangle
        push(x, y1, z1, 1.0f, 1.0f);
        push(x, y2, z2, 0.0f, 0.0f);
        push(x, y1, z2, 0.0f, 1.0f);

        // Second Triangle
        push(x, y1, z1, 1.0f, 1.0f);
        push(x, y2, z1, 1.0f, 0.0f);
        push(x, y2, z2, 0.0f, 0.0f);
    }

    void FaceY(Chunk *chunk, const glm::vec3& w, const int dir) {
        constexpr float size = 1.0f;
        const float x1 = w.x;
        const float z1 = w.z;
        const float x2 = w.x + size;
        const float z2 = w.z + size;

        const float y = w.y + (dir > 0 ? size : 0.0f); // Y+/Y-

        auto push = [&](const float x_, const float y_, const float z_, const float uv_x, const float uv_y) {
            Vertex v;
            v.pos = glm::vec3(x_, y_, z_);
            v.uv = glm::vec2(uv_x, uv_y);
            chunk->mesh.push_back(v);
        };

        // First Triangle
        push(x1, y, z1, 0.0f, 0.0f);
        push(x2, y, z1, 1.0f, 0.0f);
        push(x2, y, z2, 1.0f, 1.0f);

        // Second Triangle
        push(x1, y, z1, 0.0f, 0.0f);
        push(x1, y, z2, 0.0f, 1.0f);
        push(x2, y, z2, 1.0f, 1.0f);
    }

    void FaceZ(Chunk *chunk, const glm::vec3& w, const int dir) {
        constexpr float size = 1.0f;
        const float x1 = w.x;
        const float y1 = w.y;
        const float x2 = w.x + size;
        const float y2 = w.y + size;

        const float z = w.z + (dir > 0 ? size : 0.0f); // Z+/Z-

        auto push = [&](const float x_, const float y_, const float z_, const float uv_x, const float uv_y) {
            Vertex v;
            v.pos = glm::vec3(x_, y_, z_);
            v.uv = glm::vec2(uv_x+2.0f, uv_y+2.0f);
            chunk->mesh.push_back(v);
        };

        // First Triangle
        push(x1, y1, z, 1.0f, 1.0f);
        push(x2, y1, z, 0.0f, 1.0f);
        push(x2, y2, z, 0.0f, 0.0f);

        // Second Triangle
        push(x1, y1, z, 1.0f, 1.0f);
        push(x1, y2, z, 1.0f, 0.0f);
        push(x2, y2, z, 0.0f, 0.0f);
    }
}
