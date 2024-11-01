#include "D3D11Content.h"
#include "D3D11Core.h"
#include "Content/ContentToEngine.h"
#include "Utilities/IOStream.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::content {
namespace {
struct submesh_view
{
    ID3D11Buffer*									index_buffer;
    ID3D11Buffer*									position_buffer;
    ID3D11Buffer*									element_buffer;
    ID3D11ShaderResourceView*						position_view;
    ID3D11ShaderResourceView*						element_view;
    D3D_PRIMITIVE_TOPOLOGY							primitive_topology;
    DXGI_FORMAT										index_format{};
    u32												elements_type{};
    u32												index_count{};
};

struct d3d11_render_item
{
    id::id_type			entity_id;
    id::id_type			submesh_gpu_id;
    id::id_type			material_id;
    id::id_type			pso_id;
};

struct d3d11_pso_storage
{
    id::id_type			vs_id{ id::invalid_id };
    id::id_type			hs_id{ id::invalid_id };
    id::id_type			ds_id{ id::invalid_id };
    id::id_type			gs_id{ id::invalid_id };
    id::id_type			ps_id{ id::invalid_id };
    id::id_type			blend_id{ id::invalid_id };
    id::id_type			rasterizer_id{ id::invalid_id };
    id::id_type			sampler_id{ id::invalid_id };
};

utl::free_list<submesh_view>						submesh_views{};
std::mutex											submesh_mutex{};

utl::free_list<d3d11_texture>						textures;
utl::free_list<ID3D11ShaderResourceView*>			texture_pointers;
std::mutex											texture_mutex{};

utl::free_list<std::unique_ptr<u8[]>>				materials;
std::mutex											material_mutex{};

utl::free_list<d3d11_render_item>					render_items;
utl::free_list<std::unique_ptr<id::id_type[]>>		render_item_ids;
std::mutex											render_item_mutex{};

utl::vector<d3d11_pipeline_state>					pipeline_states;
utl::vector<d3d11_pso_storage>						pso_storages;
utl::vector<ID3D11VertexShader*>					vertex_shaders;
std::unordered_map<u64, id::id_type>				vs_map;
utl::vector<ID3D11HullShader*>						hull_shaders;
std::unordered_map<u64, id::id_type>				hs_map;
utl::vector<ID3D11DomainShader*>					domain_shaders;
std::unordered_map<u64, id::id_type>				ds_map;
utl::vector<ID3D11GeometryShader*>					geometry_shaders;
std::unordered_map<u64, id::id_type>				gs_map;
utl::vector<ID3D11PixelShader*>						pixel_shaders;
std::unordered_map<u64, id::id_type>				ps_map;

//Maybe, just maybe, we might not even be needing this stuff
utl::vector<ID3D11BlendState1*>						blend_states;
std::unordered_map<u64, id::id_type>				blend_state_map;
utl::vector<ID3D11RasterizerState2*>				rasterizer_states;
std::unordered_map<u64, id::id_type>				rasterizer_state_map;
utl::vector<ID3D11SamplerState*>					sampler_states;
std::unordered_map<u64, id::id_type>				sampler_state_map;

std::mutex											pso_mutex{};

struct {
    utl::vector<primal::content::lod_offset>		lod_offsets;
    utl::vector<id::id_type>						geometry_ids;
} frame_cache;

class d3d11_material_stream
{
public:
    DISABLE_COPY_AND_MOVE(d3d11_material_stream)
        explicit d3d11_material_stream(u8* const material_buffer)
        : _buffer{ material_buffer }
    {
        initialize();
    }

    explicit d3d11_material_stream(std::unique_ptr<u8[]>& material_buffer, material_init_info info)
    {
        assert(!material_buffer);

        u32 shader_count{ 0 };
        u32 flags{ 0 };
        for (u32 i{ 0 }; i < shader_type::count; ++i)
        {
            if (id::is_valid(info.shader_ids[i]))
            {
                ++shader_count;
                flags |= (1 << i);
            }
        }

        assert(shader_count && flags);

        const u32 buffer_size{
            sizeof(material_type::type) +								                    //material type
            sizeof(shader_flags::flags) +								                    //shader flags
            sizeof(u32) +												                    //texture count
            sizeof(material_surface) +                                                      //explains it already
            sizeof(id::id_type) * shader_count +						                    //shader ids
            (sizeof(id::id_type) + sizeof(ID3D11ShaderResourceView*)) * info.texture_count  //shader views
        };

        material_buffer = std::make_unique<u8[]>(buffer_size);
        _buffer = material_buffer.get();
        u8* const buffer{ _buffer };

        *(material_type::type*)buffer = info.type;
        *(shader_flags::flags*)(&buffer[shader_flags_index]) = (shader_flags::flags)flags;
        *(u32*)(&buffer[texture_count_index]) = info.texture_count;
        *(material_surface*)&buffer[material_surface_index] = info.surface;

        initialize();

        if (info.texture_count)
        {
            memcpy(_texture_ids, info.texture_ids, info.texture_count * sizeof(id::id_type));
            texture::get_shader_views(_texture_ids, info.texture_count, _shader_views);
        }

        u32 shader_index{ 0 };
        for (u32 i{ 0 }; i < shader_type::count; ++i)
        {
            if (id::is_valid(info.shader_ids[i]))
            {
                _shader_ids[shader_index] = info.shader_ids[i];
                ++shader_index;
            }
        }

        assert(shader_index == (u32)_mm_popcnt_u32(_shader_flags));
    }

    [[nodiscard]] constexpr u32 texture_count() const { return _texture_count; }
    [[nodiscard]] constexpr material_type::type material_type() const { return _type; }
    [[nodiscard]] constexpr shader_flags::flags shader_flags() const { return _shader_flags; }
    [[nodiscard]] constexpr id::id_type* texture_ids() const { return _texture_ids; }
    [[nodiscard]] constexpr ID3D11ShaderResourceView** shader_views() const { return _shader_views; }
    [[nodiscard]] constexpr id::id_type* shader_ids() const { return _shader_ids; }
    [[nodiscard]] constexpr material_surface* surface() const { return _material_surface; }

private:
    void initialize()
    {
        assert(_buffer);
        u8* const buffer{ _buffer };

        _type = *(material_type::type*)buffer;
        _shader_flags = *(shader_flags::flags*)(&buffer[shader_flags_index]);
        _texture_count = *(u32*)(&buffer[texture_count_index]);
        _material_surface = (material_surface*)&buffer[material_surface_index];

        _shader_ids = (id::id_type*)(&buffer[material_surface_index + sizeof(material_surface)]);
        _texture_ids = _texture_count ? &_shader_ids[_mm_popcnt_u32(_shader_flags)] : nullptr;
        _shader_views = _texture_ids ? (ID3D11ShaderResourceView**)(&_texture_ids[_texture_count]) : nullptr;
    }

    constexpr static u32	shader_flags_index{ sizeof(material_type::type) };
    constexpr static u32	texture_count_index{ shader_flags_index + sizeof(shader_flags::flags) };
    constexpr static u32	material_surface_index{ texture_count_index + sizeof(u32) };

    u8*						    _buffer;
    material_surface*           _material_surface;
    id::id_type*			    _texture_ids;
    ID3D11ShaderResourceView**  _shader_views;
    id::id_type*			    _shader_ids;
    u32			    			_texture_count;
    material_type::type		    _type;
    shader_flags::flags		    _shader_flags;
};

constexpr D3D_PRIMITIVE_TOPOLOGY
get_d3d_primitive_topology(primitive_topology::type topology)
{
    assert(topology < primitive_topology::count);

    switch (topology)
    {
    case primitive_topology::point_list:		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case primitive_topology::line_list:			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case primitive_topology::line_strip:		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case primitive_topology::triangle_list:		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case primitive_topology::triangle_strip:	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    }

    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

#pragma intrinsic(_BitScanForward)
shader_type::type
get_shader_type(u32 flag)
{
    assert(flag);
    unsigned long index;
    _BitScanForward(&index, flag);
    return (shader_type::type)index;
}

id::id_type
create_pso(id::id_type material_id, u32 elements_type)
{
    const d3d11_material_stream material{ materials[material_id].get() };
    const shader_flags::flags flags{ material.shader_flags() };

    struct shader_bytecode
    {
        const u8*			code;
        u64					size;
        u64					hash;
    };

    shader_bytecode shaders[shader_type::count]{};
    u32 shader_index{ 0 };
    for (u32 i{ 0 }; i < shader_type::count; ++i)
    {
        if (flags & (1 << i))
        {
            const u32 key{ get_shader_type(flags & (1 << i)) == shader_type::vertex ? elements_type : u32_invalid_id };
            primal::content::compiled_shader_ptr shader{ primal::content::get_shader(material.shader_ids()[shader_index], key) };
            assert(shader);
            shaders[i].code = shader->byte_code();
            shaders[i].size = shader->byte_code_size();
            shaders[i].hash = math::calc_crc32_u64(shader->hash(), shader->hash_length);
            ++shader_index;
        }
    }

    d3d11_pso_storage storage{};
    ID3D11Device5* const device{ core::device() };

    if (shaders[shader_type::vertex].code)
    {
        shader_bytecode& bytecode{ shaders[shader_type::vertex] };

        auto pair = vs_map.find(bytecode.hash);

        if (pair != vs_map.end())
        {
            assert(pair->first == bytecode.hash);
            storage.vs_id = pair->second;
        }
        else
        {
            ID3D11VertexShader* vs{ nullptr };
            DXCall(device->CreateVertexShader(bytecode.code, bytecode.size, nullptr, &vs));

            storage.vs_id = (id::id_type)vertex_shaders.size();
            vs_map[bytecode.hash] = storage.vs_id;
            vertex_shaders.emplace_back(vs);
        }
    }

    if (shaders[shader_type::hull].code)
    {
        shader_bytecode& bytecode{ shaders[shader_type::hull] };

        auto pair = hs_map.find(bytecode.hash);

        if (pair != hs_map.end())
        {
            assert(pair->first == bytecode.hash);
            storage.hs_id = pair->second;
        }
        else
        {
            ID3D11HullShader* hs{ nullptr };
            DXCall(device->CreateHullShader(bytecode.code, bytecode.size, nullptr, &hs));

            storage.hs_id = (id::id_type)hull_shaders.size();
            hs_map[bytecode.hash] = storage.hs_id;
            hull_shaders.emplace_back(hs);
        }
    }

    if (shaders[shader_type::domain].code)
    {
        shader_bytecode& bytecode{ shaders[shader_type::domain] };

        auto pair = ds_map.find(bytecode.hash);

        if (pair != ds_map.end())
        {
            assert(pair->first == bytecode.hash);
            storage.ds_id = pair->second;
        }
        else
        {
            ID3D11DomainShader* ds{ nullptr };
            DXCall(device->CreateDomainShader(bytecode.code, bytecode.size, nullptr, &ds));

            storage.ds_id = (id::id_type)domain_shaders.size();
            ds_map[bytecode.hash] = storage.ds_id;
            domain_shaders.emplace_back(ds);
        }
    }

    if (shaders[shader_type::geometry].code)
    {
        shader_bytecode& bytecode{ shaders[shader_type::geometry] };

        auto pair = gs_map.find(bytecode.hash);

        if (pair != gs_map.end())
        {
            assert(pair->first == bytecode.hash);
            storage.gs_id = pair->second;
        }
        else
        {
            ID3D11GeometryShader* gs{ nullptr };
            DXCall(device->CreateGeometryShader(bytecode.code, bytecode.size, nullptr, &gs));

            storage.gs_id = (id::id_type)geometry_shaders.size();
            gs_map[bytecode.hash] = storage.gs_id;
            geometry_shaders.emplace_back(gs);
        }
    }

    if (shaders[shader_type::pixel].code)
    {
        shader_bytecode& bytecode{ shaders[shader_type::pixel] };

        auto pair = ps_map.find(bytecode.hash);

        if (pair != ps_map.end())
        {
            assert(pair->first == bytecode.hash);
            storage.ps_id = pair->second;
        }
        else
        {
            ID3D11PixelShader* ps{ nullptr };
            DXCall(device->CreatePixelShader(bytecode.code, bytecode.size, nullptr, &ps));

            storage.ps_id = (id::id_type)pixel_shaders.size();
            ps_map[bytecode.hash] = storage.ps_id;
            pixel_shaders.emplace_back(ps);
        }
    }

    d3d11_pipeline_state pso{};
    if (id::is_valid(storage.vs_id)) pso.vs = vertex_shaders[storage.vs_id];
    if (id::is_valid(storage.hs_id)) pso.hs = hull_shaders[storage.hs_id];
    if (id::is_valid(storage.ds_id)) pso.ds = domain_shaders[storage.ds_id];
    if (id::is_valid(storage.gs_id)) pso.gs = geometry_shaders[storage.gs_id];
    if (id::is_valid(storage.ps_id)) pso.ps = pixel_shaders[storage.ps_id];

    pso_storages.emplace_back(storage);
  
    const u32 id{ (u32)pipeline_states.size() };
    
    pipeline_states.emplace_back(pso);

    return id;
}

d3d11_texture
create_resource_from_texture_data(const u8* const data)
{
    assert(data);
    utl::blob_stream_reader blob{ data };
    const u32 width{ blob.read<u32>() };
    const u32 height{ blob.read<u32>() };
    u32 depth{ 1 };
    u32 array_size{ blob.read<u32>() };
    const u32 flags{ blob.read<u32>() };
    const u32 mip_levels{ blob.read<u32>() };
    const DXGI_FORMAT format{ (DXGI_FORMAT)blob.read<u32>() };
    const bool is_3d{ (flags & primal::content::texture_flags::is_volume_map) != 0 };

    assert(mip_levels <= d3d11_texture::max_mips);

    u32 depth_per_mip_level[d3d11_texture::max_mips]{};
    for (u32 i{ 0 }; i < d3d11_texture::max_mips; ++i)
    {
        depth_per_mip_level[i] = 1;
    }

    if (is_3d)
    {
        depth = array_size;
        array_size = 1;
        u32 depth_per_mip{ depth };

        for (u32 i{ 0 }; i < mip_levels; ++i)
        {
            depth_per_mip_level[i] = depth_per_mip;
            depth_per_mip = std::max(depth_per_mip >> 1, (u32)1);
        }
    }

    utl::vector<D3D11_SUBRESOURCE_DATA> subresources{};

    for (u32 i{ 0 }; i < array_size; ++i)
    {
        for (u32 j{ 0 }; j < mip_levels; ++j)
        {
            const u32 row_pitch{ blob.read<u32>() };
            const u32 slice_pitch{ blob.read<u32>() };

            subresources.emplace_back(D3D11_SUBRESOURCE_DATA
                {
                    blob.position(),
                    row_pitch,
                    slice_pitch
                });

            blob.skip(slice_pitch * depth_per_mip_level[j]);
        }
    }

    D3D11_TEXTURE2D_DESC desc2d{};
    D3D11_TEXTURE3D_DESC desc3d{};

    d3d11_texture_init_info info{};
    info.dimension = is_3d ? texture_dimension::texture_3d : texture_dimension::texture_2d;

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};

    if (is_3d)
    {
        desc3d.Width = width;
        desc3d.Height = height;
        desc3d.Depth = depth;
        desc3d.MipLevels = mip_levels;
        desc3d.Format = format;
        desc3d.Usage = D3D11_USAGE_DEFAULT;
        desc3d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc3d.CPUAccessFlags = 0;
        desc3d.MiscFlags = 0;

        info.desc3d = &desc3d;
    }
    else//2D
    {
        desc2d.Width = width;
        desc2d.Height = height;
        desc2d.MipLevels = mip_levels;
        desc2d.ArraySize = array_size;
        desc2d.Format = format;
        desc2d.SampleDesc = { 1, 0 };
        desc2d.Usage = D3D11_USAGE_DEFAULT;
        desc2d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc2d.CPUAccessFlags = 0;
        desc2d.MiscFlags = 0;

        info.desc2d = &desc2d;
    }

    if (flags & primal::content::texture_flags::is_cube_map)
    {
        assert(array_size % 6 == 0);
        
        info.desc2d->MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        
        srv_desc.Format = format;

        if (array_size > 6)
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
            srv_desc.TextureCubeArray.MostDetailedMip = 0;
            srv_desc.TextureCubeArray.MipLevels = mip_levels;
            srv_desc.TextureCubeArray.NumCubes = array_size / 6;
        }
        else
        {
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srv_desc.TextureCube.MostDetailedMip = 0;
            srv_desc.TextureCube.MipLevels = mip_levels;
        }

        info.srv_desc = &srv_desc;
    }

    info.initial_data = &subresources[0];//????

    return d3d11_texture{ info };
}
}//anonymous namespace

bool
initialize()
{
    return true;
}

void
shutdown()
{
    for (auto& shader : vertex_shaders) core::release(shader);
    for (auto& shader : hull_shaders) core::release(shader);
    for (auto& shader : domain_shaders) core::release(shader);
    for (auto& shader : geometry_shaders) core::release(shader);
    for (auto& shader : pixel_shaders) core::release(shader);

    vs_map.clear();
    hs_map.clear();
    ds_map.clear();
    gs_map.clear();
    ps_map.clear();
    pso_storages.clear();
    pipeline_states.clear();

    assert(submesh_views.empty());
}

namespace submesh {
id::id_type
add(const u8*& data)
{
    utl::blob_stream_reader blob((const u8*)data);

    const u32 element_size{ blob.read<u32>() };
    const u32 vertex_count{ blob.read<u32>() };
    const u32 index_count{ blob.read<u32>() };
    const u32 elements_type{ blob.read<u32>() };
    const u32 primitive_topology{ blob.read<u32>() };
    const u32 index_size{ (vertex_count < (1 << 16)) ? sizeof(u16) : sizeof(u32) };

    const u32 position_buffer_size{ sizeof(math::v3) * vertex_count };
    const u32 element_buffer_size{ element_size * vertex_count };
    const u32 index_buffer_size{ index_size * index_count };

    constexpr u32 alignment{ D3D11_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE };
    const u32 aligned_position_buffer_size{ (u32)math::align_size_up<alignment>(position_buffer_size) };
    const u32 aligned_element_buffer_size{ (u32)math::align_size_up<alignment>(element_buffer_size) };

    submesh_view view{};
    auto* const device{ core::device() };

    view.index_format = (index_size == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    view.index_count = index_count;
    view.elements_type = elements_type;
    view.primitive_topology = get_d3d_primitive_topology((primitive_topology::type)primitive_topology);

    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = aligned_position_buffer_size;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.StructureByteStride = sizeof(math::v3);
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

        std::unique_ptr<u8[]> pos_buffer = std::make_unique<u8[]>(aligned_position_buffer_size);

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = pos_buffer.get();
        blob.read((u8*)srd.pSysMem, aligned_position_buffer_size);
        srd.SysMemPitch = aligned_position_buffer_size;

        DXCall(device->CreateBuffer(&desc, &srd, &view.position_buffer));

        assert(view.position_buffer);
        if (!view.position_buffer) return id::invalid_id;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc{};
        srvdesc.Format = DXGI_FORMAT_UNKNOWN;
        srvdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvdesc.Buffer.FirstElement = 0;
        srvdesc.Buffer.NumElements = vertex_count;

        core::device()->CreateShaderResourceView(view.position_buffer, &srvdesc, &view.position_view);
    }
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = aligned_element_buffer_size;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.StructureByteStride = element_size;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

        std::unique_ptr<u8[]> element_buffer = std::make_unique<u8[]>(aligned_element_buffer_size);

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = element_buffer.get();
        blob.read((u8*)srd.pSysMem, aligned_element_buffer_size);
        srd.SysMemPitch = aligned_element_buffer_size;

        DXCall(device->CreateBuffer(&desc, &srd, &view.element_buffer));

        assert(view.element_buffer);
        if (!view.element_buffer) return id::invalid_id;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc{};
        srvdesc.Format = DXGI_FORMAT_UNKNOWN;
        srvdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvdesc.Buffer.FirstElement = 0;
        srvdesc.Buffer.NumElements = vertex_count;

        core::device()->CreateShaderResourceView(view.element_buffer, &srvdesc, &view.element_view);
    }
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = index_buffer_size;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        std::unique_ptr<u8[]> ind_buffer = std::make_unique<u8[]>(index_buffer_size);

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = ind_buffer.get();
        blob.read((u8*)srd.pSysMem, index_buffer_size);
        srd.SysMemPitch = index_buffer_size;

        DXCall(device->CreateBuffer(&desc, &srd, &view.index_buffer));
    }

    std::lock_guard lock{ submesh_mutex };
    return submesh_views.add(view);
}

void
remove(id::id_type id)
{
    std::lock_guard lock{ submesh_mutex };
    submesh_view& view{ submesh_views[id] };

    core::deferred_release(view.position_buffer);
    core::deferred_release(view.element_buffer);
    core::deferred_release(view.index_buffer);
    core::deferred_release(view.position_view);
    core::deferred_release(view.element_view);

    submesh_views.remove(id);
}

void
get_views(const id::id_type* const gpu_ids, u32 id_count, const views_cache& cache)
{
    assert(gpu_ids && id_count);
    assert(cache.index_buffers && cache.primitive_topologies && cache.elements_types);

    std::lock_guard lock{ submesh_mutex };

    for (u32 i{ 0 }; i < id_count; ++i)
    {
        const submesh_view& view{ submesh_views[gpu_ids[i]] };
        cache.index_buffers[i] = view.index_buffer;
        cache.index_formats[i] = view.index_format;
        cache.position_views[i] = view.position_view;
        cache.element_views[i] = view.element_view;
        cache.primitive_topologies[i] = view.primitive_topology;
        cache.elements_types[i] = view.elements_type;
        cache.index_counts[i] = view.index_count;
    }
}
}//submesh namespace

namespace texture {
id::id_type
add(const u8* const data)
{
    assert(data);

    d3d11_texture texture{ create_resource_from_texture_data(data) };

    std::lock_guard lock{ texture_mutex };
    const id::id_type id{ textures.add(std::move(texture)) };
    texture_pointers.add(textures[id].srv());

    return id;
}

void
remove(id::id_type id)
{
    std::lock_guard lock{ texture_mutex };

    textures.remove(id);
    texture_pointers.remove(id);
}

void
get_shader_views(const id::id_type* const texture_ids, u32 id_count, ID3D11ShaderResourceView** const views)
{
    assert(texture_ids && id_count && views);
    std::lock_guard lock{ texture_mutex };
    for (u32 i{ 0 }; i < id_count; ++i)
    {
        views[i] = texture_pointers[texture_ids[i]];
    }
}
}//texture namespace

namespace material {
id::id_type
add(material_init_info info)
{
    std::unique_ptr<u8[]> buffer;
    std::lock_guard lock{ material_mutex };

    d3d11_material_stream stream{ buffer, info };

    assert(buffer);
    return materials.add(std::move(buffer));
}

void
remove(id::id_type id)
{
    std::lock_guard lock{ material_mutex };
    materials.remove(id);
}

void
get_materials(const id::id_type* const material_ids, u32 material_count, const materials_cache& cache, u32& shader_view_count)
{
    assert(material_ids && material_count);
    assert(cache.material_types);
    std::lock_guard lock{ material_mutex };

    u32 total_view_count{ 0 };

    for (u32 i{ 0 }; i < material_count; ++i)
    {
        const d3d11_material_stream stream{ materials[material_ids[i]].get() };
        cache.material_types[i] = stream.material_type();
        cache.shader_views[i] = stream.shader_views();
        cache.texture_counts[i] = stream.texture_count();
        cache.material_surfaces[i] = stream.surface();
        total_view_count += stream.texture_count();
    }

    shader_view_count = total_view_count;
}
}//material namespace

namespace render_item {
id::id_type
add(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count, const id::id_type* const material_ids)
{
    assert(id::is_valid(entity_id) && id::is_valid(geometry_content_id));
    assert(material_count && material_ids);
    id::id_type* const gpu_ids{ (id::id_type* const)_alloca(material_count * sizeof(id::id_type)) };
    primal::content::get_submesh_gpu_ids(geometry_content_id, material_count, gpu_ids);

    submesh::views_cache views_cache
    {
        (ID3D11Buffer** const)_alloca(material_count * sizeof(ID3D11Buffer*)),
        (ID3D11ShaderResourceView** const)_alloca(material_count * sizeof(ID3D11ShaderResourceView*)),
        (ID3D11ShaderResourceView** const)_alloca(material_count * sizeof(ID3D11ShaderResourceView*)),
        (D3D_PRIMITIVE_TOPOLOGY* const)_alloca(material_count * sizeof(D3D_PRIMITIVE_TOPOLOGY)),
        (DXGI_FORMAT* const)_alloca(material_count * sizeof(DXGI_FORMAT)),
        (u32* const)_alloca(material_count * sizeof(u32)),
        (u32* const)_alloca(material_count * sizeof(u32))
    };

    submesh::get_views(gpu_ids, material_count, views_cache);

    if (!gpu_ids)
        return id::invalid_id;

    std::unique_ptr<id::id_type[]> items{ std::make_unique<id::id_type[]>(sizeof(id::id_type) * (1 + (u64)material_count + 1)) };

    items[0] = geometry_content_id;
    id::id_type* const items_ids{ &items[1] };

    std::lock_guard lock{ render_item_mutex };

    for (u32 i{ 0 }; i < material_count; ++i)
    {
        d3d11_render_item item{};
        item.entity_id = entity_id;
        item.submesh_gpu_id = gpu_ids[i];
        item.material_id = material_ids[i];
        item.pso_id = create_pso(material_ids[i], views_cache.elements_types[i]);

        assert(id::is_valid(item.submesh_gpu_id) && id::is_valid(item.material_id));
        items_ids[i] = render_items.add(item);
    }

    items_ids[material_count] = id::invalid_id;

    return render_item_ids.add(std::move(items));
}

void
remove(id::id_type id)
{
    std::lock_guard lock{ render_item_mutex };
    const id::id_type* const item_ids{ &render_item_ids[id][1] };

    for (u32 i{ 0 }; item_ids[i] != id::invalid_id; ++i)
    {
        render_items.remove(item_ids[i]);
    }

    render_item_ids.remove(id);
}

void
get_d3d11_render_item_ids(const frame_info& info, utl::vector<id::id_type>& d3d11_render_item_ids)
{
    assert(info.render_item_ids && info.thresholds && info.render_item_count);
    assert(d3d11_render_item_ids.empty());

    frame_cache.lod_offsets.clear();
    frame_cache.geometry_ids.clear();
    const u32 count(info.render_item_count);

    std::lock_guard lock{ render_item_mutex };

    for (u32 i{ 0 }; i < count; ++i)
    {
        const id::id_type* const buffer{ render_item_ids[info.render_item_ids[i]].get() };
        frame_cache.geometry_ids.emplace_back(buffer[0]);
    }

    primal::content::get_lod_offsets(frame_cache.geometry_ids.data(), info.thresholds, count, frame_cache.lod_offsets);
    assert(frame_cache.lod_offsets.size() == count);

    u32 d3d11_render_item_count{ 0 };
    for (u32 i{ 0 }; i < count; ++i)
    {
        d3d11_render_item_count += frame_cache.lod_offsets[i].count;
    }

    assert(d3d11_render_item_count);
    d3d11_render_item_ids.resize(d3d11_render_item_count);

    u32 item_index{ 0 };
    for (u32 i{ 0 }; i < count; ++i)
    {
        const id::id_type* const item_ids{ &render_item_ids[info.render_item_ids[i]][1] };
        const primal::content::lod_offset& lod_offset{ frame_cache.lod_offsets[i] };
        memcpy(&d3d11_render_item_ids[item_index], &item_ids[lod_offset.offset], sizeof(id::id_type) * lod_offset.count);
        item_index += lod_offset.count;
        assert(item_index <= d3d11_render_item_count);
    }

    assert(item_index <= d3d11_render_item_count);
}

void
get_items(const id::id_type* const d3d11_render_item_ids, u32 id_count, const items_cache& cache)
{
    assert(d3d11_render_item_ids && id_count);
    assert(cache.entity_ids && cache.submesh_gpu_ids && cache.material_ids &&
        cache.psos);

    std::lock_guard lock1{ render_item_mutex };
    std::lock_guard lock2{ pso_mutex };

    for (u32 i{ 0 }; i < id_count; ++i)
    {
        const d3d11_render_item& item{ render_items[d3d11_render_item_ids[i]] };
        cache.entity_ids[i] = item.entity_id;
        cache.submesh_gpu_ids[i] = item.submesh_gpu_id;
        cache.material_ids[i] = item.material_id;
        cache.psos[i] = pipeline_states[item.pso_id];
    }
}
}//render_item namespace
}

#endif
