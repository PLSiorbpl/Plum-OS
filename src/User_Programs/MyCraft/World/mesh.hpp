#pragma once
#include "chunk.hpp"
#include "std/math_types.hpp"

namespace MyCraft {
    void Generate_mesh(Chunk *chunk);

    void FaceX(Chunk *chunk, const glm::vec3& w, int dir);
    void FaceY(Chunk *chunk, const glm::vec3& w, int dir);
    void FaceZ(Chunk *chunk, const glm::vec3& w, int dir);
}
