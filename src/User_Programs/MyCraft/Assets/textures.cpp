#include "textures.hpp"

#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/texture.hpp"

namespace MyCraft {
    using namespace OpenPL;

    Texture2D T_grass_side = {};
    Texture2D T_grass_top = {};

    void Texture_init() {
        T_grass_top.texture = (uint8_t *)grass_top;
        T_grass_top.width = grass_top_size;
        T_grass_top.height = grass_top_size;

        T_grass_side.texture = (uint8_t *)grass_side;
        T_grass_side.width = grass_side_size;
        T_grass_side.height = grass_side_size;
    }
}
