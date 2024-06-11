#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
struct d3d11_frame_info;
}

namespace primal::graphics::d3d11::gpass {
constexpr DXGI_FORMAT               main_buffer_format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
constexpr DXGI_FORMAT               depth_buffer_format{ DXGI_FORMAT_D32_FLOAT };

bool initialize();
void shutdown();

_NODISCARD d3d11_render_texture& main_buffer();
_NODISCARD d3d11_depth_buffer& depth_buffer();

void set_size(math::u32v2 size);
void depth_prepass(ID3D11DeviceContext4* ctx, const d3d11_frame_info& d3d11_info);
void render(ID3D11DeviceContext4* ctx, const d3d11_frame_info& d3d11_info);
void set_render_targets_for_depth_prepass(ID3D11DeviceContext4* ctx);
void set_render_targets_for_gpass(ID3D11DeviceContext4* ctx);
}

#endif
