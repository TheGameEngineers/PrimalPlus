#include "D3D11PostProcess.h"
#include "D3D11Shaders.h"
#include "D3D11GPass.h"
#include "D3D11Core.h"
#include "Shaders/SharedTypes.h"

#include "D3D11LightCulling.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::fx {
namespace {
ID3D11VertexShader*			fullscreen_triangle_shader{ nullptr };
ID3D11PixelShader*			post_process_shader{ nullptr };
ID3D11RasterizerState2*		rs_state{ nullptr };
}//anonymous namespace

bool
initialize()
{
    fullscreen_triangle_shader = (ID3D11VertexShader*)shaders::get_engine_shader(shaders::engine_shader::fullscreen_triangle_vs);
    post_process_shader = (ID3D11PixelShader*)shaders::get_engine_shader(shaders::engine_shader::post_process_ps);

    auto* const device{ core::device() };
    DXCall(device->CreateRasterizerState2(&d3dx::rasterizer_state.no_cull, &rs_state));

    return (fullscreen_triangle_shader && post_process_shader);
}

void
shutdown()
{
    core::release(rs_state);
}

void
post_process(ID3D11DeviceContext4* ctx, const d3d11_frame_info& d3d11_info, ID3D11RenderTargetView* target_rtv)
{
    ID3D11ShaderResourceView* const srvs[]{ gpass::main_buffer().srv(), lightculling::frustums(d3d11_info.light_culling_id, d3d11_info.frame_index),
    lightculling::light_grid_opaque(d3d11_info.light_culling_id, d3d11_info.frame_index) };

    constant_buffer& cbuffer{ core::cbuffer() };
    ID3D11Buffer* const cbvs[]{ cbuffer.buffer(), cbuffer.buffer() };

    UINT first_constants[]{ d3d11_info.global_shader_data_offset };
    UINT num_constants[]{ d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::GlobalShaderData)) };
    
    ctx->OMSetRenderTargets(1, &target_rtv, nullptr);
    ctx->RSSetState(rs_state);
    ctx->VSSetShader(fullscreen_triangle_shader, 0, 0);
    ctx->PSSetShader(post_process_shader, 0, 0);
    ctx->PSSetShaderResources(0, _countof(srvs), srvs);
    ctx->PSSetConstantBuffers1(0, 1, cbvs, first_constants, num_constants);

    ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->DrawInstanced(3, 1, 0, 0);
    
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
}
}

#endif
