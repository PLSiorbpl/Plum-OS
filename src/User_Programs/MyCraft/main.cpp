#include "main.hpp"
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

        world.reserve(256);

        uni.cam = {0, -2.0f, 0};

        glm::ivec2 cam_chunk{};
        auto last_cam_chunk = glm::ivec2(9999);
        auto old_cam = glm::vec3(9999);

        while (true) {
            kb::key_code key = sys_get_key(false);

            if (cam_chunk != last_cam_chunk) {
                GenerateChunks(cam_chunk, 4);
                last_cam_chunk = cam_chunk;
            }


            if (key == kb::key_code::KEY_ESC) {
                std::printf("Exiting MyCraft\n");
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
            cam_chunk.x = std::floor(-uni.cam.x / 8.0f);
            cam_chunk.y = std::floor(-uni.cam.z / 8.0f);

            ctx.Clear(0x303030);

            ctx.set_uniform_ptr(reinterpret_cast<uint8_t *>(&uni)); // We have to do this every time something changes in uniforms
            for (auto &chunk : world) {
                chunk.Draw();
            }

            sys_openPL(&ctx, GL_SWAP);
        }
    }

    void setup_window(int w, int h) {
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
        auto *raw_depthkbuffer = static_cast<float *>(heap::malloc(w * h * sizeof(float)));
        if (raw_framebuffer == nullptr || raw_depthkbuffer == nullptr) {
            heap::free(raw_framebuffer);
            heap::free(raw_depthkbuffer);
            return;
        }
        framebuffer.framebuffer = raw_framebuffer;
        framebuffer.depthbuffer = raw_depthkbuffer;
        ctx.bind_framebuffer(framebuffer);
    }
}
