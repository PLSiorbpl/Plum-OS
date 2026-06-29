#pragma once
#include "../main.hpp"
#include "std/types.hpp"
#include "std/vector.hpp"
#include "std/math_types.hpp"

namespace MyCraft {
    class Chunk {
    public:
        Chunk() = default;

        struct Block {
            uint8_t id = 0;
            // Transparent
            uint8_t flags = 0;

            Block(const uint8_t id = 0, const uint8_t flags = 0) : id(id), flags(flags) {}
        };

        static constexpr uint8_t width = 8;
        static constexpr uint8_t height = 4;
        static constexpr uint8_t depth = 8;
        static constexpr uint32_t SIZE = 16*16*16;
        Block blocks[SIZE] = {};

        int chunk_x = 0;
        int chunk_z = 0;
        bool is_edge = false;
        bool has_terrain = false;
        bool has_mesh = false;

        struct Vertex {
            glm::vec3 pos = {};
            glm::vec3 color = {};
        };
        std::vector<Vertex> mesh;

        static int index(const int x, const int y, const int z) {
            /*assert(x >= 0 && x < width);
            assert(y >= 0 && y < height);
            assert(z >= 0 && z < depth);*/

            const int idx = x + z * width + y * width * depth;
            //assert(idx >= 0 && idx < (int)blocks.size());
            return idx;
        }

        [[nodiscard]] const Block& get(const int x, const int y, const int z) const noexcept {
            return blocks[index(x, y, z)];
        }

        void set(const int x, const int y, const int z, const Block& block) {
            blocks[index(x, y, z)] = block;
        }

        void Draw() {
            ctx.bind_vertex_buffer(reinterpret_cast<uint8_t *>(mesh.m_data), mesh.size());
            ctx.set_vertex_attr_type(0, OpenPL::AttributeType::ATTR_VEC3); // Position
            ctx.set_vertex_attr_type(1, OpenPL::AttributeType::ATTR_VEC3); // Color

            ctx.Draw(OpenPL::PrimitiveType::TRIANGLES, 0, mesh.size());
        }
    };
}
