#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
struct d3d11_frame_info;
}

namespace primal::graphics::d3d11::light {
bool initialize();
void shutdown();

void create_light_set(u64 key);
void remove_light_set(u64 key);
graphics::light create(light_init_info info);
void remove(light_id id, u64 light_set_key);
void set_parameter(light_id id, u64 light_set_key, light_parameter::parameter parameter, const void* const data, u32 data_size);
void get_parameter(light_id id, u64 light_set_key, light_parameter::parameter parameter, void* const data, u32 data_size);

void update_light_buffers(const d3d11_frame_info& d3d11_info, ID3D11DeviceContext4* const ctx);
ID3D11ShaderResourceView* const non_cullable_light_buffer(u32 frame_index);
ID3D11ShaderResourceView* const cullable_light_buffer(u32 frame_index);
ID3D11ShaderResourceView* const culling_info_buffer(u32 frame_index);
ID3D11ShaderResourceView* const bounding_spheres_buffer(u32 frame_index);
u32 non_cullable_light_count(u64 light_set_key);
u32 cullable_light_count(u64 light_set_key);
}

#endif
