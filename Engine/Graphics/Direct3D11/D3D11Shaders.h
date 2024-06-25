#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::shaders {
struct engine_shader {
    enum id : u32 {
        fullscreen_triangle_vs = 0,
        fill_color_ps = 1,
        post_process_ps = 2,
        grid_frustums_cs = 3,
        light_culling_cs = 4,
        count
    };
};

bool initialize();
void shutdown();
void* get_engine_shader(engine_shader::id id);
}

#endif
