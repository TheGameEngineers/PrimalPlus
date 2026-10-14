#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::shaders {
struct engine_shader {
    enum id : u32 {
        fullscreen_triangle_vs = 0,
        post_process_ps,
        grid_frustums_cs,
        light_culling_cs,
        count
    };
};

bool initialize();
void shutdown();
void* get_engine_shader(engine_shader::id id);
}

#endif
