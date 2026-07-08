#pragma once
#include "../main.hpp"
#include "std/types.hpp"
#include "std/vector.hpp"
#include "std/math_types.hpp"

namespace MyCraft {
    enum class Type : uint8_t {
        Air = 0,
        Grass = 1,
        Stone = 2,
    };

    struct Block {
        Type type = Type::Air;

        explicit Block(const Type type = Type::Air) : type(type) {}
    };

    struct Vertex {
        glm::vec3 pos = {};
        glm::vec2 uv = {};
    };

    class Chunk {
    public:
        Chunk() = default;

        static constexpr uint8_t width = 16;
        static constexpr uint8_t height = 48;
        static constexpr uint8_t depth = 16;
        static constexpr uint32_t SIZE = width*height*depth;
        Block blocks[SIZE];

        int chunk_x = 0;
        int chunk_z = 0;
        bool is_edge = false;
        bool has_terrain = false;
        bool has_mesh = false;

        std::vector<Vertex> mesh;

        static int index(const int x, const int y, const int z) {
            const int idx = x + z * width + y * width * depth;
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

            ctx.Draw(OpenPL::PrimitiveType::TRIANGLES, 0, mesh.size());
        }
    };
}
