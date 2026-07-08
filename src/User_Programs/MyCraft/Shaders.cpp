#include "main.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "std/math_types.hpp"
#include "Assets/textures.hpp"

namespace MyCraft {
    using namespace OpenPL;

    void vshader(const Shader::VS_ShaderIn *In, Shader::VS_ShaderOut *out, void *uniform) {
        const auto uni_ = *static_cast<Uniforms *>(uniform);
        const auto pos = *reinterpret_cast<glm::vec3 *>(In->attributes[0].data);
        const auto uv = *reinterpret_cast<glm::vec2 *>(In->attributes[1].data);

        const auto p = pos + uni_.cam;

        out->position = p;
        out->inv_w = 1.0f / p.z;

        out->varyings[0] = out->inv_w;
        out->varyings[1] = uv.x / p.z;
        out->varyings[2] = uv.y / p.z;

        out->used_mask = 0b111;
        out->flat_mask = ~0b111;
    }

    bool frshader(const Shader::FR_ShaderIN *In, Shader::FS_ShaderOut *out, void *uniform) {
        const float w = 1.0f / In->varyings[0];
        const glm::vec2 uv = {In->varyings[1]*w, In->varyings[2]*w};

        if (uv.x < 2.0f)
            out->color = T_grass_top.geti(uv.x, uv.y);
        else
            out->color = T_grass_side.geti(uv.x-2.0f, uv.y-2.0f);

        return true;
    }
}
