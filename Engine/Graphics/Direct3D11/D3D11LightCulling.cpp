#include "D3D11LightCulling.h"
#include "D3D11Core.h"
#include "Shaders/SharedTypes.h"
#include "D3D11Shaders.h"
#include "D3D11Light.h"
#include "D3D11Camera.h"
#include "D3D11GPass.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::lightculling {
namespace {
class uav_srv_buffer
{
public:
    uav_srv_buffer() = default;
    uav_srv_buffer(d3d11_buffer_init_info info)
        : _buffer(info)
    {
        core::release(_uav);
        core::release(_srv);

        ID3D11Buffer* const buffer{ _buffer.buffer() };

        core::device()->CreateUnorderedAccessView(buffer, nullptr, &_uav);
        core::device()->CreateShaderResourceView(buffer, nullptr, &_srv);
    }

    constexpr uav_srv_buffer(uav_srv_buffer&& o) noexcept
        : _buffer{ std::move(o._buffer) }, _uav{ o._uav }, _srv{ o._srv }
    {
        o.reset();
    }

    constexpr uav_srv_buffer& operator=(uav_srv_buffer&& o) noexcept
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }

    ~uav_srv_buffer()
    {
        release();
    }

    void release()
    {
        core::release(_uav);
        core::release(_srv);
        _buffer.release();
    }

    _NODISCARD constexpr ID3D11Buffer* const buffer() const { return _buffer.buffer(); }
    _NODISCARD constexpr u32 size() const { return _buffer.size(); }
    _NODISCARD constexpr ID3D11UnorderedAccessView* uav() const { return _uav; }
    _NODISCARD constexpr ID3D11ShaderResourceView* srv() const { return _srv; }

    constexpr static d3d11_buffer_init_info get_default_init_info(u32 size, u32 alignment)
    {
        d3d11_buffer_init_info info{};
        info.size = size;
        info.alignment = alignment;
        info.misc_flags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        info.bind_flags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        return info;
    }

private:
    constexpr void move(uav_srv_buffer& o)
    {
        _buffer = std::move(o._buffer);
        _uav = o._uav;
        _srv = o._srv;
        o.reset();
    }

    constexpr void reset()
    {
        _buffer = {};
        _uav = nullptr;
        _srv = nullptr;
    }

    d3d11_buffer				_buffer;
    ID3D11UnorderedAccessView*	_uav{ nullptr };
    ID3D11ShaderResourceView*	_srv{ nullptr };
};

struct culling_parameters
{
    uav_srv_buffer							frustums;
    uav_srv_buffer							light_grid_opaque_buffer;
    uav_srv_buffer							light_index_list_opaque_buffer;
    uav_clearable_buffer					light_index_counter;
    hlsl::LightCullingDispatchParameters	grid_frustums_dispatch_params{};
    hlsl::LightCullingDispatchParameters	light_culling_dispatch_params{};
    u32										frustum_count{ 0 };
    u32										view_width{ 0 };
    u32										view_height{ 0 };
    f32										camera_fov{ 0.f };
    bool									has_lights{ true };
};

struct light_culler
{
    culling_parameters						cullers[frame_buffer_count]{};
};

constexpr u32								max_lights_per_tile{ 256 };

utl::free_list<light_culler>				light_cullers;

void
resize_buffers(culling_parameters& culler)
{
    const u32 frustum_count{ culler.frustum_count };
    const u32 frustums_buffer_size{ sizeof(hlsl::Frustum) * frustum_count };
    const u32 light_grid_buffer_size{ (u32)math::align_size_up<sizeof(math::v4)>(sizeof(math::u32v2) * frustum_count) };
    const u32 light_index_list_buffer_size{ (u32)math::align_size_up<sizeof(math::v4)>(sizeof(u32) * max_lights_per_tile * frustum_count) };

    d3d11_buffer_init_info info{};

    if (frustums_buffer_size > culler.frustums.size())
    {
        culler.frustums = uav_srv_buffer{ uav_srv_buffer::get_default_init_info(frustums_buffer_size, sizeof(hlsl::Frustum)) };
    }

    if (light_grid_buffer_size > culler.light_grid_opaque_buffer.size())
    {
        culler.light_grid_opaque_buffer = uav_srv_buffer{ uav_srv_buffer::get_default_init_info(light_grid_buffer_size, sizeof(hlsl::uint2)) };

        NAME_D3D11_OBJECT_INDEXED(culler.light_grid_opaque_buffer.buffer(), light_grid_buffer_size,
            L"Light Grid Opaque Buffer - size");

        if (!culler.light_index_counter.buffer())
        {
            culler.light_index_counter = uav_clearable_buffer{ uav_clearable_buffer::get_default_init_info(1, sizeof(hlsl::uint)) };
            NAME_D3D11_OBJECT_INDEXED(culler.light_index_counter.buffer(), core::current_frame_index(), L"Light Index Counter Buffer");
        }
    }

    if (light_index_list_buffer_size > culler.light_index_list_opaque_buffer.size())
    {
        culler.light_index_list_opaque_buffer = uav_srv_buffer{ uav_srv_buffer::get_default_init_info(light_index_list_buffer_size, sizeof(hlsl::uint)) };


        NAME_D3D11_OBJECT_INDEXED(culler.light_index_list_opaque_buffer.buffer(), light_index_list_buffer_size,
            L"Light Index List Opaque Buffer - size");
    }
}

void
resize(culling_parameters& culler)
{
    constexpr u32 tile_size{ light_culling_tile_size };
    assert(culler.view_width >= tile_size && culler.view_height >= tile_size);
    const math::u32v2 tile_count
    {
        (u32)math::align_size_up<tile_size>(culler.view_width) / tile_size,
        (u32)math::align_size_up<tile_size>(culler.view_height) / tile_size
    };

    culler.frustum_count = tile_count.x * tile_count.y;

    {
        hlsl::LightCullingDispatchParameters& params{ culler.grid_frustums_dispatch_params };
        params.NumThreads = tile_count;
        params.NumThreadGroups.x = (u32)math::align_size_up<tile_size>(tile_count.x) / tile_size;
        params.NumThreadGroups.y = (u32)math::align_size_up<tile_size>(tile_count.y) / tile_size;
    }

    {
        hlsl::LightCullingDispatchParameters& params{ culler.light_culling_dispatch_params };
        params.NumThreads.x = tile_count.x * tile_size;
        params.NumThreads.y = tile_count.y * tile_size;
        params.NumThreadGroups = tile_count;
    }

    resize_buffers(culler);
}

void
calculate_grid_frustums(culling_parameters& culler, ID3D11DeviceContext4* const ctx,
    const d3d11_frame_info& d3d11_info)
{
    constant_buffer& cbuffer{ core::cbuffer() };
    hlsl::LightCullingDispatchParameters* const buffer{ cbuffer.allocate<hlsl::LightCullingDispatchParameters>() };
    const hlsl::LightCullingDispatchParameters& params{ culler.grid_frustums_dispatch_params };
    memcpy(buffer, &params, sizeof(hlsl::LightCullingDispatchParameters));
    
    ID3D11UnorderedAccessView* const uavs[]{ culler.frustums.uav() };
    ID3D11Buffer* const buffers[]{ cbuffer.buffer(), cbuffer.buffer() };
    UINT constants[]{ d3d11_info.global_shader_data_offset, cbuffer.offset(buffer) };
    constexpr UINT offsets[]{ d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::GlobalShaderData)),
    d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::LightCullingDispatchParameters)) };

    ctx->CSSetShader((ID3D11ComputeShader*)shaders::get_engine_shader(shaders::engine_shader::grid_frustums_cs), nullptr, 0);
    ctx->CSSetConstantBuffers1(0, _countof(buffers), buffers, constants, offsets);
    ctx->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
    ctx->Dispatch(params.NumThreadGroups.x, params.NumThreadGroups.y, 1);

    //unbind uav
    ID3D11UnorderedAccessView* clr[1]{ nullptr };
    ctx->CSSetUnorderedAccessViews(0, 1, clr, nullptr);
}

void __declspec(noinline)
resize_and_calculate_grid_frustums(culling_parameters& culler, ID3D11DeviceContext4* const ctx,
    const d3d11_frame_info& d3d11_info)
{
    culler.camera_fov = d3d11_info.camera->field_of_view();
    culler.view_width = d3d11_info.surface_width;
    culler.view_height = d3d11_info.surface_height;

    resize(culler);

    const math::u32v4 clear_value{ 0, 0, 0, 0 };
    ctx->ClearUnorderedAccessViewUint(culler.light_grid_opaque_buffer.uav(), &clear_value.x);

    calculate_grid_frustums(culler, ctx, d3d11_info);
}
}//anonyoums namespace

bool
initialize()
{
    return light::initialize();
}

void
shutdown()
{
    light::shutdown();
}

_NODISCARD id::id_type
add_culler()
{
    return light_cullers.add();
}

void
remove_culler(id::id_type id)
{
    assert(id::is_valid(id));
    light_cullers.remove(id);
}

void cull_lights(ID3D11DeviceContext4* const ctx,
    const d3d11_frame_info& d3d11_info)
{
    const id::id_type id{ d3d11_info.light_culling_id };
    assert(id::is_valid(id));
    culling_parameters& culler{ light_cullers[id].cullers[d3d11_info.frame_index] };

    if (d3d11_info.surface_width != culler.view_width ||
        d3d11_info.surface_height != culler.view_height ||
        !math::is_equal(d3d11_info.camera->field_of_view(), culler.camera_fov))
    {
        resize_and_calculate_grid_frustums(culler, ctx, d3d11_info);
    }

    const u32 frame_idx{ d3d11_info.frame_index };

    hlsl::LightCullingDispatchParameters& params{ culler.light_culling_dispatch_params };
    params.NumLights = light::cullable_light_count(d3d11_info.info->light_set_key);

    if (!params.NumLights && !culler.has_lights) return;

    culler.has_lights = params.NumLights > 0;

    constant_buffer& cbuffer{ core::cbuffer() };
    hlsl::LightCullingDispatchParameters* const buffer{ cbuffer.allocate<hlsl::LightCullingDispatchParameters>() };
    memcpy(buffer, &params, sizeof(hlsl::LightCullingDispatchParameters));

    const math::u32v4 clear_value{ 0, 0, 0, 0 };
    culler.light_index_counter.clear_uav(ctx, &clear_value.x);

    ID3D11Buffer* const buffers[]{ cbuffer.buffer(), cbuffer.buffer() };
    
    UINT constants[]{ d3d11_info.global_shader_data_offset, cbuffer.offset(buffer) };
    
    constexpr UINT offsets[]{ d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::GlobalShaderData)),
    d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::LightCullingDispatchParameters)) };
    
    ID3D11ShaderResourceView* const srvs[]{ culler.frustums.srv(), light::culling_info_buffer(frame_idx),
#if USE_BOUNDING_SPHERES
        light::bounding_spheres_buffer(frame_idx),
#endif
        gpass::depth_buffer().srv() };

    ID3D11UnorderedAccessView* const uavs[]{ culler.light_index_counter.uav(), culler.light_grid_opaque_buffer.uav(), nullptr,
        culler.light_index_list_opaque_buffer.uav() };
    
    constexpr UINT uav_counts[]{ (UINT)-1, (UINT)-1, (UINT)-1, (UINT)-1 };

    ctx->CSSetShader((ID3D11ComputeShader*)shaders::get_engine_shader(shaders::engine_shader::light_culling_cs), nullptr, 0);
    ctx->CSSetConstantBuffers1(0, _countof(buffers), buffers, constants, offsets);
    ctx->CSSetShaderResources(0, _countof(srvs), srvs);
    ctx->CSSetUnorderedAccessViews(0, _countof(uavs), uavs, uav_counts);

    ctx->Dispatch(params.NumThreadGroups.x, params.NumThreadGroups.y, 1);
    
    ID3D11ShaderResourceView* const clear_srvs[]{ nullptr, nullptr, nullptr, nullptr };
    ID3D11UnorderedAccessView* const clear_uavs[]{ nullptr, nullptr, nullptr, nullptr };
    ctx->CSSetShaderResources(0, _countof(clear_srvs), clear_srvs);
    ctx->CSSetUnorderedAccessViews(0, _countof(clear_uavs), clear_uavs, uav_counts);
}

//TODO: REMOVE!!!
ID3D11ShaderResourceView*
frustums(id::id_type light_culling_id, u32 frame_index)
{
    assert(frame_index < frame_buffer_count && id::is_valid(light_culling_id));
    return light_cullers[light_culling_id].cullers[frame_index].frustums.srv();
}

ID3D11ShaderResourceView*
light_grid_opaque(id::id_type light_culling_id, u32 frame_index)
{
    assert(frame_index < frame_buffer_count && id::is_valid(light_culling_id));
    return light_cullers[light_culling_id].cullers[frame_index].light_grid_opaque_buffer.srv();
}

ID3D11ShaderResourceView*
light_index_list_opaque(id::id_type light_culling_id, u32 frame_index)
{
    assert(frame_index < frame_buffer_count && id::is_valid(light_culling_id));
    return light_cullers[light_culling_id].cullers[frame_index].light_index_list_opaque_buffer.srv();
}
}

#endif
