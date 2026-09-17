#pragma once
#include "Drivers/GPU/OpenPL/OpenPL.hpp"

namespace MyCraft {
    extern OpenPL::Framebuffer framebuffer;
    extern OpenPL::Pipeline pipeline;
    extern OpenPL::Context ctx;

    void vshader(const OpenPL::Shader::VS_ShaderIn *In, OpenPL::Shader::VS_ShaderOut *out, void *uniform);
    bool frshader(const OpenPL::Shader::FR_ShaderIN *In, OpenPL::Shader::FS_ShaderOut *out, void *uniform);

    struct Uniforms {
        glm::mat4x4 model_matrix;
        glm::mat4x4 view_matrix;
        glm::mat4x4 proj_matrix;
        glm::mat4x4 MVP;
    };
    extern Uniforms uni;

    struct Camera {
        glm::ivec2 l_chunk = glm::ivec2(99999);
        glm::ivec2 chunk = glm::ivec2(0);
        glm::vec3 pos = glm::vec3(0.0f, 15.0f, 0.0f);
        float yaw = -45.0f;
        float pitch = -45.0f;
        float Sensitivity = 0.1f;
    };

    extern Camera camera;

    void main(int argc, char** argv);
    void setup_window(int w, int h);
    void update_camera(float xpos, float ypos);
    void exit();
}
