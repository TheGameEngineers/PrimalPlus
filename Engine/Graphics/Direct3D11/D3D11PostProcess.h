#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
struct d3d11_frame_info;
}

namespace primal::graphics::d3d11::fx {
bool initialize();
void shutdown();

void post_process(ID3D11DeviceContext4* ctx, const d3d11_frame_info& d3d11_info, ID3D11RenderTargetView* target_rtv);
}

#endif
