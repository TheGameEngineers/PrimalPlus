#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
namespace camera { class d3d11_camera; }
struct d3d11_frame_info
{
    const frame_info*				info{ nullptr };
    camera::d3d11_camera*			camera{ nullptr };
    u32								global_shader_data_offset{ 0 };
    u32								surface_width{ 0 };
    u32								surface_height{ 0 };
    id::id_type						light_culling_id{ id::invalid_id };
    u32								frame_index{ 0 };
    f32								delta_time{ 16.7f };
};
}

namespace primal::graphics::d3d11::core {
bool initialize();
void shutdown();

template<typename T>
constexpr void release(T*& resource)
{
    if (resource)
    {
        resource->Release();
        resource = nullptr;
    }
}

namespace detail {
void deferred_release(IUnknown* resource);
}

template<typename T>
constexpr void deferred_release(T*& resource)
{
    if (resource)
    {
        detail::deferred_release(resource);
        resource = nullptr;
    }
}

_NODISCARD ID3D11Device5* const device();
_NODISCARD ID3D11DeviceContext4* const imm_context();
_NODISCARD constant_buffer& cbuffer();
_NODISCARD u32 current_frame_index();
void set_deferred_releases_flag();

_NODISCARD surface create_surface(platform::window window);
void remove_surface(surface_id id);
void resize_surface(surface_id id, u32, u32);
_NODISCARD u32 surface_width(surface_id id);
_NODISCARD u32 surface_height(surface_id id);
void render_surface(surface_id id, frame_info info);
}

#endif
