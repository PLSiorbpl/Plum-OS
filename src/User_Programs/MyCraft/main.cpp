#include "main.hpp"
#include "User_Programs/Chess/main.hpp"

#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "kernel/Memory/heap.hpp"
#include "arch/x86_64/syscall/syscall.h"
#include "Assets/textures.hpp"
#include "std/math.hpp"
#include "std/printf.hpp"
#include "World/chunk.hpp"
#include "World/generation.hpp"
#include "World/mesh.hpp"
#include "World/world.hpp"
#include "std/trigonometry.hpp"
#include "std/math_functions.hpp"
#include "Drivers/Network/Sockets/socket.hpp"
#include "Drivers/USB/mouse.hpp"

namespace MyCraft {
    using namespace OpenPL;

    Framebuffer framebuffer = {};
    Pipeline pipeline = {};
    Context ctx = {};
    Uniforms uni = {};

    Camera camera;

    void main(const int argc, char** argv) {
        setup_window(480, 360);
        Texture_init();

        camera.pos = {0, 10, 0};
        camera.l_chunk = {9999, 9999};
        camera.Sensitivity = 0.5f;

        while (true) {
            const kb::key_code kb_key = sys_get_key(false);

            if (camera.chunk != camera.l_chunk) {
                GenerateChunks(camera.chunk, 2);
                RemoveChunks(camera.chunk, 2);
                for (auto& [h, key, chunk] : World::world) {
                    if (chunk.has_mesh || !chunk.has_terrain || chunk.is_edge) continue;
                    Generate_mesh(&chunk);
                }
                camera.l_chunk = camera.chunk;
            }


            if (kb_key == kb::key_code::KEY_ESC) {
                exit();
                return;
            }
            if (kb_key == kb::key_code::KEY_A)
                camera.pos.z -= 0.5f;
            if (kb_key == kb::key_code::KEY_D)
                camera.pos.z += 0.5f;
            if (kb_key == kb::key_code::KEY_S)
                camera.pos.x -= 0.5f;
            if (kb_key == kb::key_code::KEY_W)
                camera.pos.x += 0.5f;
            if (kb_key == kb::key_code::KEY_Q)
                camera.pos.y -= 0.5f;
            if (kb_key == kb::key_code::KEY_E)
                camera.pos.y += 0.5f;
            camera.chunk.x = std::floor(camera.pos.x / static_cast<float>(Chunk::width));
            camera.chunk.y = std::floor(camera.pos.z / static_cast<float>(Chunk::depth));

            const auto model = glm::mat4x4(1.0f);
            uni.model_matrix = model;
            update_camera(USB::mouse_state.x, USB::mouse_state.y);
            USB::mouse_state.x = 0; USB::mouse_state.y = 0;
            constexpr float aspect = 480.0f / 360.0f;
            uni.proj_matrix = glm::perspective(120.0f, aspect, 0.1f, 100.0f);

            uni.MVP = uni.proj_matrix * uni.view_matrix * uni.model_matrix;

            ctx.Clear(0x87CEFA);

            ctx.set_uniform_ptr(reinterpret_cast<uint8_t *>(&uni));
            ctx.set_vertex_attr_type(0, AttributeType::ATTR_VEC3); // Position
            ctx.set_vertex_attr_type(1, AttributeType::ATTR_VEC2); // Color
            for (auto& [h, key, chunk] : World::world) {
                chunk.Draw();
            }

            sys_openPL(&ctx, GL_SWAP);
        }
    }

    void update_camera(float xpos, float ypos) {
        float xoffset = xpos;
        float yoffset = -ypos;

        xoffset *= camera.Sensitivity;
        yoffset *= camera.Sensitivity;

        camera.yaw   += xoffset;
        camera.pitch += yoffset;

        camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);

        const auto direction = glm::vec3(
            std::cos(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch)),
            std::sin(glm::radians(camera.pitch)),
            std::sin(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch))
        );

        const glm::vec3 front = glm::normalize(direction);
        const glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));
        const glm::vec3 up    = glm::normalize(glm::cross(right, front));

        uni.view_matrix = glm::look_at(camera.pos, camera.pos + front, up);
    }

    void setup_window(const int w, const int h) {
        // Now we can create a pipeline
        pipeline.Vertex_shader = vshader;
        pipeline.Fragment_shader = frshader;
        pipeline.near_plane = 0.05f;
        pipeline.far_plane = 1000.0f;
        pipeline.cull_mode = CullingMode::NONE;
        ctx.bind_pipeline(pipeline);

        // Creating Framebuffer and Depthbuffer
        framebuffer.bpp = 32;
        framebuffer.width = w;
        framebuffer.height = h;
        auto *raw_framebuffer = static_cast<uint32_t *>(heap::malloc(w * h * (framebuffer.bpp/8)));
        auto *raw_depthbuffer = static_cast<float *>(heap::malloc(w * h * sizeof(float)));
        if (raw_framebuffer == nullptr || raw_depthbuffer == nullptr) {
            heap::free(raw_framebuffer);
            heap::free(raw_depthbuffer);
            return;
        }
        framebuffer.framebuffer = raw_framebuffer;
        framebuffer.depthbuffer = raw_depthbuffer;
        ctx.bind_framebuffer(framebuffer);
    }

    void exit() {
        std::printf("&cExiting &fMyCraft\n");
        std::printf("\t&cDeleting Chunks\n");
        for (auto & entry : World::world)
            entry.value.mesh.release();
        World::world.clear();
        std::printf("\t&cDeleting OpenPL Context\n");
        heap::free(framebuffer.framebuffer);
        heap::free(framebuffer.depthbuffer);
        ctx.Delete_ctx();
    }
}
