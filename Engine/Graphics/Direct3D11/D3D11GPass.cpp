#include "D3D11GPass.h"
#include "D3D11Core.h"
#include "D3D11Shaders.h"
#include "D3D11Content.h"
#include "D3D11Camera.h"
#include "D3D11Light.h"
#include "D3D11LightCulling.h"
#include "Shaders/SharedTypes.h"
#include "Components/Entity.h"
#include "Components/Transform.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::gpass {
namespace {
constexpr math::u32v2				initial_dimensions{ 100, 100 };

d3d11_render_texture				gpass_main_buffer{};
d3d11_depth_buffer					gpass_depth_buffer{};
math::u32v2							dimensions{ initial_dimensions };
ID3D11DepthStencilState*			depth_state{ nullptr };
ID3D11DepthStencilState*			readonly_depth_state{ nullptr };
ID3D11RasterizerState2*				rs_state_cull{ nullptr };

#if _DEBUG
constexpr f32						clear_value[4]{ 0.5f, 0.5f, .5f, 1.f };
#else
constexpr f32						clear_value[4]{ 0.f, 0.f, 0.f, 1.f };
#endif

#if USE_STL_VECTOR
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

struct gpass_cache
{
	utl::vector<id::id_type>		d3d11_render_item_ids;
	id::id_type*					entity_ids{ nullptr };
	id::id_type*					submesh_gpu_ids{ nullptr };
	id::id_type*					material_ids{ nullptr };
	d3d11_pipeline_state*			pipeline_states{ nullptr };
	material_type::type*			material_types{ nullptr };
	D3D_PRIMITIVE_TOPOLOGY*			primitive_topologies{ nullptr };
	u32*							elements_types{ nullptr };
	u32*							per_object_data_offsets{ nullptr };
	ID3D11Buffer**					index_buffers{ nullptr };
	ID3D11ShaderResourceView**		position_views{ nullptr };
	ID3D11ShaderResourceView**		element_views{ nullptr };
	DXGI_FORMAT*					index_formats{ nullptr };
	u32*							index_counts{ nullptr };

	constexpr content::render_item::items_cache items_cache() const
	{
		return { entity_ids, submesh_gpu_ids, material_ids, pipeline_states };
	}

	constexpr content::submesh::views_cache views_cache() const
	{
		return { index_buffers, position_views, element_views, primitive_topologies, index_formats, elements_types, index_counts };
	}

	constexpr content::material::materials_cache materials_cache() const
	{
		return { material_types };
	}

	CONSTEXPR u32 size() const
	{
		return (u32)d3d11_render_item_ids.size();
	}

	CONSTEXPR void clear()
	{
		d3d11_render_item_ids.clear();
	}

	CONSTEXPR void resize()
	{
		const u64 items_count{ d3d11_render_item_ids.size() };
		const u64 new_buffer_size{ items_count * struct_size };
		const u64 old_buffer_size{ _buffer.size() };
		if (new_buffer_size > old_buffer_size)
		{
			_buffer.resize(new_buffer_size);
		}

		if (new_buffer_size != old_buffer_size)
		{
			entity_ids = (id::id_type*)(_buffer.data());
			submesh_gpu_ids = (id::id_type*)(&entity_ids[items_count]);
			material_ids = (id::id_type*)(&submesh_gpu_ids[items_count]);
			pipeline_states = (d3d11_pipeline_state*)(&material_ids[items_count]);
			material_types = (material_type::type*)(&pipeline_states[items_count]);
			primitive_topologies = (D3D_PRIMITIVE_TOPOLOGY*)(&material_types[items_count]);
			elements_types = (u32*)(&primitive_topologies[items_count]);
			per_object_data_offsets = (u32*)(&elements_types[items_count]);
			index_buffers = (ID3D11Buffer**)(&per_object_data_offsets[items_count]);
			position_views = (ID3D11ShaderResourceView**)(&index_buffers[items_count]);
			element_views = (ID3D11ShaderResourceView**)(&position_views[items_count]);
			index_formats = (DXGI_FORMAT*)(&element_views[items_count]);
			index_counts = (u32*)(&index_formats[items_count]);
		}
	}

private:
	constexpr static u32 struct_size{
		sizeof(id::id_type) +									//entity_ids{ nullptr };
			sizeof(id::id_type) +								//submesh_ids{ nullptr };
			sizeof(id::id_type) +								//material_ids{ nullptr };
			sizeof(d3d11_pipeline_state) +						//pipeline_states{ nullptr };
			sizeof(material_type::type) +						//material_types{ nullptr };
			sizeof(D3D_PRIMITIVE_TOPOLOGY) +					//primitive_topologies{ nullptr }
			sizeof(u32) +										//elements_types{ nullptr };
			sizeof(u32) +										//per object data offsets{ nullptr };
			sizeof(ID3D11Buffer*) +								//index_buffers{ nullptr };
			sizeof(ID3D11ShaderResourceView*) +					//position_buffers{ nullptr };
			sizeof(ID3D11ShaderResourceView*) +					//element_buffers{ nullptr };
			sizeof(DXGI_FORMAT) +								//index_formats{ nullptr };
			sizeof(u32)											//index_counts{ nullptr };
	};

	utl::vector<u8>					_buffer;
} frame_cache;

#undef CONSTEXPR

bool
create_buffers(math::u32v2 size)
{
	assert(size.x && size.y);
	gpass_main_buffer.release();
	gpass_depth_buffer.release();

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = main_buffer_format;
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = 0;
	desc.SampleDesc = { 1, 0 };

	{
		d3d11_texture_init_info info{};
		info.desc = &desc;

		gpass_main_buffer = d3d11_render_texture{ info };

	}

	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	desc.Format = depth_buffer_format;
	desc.MipLevels = 1;

	{
		d3d11_texture_init_info info{};
		info.desc = &desc;

		gpass_depth_buffer = d3d11_depth_buffer{ info };
	}

	NAME_D3D11_OBJECT(gpass_main_buffer.resource(), L"GPass Main Buffer");
	NAME_D3D11_OBJECT(gpass_depth_buffer.resource(), L"GPass Depth Buffer");

	return gpass_main_buffer.resource() && gpass_depth_buffer.resource();
}

void
fill_in_per_object_buffer(const d3d11_frame_info& d3d11_info)
{
	const gpass_cache& cache{ frame_cache };
	const u32 render_items_count{ (u32)cache.size() };
	id::id_type current_entity_id{ id::invalid_id };
	hlsl::PerObjectData* current_data_pointer{ nullptr };

	constant_buffer& cbuffer{ core::cbuffer() };

	using namespace DirectX;
	for (u32 i{ 0 }; i < render_items_count; ++i)
	{
		if (current_entity_id != cache.entity_ids[i])
		{
			current_entity_id = cache.entity_ids[i];
			hlsl::PerObjectData data{};
			transform::get_transform_matrices(game_entity::entity_id{ current_entity_id }, data.World, data.InvWorld);
			XMMATRIX world{ XMLoadFloat4x4(&data.World) };
			XMMATRIX exp{ XMMatrixMultiply(world, d3d11_info.camera->view_projection()) };
			XMStoreFloat4x4(&data.WorldViewProjection, exp);

			current_data_pointer = cbuffer.allocate<hlsl::PerObjectData>();
			memcpy(current_data_pointer, &data, sizeof(hlsl::PerObjectData));
		}

		assert(current_data_pointer);
		cache.per_object_data_offsets[i] = cbuffer.offset(current_data_pointer);
	}
}

void
set_data_views(ID3D11DeviceContext4* const ctx, u32 cache_index)
{
	gpass_cache& cache{ frame_cache };
	assert(cache_index < cache.size());

	const material_type::type mtl_type{ cache.material_types[cache_index] };
	switch (mtl_type)
	{
	case material_type::opaque:
	{
		ID3D11ShaderResourceView* const srvs[]{ cache.position_views[cache_index], cache.element_views[cache_index] };
		ctx->VSSetShaderResources(0, _countof(srvs), srvs);
	} break;
	}
}

void
prepare_render_frame(const d3d11_frame_info& d3d11_info)
{
	assert(d3d11_info.info && d3d11_info.camera);
	assert(d3d11_info.info->render_item_ids && d3d11_info.info->render_item_count);
	gpass_cache& cache{ frame_cache };
	cache.clear();

	using namespace content;
	render_item::get_d3d11_render_item_ids(*d3d11_info.info, cache.d3d11_render_item_ids);
	cache.resize();
	const u32 items_count{ cache.size() };
	const render_item::items_cache items_cache{ cache.items_cache() };
	render_item::get_items(cache.d3d11_render_item_ids.data(), items_count, items_cache);

	const submesh::views_cache views_cache{ cache.views_cache() };
	submesh::get_views(items_cache.submesh_gpu_ids, items_count, views_cache);

	const material::materials_cache materials_cache{ cache.materials_cache() };
	material::get_materials(items_cache.material_ids, items_count, materials_cache);

	fill_in_per_object_buffer(d3d11_info);
}
}//anonymous namespace

bool
initialize()
{
	DXCall(core::device()->CreateDepthStencilState(&d3dx::depth_state.reversed, &depth_state));
	DXCall(core::device()->CreateDepthStencilState(&d3dx::depth_state.reversed_readonly, &readonly_depth_state));
	DXCall(core::device()->CreateRasterizerState2(&d3dx::rasterizer_state.backface_cull, &rs_state_cull));
	NAME_D3D11_OBJECT(rs_state_cull, L"Default D3D11 Rasterizer State");

	return create_buffers(initial_dimensions);
}

void
shutdown()
{
	core::release(depth_state);
	core::release(readonly_depth_state);
	core::release(rs_state_cull);
	gpass_main_buffer.release();
	gpass_depth_buffer.release();
	dimensions = initial_dimensions;
}

_NODISCARD d3d11_render_texture&
main_buffer()
{
	return gpass_main_buffer;
}

_NODISCARD d3d11_depth_buffer&
depth_buffer()
{
	return gpass_depth_buffer;
}

void
set_size(math::u32v2 size)
{
	math::u32v2& d{ dimensions };
	if (size.x > d.x || size.y > d.y)
	{
		d = { std::max(size.x, d.x), std::max(size.y, d.y) };
		create_buffers(d);
	}
}

void
depth_prepass(ID3D11DeviceContext4* ctx, const d3d11_frame_info& d3d11_info)
{
	prepare_render_frame(d3d11_info);

	const gpass_cache& cache{ frame_cache };
	const u32 items_count{ cache.size() };

	ctx->OMSetDepthStencilState(depth_state, 0);

	d3d11_pipeline_state current_state{};
	ctx->PSSetShader(current_state.ps, nullptr, 0);

	ctx->RSSetState(rs_state_cull);

	for (u32 i{ 0 }; i < items_count; ++i)
	{
		set_data_views(ctx, i);
		d3d11_pipeline_state& state{ cache.pipeline_states[i] };

		{
			ID3D11Buffer* const buffers[]{ core::cbuffer().buffer(), core::cbuffer().buffer() };
			UINT offsets[]{ d3d11_info.global_shader_data_offset, cache.per_object_data_offsets[i] };
			constexpr UINT constants[]{ d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::GlobalShaderData)),
			d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::PerObjectData)) };

			ctx->VSSetConstantBuffers1(0, _countof(buffers), buffers, offsets, constants);
		}

		if (current_state.vs != state.vs)
		{
			ctx->VSSetShader(state.vs, nullptr, 0);
			current_state.vs = state.vs;
		}
		if (current_state.hs != state.hs)
		{
			ctx->HSSetShader(state.hs, nullptr, 0);
			current_state.hs = state.hs;
		}
		if (current_state.ds != state.ds)
		{
			ctx->DSSetShader(state.ds, nullptr, 0);
			current_state.ds = state.ds;
		}
		if (current_state.gs != state.gs)
		{
			ctx->GSSetShader(state.gs, nullptr, 0);
			current_state.gs = state.gs;
		}

		ctx->IASetIndexBuffer(cache.index_buffers[i], cache.index_formats[i], 0);
		ctx->IASetPrimitiveTopology(cache.primitive_topologies[i]);
		ctx->DrawIndexedInstanced(cache.index_counts[i], 1, 0, 0, 0);
	}

	//unbind output
	ID3D11RenderTargetView* null_rtv{ nullptr };
	ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
}

void
render(ID3D11DeviceContext4* ctx, const d3d11_frame_info& d3d11_info)
{
	const gpass_cache& cache{ frame_cache };
	const u32 items_count{ cache.size() };
	const u32 frame_idx{ d3d11_info.frame_index };
	const u32 light_culling_id{ d3d11_info.light_culling_id };

	ctx->OMSetDepthStencilState(readonly_depth_state, 0);

	d3d11_pipeline_state current_state{ nullptr };

	ID3D11ShaderResourceView* const pssrvs[]{ light::non_cullable_light_buffer(frame_idx),
	light::cullable_light_buffer(frame_idx), lightculling::light_grid_opaque(light_culling_id, frame_idx),
	lightculling::light_index_list_opaque(light_culling_id, frame_idx) };
	ctx->PSSetShaderResources(3, _countof(pssrvs), pssrvs);

	for (u32 i{ 0 }; i < items_count; ++i)
	{
		set_data_views(ctx, i);
		d3d11_pipeline_state& state{ cache.pipeline_states[i] };

		ID3D11Buffer* const buffers[]{ core::cbuffer().buffer(), core::cbuffer().buffer() };
		UINT offsets[]{ d3d11_info.global_shader_data_offset, cache.per_object_data_offsets[i] };
		constexpr UINT constants[]{ d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::GlobalShaderData)),
		d3dx::align_size_for_constant_buffer_offset(sizeof(hlsl::PerObjectData)) };

		ctx->VSSetConstantBuffers1(0, _countof(buffers), buffers, offsets, constants);
		ctx->PSSetConstantBuffers1(0, _countof(buffers), buffers, offsets, constants);

		if (current_state.vs != state.vs)
		{
			ctx->VSSetShader(state.vs, nullptr, 0);
			current_state.vs = state.vs;
		}
		if (current_state.hs != state.hs)
		{
			ctx->HSSetShader(state.hs, nullptr, 0);
			current_state.hs = state.hs;
		}
		if (current_state.ds != state.ds)
		{
			ctx->DSSetShader(state.ds, nullptr, 0);
			current_state.ds = state.ds;
		}
		if (current_state.gs != state.gs)
		{
			ctx->VSSetShader(state.vs, nullptr, 0);
			current_state.gs = state.gs;
		}
		if (current_state.ps != state.ps)
		{
			ctx->PSSetShader(state.ps, nullptr, 0);
			current_state.ps = state.ps;
		}

		ctx->IASetIndexBuffer(cache.index_buffers[i], cache.index_formats[i], 0);
		ctx->IASetPrimitiveTopology(cache.primitive_topologies[i]);
		ctx->DrawIndexedInstanced(cache.index_counts[i], 1, 0, 0, 0);
	}

	//unbind output
	ID3D11RenderTargetView* null_rtv{ nullptr };
	ctx->OMSetRenderTargets(1, &null_rtv, nullptr);
}

void
set_render_targets_for_depth_prepass(ID3D11DeviceContext4* ctx)
{
	ID3D11DepthStencilView* const dsv{ gpass_depth_buffer.dsv() };
	ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.f, 0);
	ctx->OMSetRenderTargets(0, nullptr, dsv);
}

void
set_render_targets_for_gpass(ID3D11DeviceContext4* ctx)
{
	ID3D11RenderTargetView* const rtvs[]{ gpass_main_buffer.rtv(0) };
	ID3D11DepthStencilView* const dsv{ gpass_depth_buffer.dsv() };
	ctx->ClearRenderTargetView(rtvs[0], clear_value);
	ctx->OMSetRenderTargets(1, rtvs, dsv);
}
}

#endif
