#include "main.hpp"
#include "User_Programs/Chess/main.hpp"

#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "kernel/Memory/heap.hpp"
#include "arch/x86_64/syscall/syscall.h"
#include "std/math.hpp"
#include "std/printf.hpp"
#include "World/chunk.hpp"
#include "World/generation.hpp"
#include "World/world.hpp"

namespace MyCraft {
    using namespace OpenPL;

    Framebuffer framebuffer = {};
    Pipeline pipeline = {};
    Context ctx = {};
    Uniforms uni = {};

    void main(const int argc, char** argv) {
        // One of legacy resolutions
        setup_window(480, 360);

        uni.cam = {0, -2.0f, 0};

        glm::ivec2 cam_chunk{};
        auto last_cam_chunk = glm::ivec2(9999);
        auto old_cam = glm::vec3(9999);

        while (true) {
            kb::key_code key = sys_get_key(false);

            if (cam_chunk != last_cam_chunk) {
                GenerateChunks(cam_chunk, 4);
                RemoveChunks(cam_chunk, 4);
                last_cam_chunk = cam_chunk;
            }


            if (key == kb::key_code::KEY_ESC) {
                exit();
                return;
            }
            if (key == kb::key_code::KEY_A)
                uni.cam.x += 0.1f;
            if (key == kb::key_code::KEY_D)
                uni.cam.x -= 0.1f;
            if (key == kb::key_code::KEY_S)
                uni.cam.z += 0.1f;
            if (key == kb::key_code::KEY_W)
                uni.cam.z -= 0.1f;
            if (key == kb::key_code::KEY_Q)
                uni.cam.y += 0.1f;
            if (key == kb::key_code::KEY_E)
                uni.cam.y -= 0.1f;
            cam_chunk.x = -std::floor(uni.cam.x / static_cast<float>(Chunk::width));
            cam_chunk.y = -std::floor(uni.cam.z / static_cast<float>(Chunk::depth));

            ctx.Clear(0x303030);

            ctx.set_uniform_ptr(reinterpret_cast<uint8_t *>(&uni));
            ctx.set_vertex_attr_type(0, AttributeType::ATTR_VEC3); // Position
            ctx.set_vertex_attr_type(1, AttributeType::ATTR_VEC3); // Color
            for (auto &chunk : world) {
                chunk.Draw();
            }

            sys_openPL(&ctx, GL_SWAP);
        }
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
        std::print("&cExiting &fMyCraft\n");
        std::print("\t&cDeleting Chunks\n");
        while (!world.empty()) {
            auto &chunk = world.back();
            chunk.mesh.clear();
            world.pop_back();
        }
        world.release();
        std::print("\t&cDeleting OpenPL Context\n");
        heap::free(framebuffer.framebuffer);
        heap::free(framebuffer.depthbuffer);
        ctx.Delete_ctx();
    }
}
