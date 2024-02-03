#include "D3D11PostProcess.h"
#include "D3D11Shaders.h"
#include "D3D11GPass.h"
#include "D3D11Core.h"
#include "Shaders/SharedTypes.h"

#include "D3D11LightCulling.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::fx {
namespace {
#pragma region blur stuff... No used for now
constexpr const math::v4 kernel_disabled[9]{
	{ -1.f, -1.f, 0.f, 0.f },
	{ -1.f,  0.f, 0.f, 0.f },
	{ -1.f,  1.f, 0.f, 0.f },
	{  0.f, -1.f, 0.f, 0.f },
	{  0.f,  0.f, 1.f, 0.f },
	{  0.f,  1.f, 0.f, 0.f },
	{  1.f, -1.f, 0.f, 0.f },
	{  1.f,  0.f, 0.f, 0.f },
	{  1.f,  1.f, 0.f, 0.f },
};

constexpr const math::v4 kernel_sharpen[9]{
	{ -1.f, -1.f,  0.f, 0.f },
	{ -1.f,  0.f, -1.f, 0.f },
	{ -1.f,  1.f,  0.f, 0.f },
	{  0.f, -1.f, -1.f, 0.f },
	{  0.f,  0.f,  5.f, 0.f },
	{  0.f,  1.f, -1.f, 0.f },
	{  1.f, -1.f,  0.f, 0.f },
	{  1.f,  0.f, -1.f, 0.f },
	{  1.f,  1.f,  0.f, 0.f },
};

constexpr const math::v4 kernel_emboss[9]{
	{ -1.f, -1.f, -2.f, 0.f },
	{ -1.f,  0.f, -1.f, 0.f },
	{ -1.f,  1.f,  0.f, 0.f },
	{  0.f, -1.f, -1.f, 0.f },
	{  0.f,  0.f,  1.f, 0.f },
	{  0.f,  1.f,  1.f, 0.f },
	{  1.f, -1.f,  0.f, 0.f },
	{  1.f,  0.f,  1.f, 0.f },
	{  1.f,  1.f,  2.f, 0.f },
};

constexpr const math::v4 kernel_edge_detection[9]{
	{ -1.f, -1.f,  1.f, 0.f },
	{ -1.f,  0.f,  0.f, 0.f },
	{ -1.f,  1.f, -1.f, 0.f },
	{  0.f, -1.f,  0.f, 0.f },
	{  0.f,  0.f,  0.f, 0.f },
	{  0.f,  1.f,  0.f, 0.f },
	{  1.f, -1.f,  0.f, 0.f },
	{  1.f,  0.f, -1.f, 0.f },
	{  1.f,  1.f,  1.f, 0.f },
};

struct blur_parameters
{
	constexpr static u32 sample_count{ 9 };

	enum type : u32
	{
		disabled = 0,
		gaussian,
		box,
		sharpen,
		emboss,
		edge_detection
	};

	math::v4    values[sample_count];

	void set_params(f32 amount, type blur_type)
	{
		if (blur_type == type::disabled)
		{

			return;
		}
		else if (blur_type == type::sharpen)
		{
			memcpy(&values[0], &kernel_sharpen[0], sizeof(math::v4) * sample_count);
			return;
		}
		else if (blur_type == type::emboss)
		{
			memcpy(&values[0], &kernel_emboss[0], sizeof(math::v4) * sample_count);
			return;
		}
		else if (blur_type == type::edge_detection)
		{
			memcpy(&values[0], &kernel_edge_detection[0], sizeof(math::v4) * sample_count);
			return;
		}

		values[0].z = compute_blur(0.f, amount, blur_type);
		values[0].x = values[0].y = 0.f;

		f32 total = values[0].z;

		for (u32 i{ 0 }; i < sample_count / 2; ++i)
		{
			f32 w = compute_blur(f32(i + 1.f), amount, blur_type);

			values[i * 2 + 1].z = w;
			values[i * 2 + 2].z = w;

			total += w * 2;

			f32 offset = f32(i) * 2.f + 1.5f;
			math::v2 delta = { offset, offset };

			values[i * 2 + 1].x = delta.x;
			values[i * 2 + 1].y = delta.y;
			values[i * 2 + 2].x = -delta.x;
			values[i * 2 + 2].y = -delta.y;
		}

		for (u32 i{ 0 }; i < sample_count; ++i)
		{
			values[i].z /= total;
		}
	}

private:
	f32 compute_blur(f32 n, f32 theta, type blur_type)
	{
		switch (blur_type)
		{
		case type::gaussian:
			return (f32)((1.f / sqrtf(2 * math::pi * theta)) * expf(-(n * n) / (2 * theta * theta)));
		case type::box:
			return n / theta;
		default:
			return 0.f;
		}
	}
};

static_assert((sizeof(blur_parameters) % 16) == 0, "Make sure blur_parameters is formatted in 16 byte chunks without any implicit padding");
#pragma endregion

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
