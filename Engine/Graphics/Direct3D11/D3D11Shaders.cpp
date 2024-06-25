#include "D3D11Shaders.h"
#include "D3D11Core.h"

#if PRIMAL_BUILD_D3D11

#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace primal::graphics::d3d11::shaders {
namespace {
struct engine_shader_info {
    const wchar_t*			full_path;
    const char*				function;
    shader_type::type		type;
    engine_shader::id		engine_id;
};

constexpr const char* targets[shader_type::count]{ "vs_5_0", "hs_5_0", "ds_5_0", "gs_5_0", "ps_5_0", "cs_5_0", nullptr, nullptr };
#if _DEBUG
constexpr u32 flags{ D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG };
#else
constexpr u32 flags{ D3DCOMPILE_OPTIMIZATION_LEVEL3 };
#endif

void* engine_shaders[engine_shader::count]{};
engine_shader_info shaders_info[engine_shader::count]
{
    { L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\FullScreenTriangle.hlsl", "FullScreenTriangleVS", shader_type::vertex, engine_shader::fullscreen_triangle_vs },
    { L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\FillColor.hlsl", "FillColorPS", shader_type::pixel, engine_shader::fill_color_ps },
    { L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\PostProcess.hlsl", "PostProcessPS", shader_type::pixel, engine_shader::post_process_ps },
    { L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\GridFrustums.hlsl", "GridFrustumsCS", shader_type::compute, engine_shader::grid_frustums_cs },
    { L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\CullLights.hlsl", "CullLightsCS", shader_type::compute, engine_shader::light_culling_cs }
};

void
compile_shader(const engine_shader_info& info)
{
    ID3DBlob* shader_blob{ nullptr };
    ID3DBlob* err_blob{ nullptr };
    HRESULT hr{ D3DCompileFromFile(info.full_path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, info.function, targets[info.type], flags, 0, &shader_blob, &err_blob) };
    if (err_blob || FAILED(hr))
    {
        OutputDebugStringA((char*)err_blob->GetBufferPointer());
        if (FAILED(hr)) return;
    }

    const u32& shader_id{ info.engine_id };

    switch (shader_id)
    {
    case engine_shader::fullscreen_triangle_vs:
        DXCall(hr = core::device()->CreateVertexShader(shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(), nullptr, (ID3D11VertexShader**)&engine_shaders[shader_id]));
        break;
    case engine_shader::fill_color_ps:
        DXCall(hr = core::device()->CreatePixelShader(shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(), nullptr, (ID3D11PixelShader**)&engine_shaders[shader_id]));
        break;
    case engine_shader::post_process_ps:
        DXCall(hr = core::device()->CreatePixelShader(shader_blob->GetBufferPointer(),
        shader_blob->GetBufferSize(), nullptr, (ID3D11PixelShader**)&engine_shaders[shader_id]));
        break;
    case engine_shader::grid_frustums_cs:
        DXCall(hr = core::device()->CreateComputeShader(shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(), nullptr, (ID3D11ComputeShader**)&engine_shaders[shader_id]));
        break;
    case engine_shader::light_culling_cs:
        DXCall(hr = core::device()->CreateComputeShader(shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(), nullptr, (ID3D11ComputeShader**)&engine_shaders[shader_id]));
        break;
    default:
        break;
    }
}
}//anonymous namespace

bool
initialize()
{
    utl::vector<std::thread> threads;
    for (u32 i{ 0 }; i < engine_shader::count; ++i)
        threads.emplace_back(std::thread(compile_shader, shaders_info[i]));

    for (u32 i{ 0 }; i < engine_shader::count; ++i)
        threads[i].join();

    return true;
}

void
shutdown()
{
    for (auto& shader : engine_shaders)
    {
        core::release(reinterpret_cast<ID3D11DeviceChild*&>(shader));
    }
}

void*
get_engine_shader(engine_shader::id id)
{
    return engine_shaders[id];
}
}

#endif
