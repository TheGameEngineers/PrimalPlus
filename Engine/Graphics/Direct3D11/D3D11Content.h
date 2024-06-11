#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::content {
bool initialize();
void shutdown();

namespace submesh {
struct views_cache
{
    ID3D11Buffer** const							index_buffers;
    ID3D11ShaderResourceView** const				position_views;
    ID3D11ShaderResourceView** const				element_views;
    D3D_PRIMITIVE_TOPOLOGY* const					primitive_topologies;
    DXGI_FORMAT* const								index_formats{};
    u32* const										elements_types{};
    u32* const										index_counts;
};

id::id_type add(const u8*& data);
void remove(id::id_type id);
void get_views(const id::id_type* const gpu_ids, u32 id_count, const views_cache& cache);
}//namespace submesh

namespace texture {
id::id_type add(const u8* const);
void remove(id::id_type);
}

namespace material {
struct materials_cache
{
    material_type::type* const		material_types;
};

id::id_type add(material_init_info info);
void remove(id::id_type id);
void get_materials(const id::id_type* const material_ids, u32 material_count, const materials_cache& cache);
}//namespace material

namespace render_item {
struct items_cache {
    id::id_type* const					entity_ids;
    id::id_type* const					submesh_gpu_ids;
    id::id_type* const					material_ids;
    d3d11_pipeline_state* const			psos;
};
id::id_type add(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count, const id::id_type* const material_ids);
void remove(id::id_type id);
void get_d3d11_render_item_ids(const frame_info& info, utl::vector<id::id_type>& d3d11_render_item_ids);
void get_items(const id::id_type* const d3d11_render_item_ids, u32 id_count, const items_cache& cache);
}//namespace render_item
}

#endif
