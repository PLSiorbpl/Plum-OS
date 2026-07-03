#pragma once
#include "Drivers/GPU/OpenPL/OpenPL.hpp"

namespace MyCraft {
    extern OpenPL::Framebuffer framebuffer;
    extern OpenPL::Pipeline pipeline;
    extern OpenPL::Context ctx;

    void vshader(const OpenPL::Shader::VS_ShaderIn *In, OpenPL::Shader::VS_ShaderOut *out, void *uniform);
    bool frshader(const OpenPL::Shader::FR_ShaderIN *In, OpenPL::Shader::FS_ShaderOut *out, void *uniform);

    struct Uniforms {
        glm::vec3 cam; // Camera
    };
    extern Uniforms uni;

    void main(int argc, char** argv);
    void setup_window(int w, int h);
    void exit();
}
