#pragma once
#include "chunk.hpp"
#include "std/noise.hpp"

namespace MyCraft {
    extern std::perlin2d rand;

    void Generate_terrain(Chunk *chunk);
}