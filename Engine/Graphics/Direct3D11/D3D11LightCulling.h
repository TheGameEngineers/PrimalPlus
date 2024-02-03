#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
struct d3d11_frame_info;
}

namespace primal::graphics::d3d11::lightculling {
constexpr u32 light_culling_tile_size{ 32 };

bool initialize();
void shutdown();

_NODISCARD id::id_type add_culler();
void remove_culler(id::id_type id);

void cull_lights(ID3D11DeviceContext4* const ctx,
	const d3d11_frame_info& d3d11_info);

//REMOVE!!!
ID3D11ShaderResourceView* frustums(id::id_type light_culling_id, u32 frame_index);
ID3D11ShaderResourceView* light_grid_opaque(id::id_type light_culling_id, u32 frame_index);
ID3D11ShaderResourceView* light_index_list_opaque(id::id_type light_culling_id, u32 frame_index);
}

#endif
