#include "D3D11Resources.h"
#include "D3D11Core.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
////////// D3D11 BUFFER /////////////////////////////////////////////////////
d3d11_buffer::d3d11_buffer(d3d11_buffer_init_info info)
{
    _size = (u32)math::align_size_up(info.size, info.alignment);
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = _size;
    desc.Usage = info.usage;
    desc.BindFlags = info.bind_flags;
    desc.CPUAccessFlags = info.cpu_access_flags;
    desc.MiscFlags = info.misc_flags;
    desc.StructureByteStride = info.alignment;

    D3D11_SUBRESOURCE_DATA subresource_data{};
    subresource_data.pSysMem = info.data;
    subresource_data.SysMemPitch = info.data ? info.size : 0;

    HRESULT hr{ core::device()->CreateBuffer(&desc, info.data ? &subresource_data : nullptr, &_buffer) };
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create D3D11 Buffer");
    }
}

void
d3d11_buffer::release()
{
    core::deferred_release(_buffer);
}

////////// CONSTANT BUFFER /////////////////////////////////////////////////////
constant_buffer::constant_buffer(d3d11_buffer_init_info info, ID3D11DeviceContext4* const ctx)
    : _buffer{ info }
{
    D3D11_MAPPED_SUBRESOURCE map{};
    ctx->Map(_buffer.buffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    _cpu_address = (u8*)map.pData;
}

u8* const
constant_buffer::allocate(u32 size)
{
    std::lock_guard lock{ _mutex };
    const u32 aligned_size{ (u32)d3dx::align_size_for_constant_buffer(size) };
    assert(_cpu_offset + aligned_size <= _buffer.size());
    if (_cpu_offset + aligned_size <= _buffer.size())
    {
        u8* const address{ _cpu_address + _cpu_offset };
        _cpu_offset += aligned_size;
        return address;
    }

    return nullptr;
}

////////// STRUCTURED BUFFER /////////////////////////////////////////////////////
uav_clearable_buffer::uav_clearable_buffer(const d3d11_buffer_init_info& info)
{
    auto* const device{ core::device() };

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = info.alignment != 0 ? (u32)math::align_size_up(info.size, info.alignment) : info.size;
    desc.Usage = info.usage;
    desc.BindFlags = info.bind_flags;
    desc.CPUAccessFlags = info.cpu_access_flags;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = info.alignment;

    D3D11_SUBRESOURCE_DATA subresource_data{};
    subresource_data.pSysMem = info.data;
    subresource_data.SysMemPitch = info.data ? info.size : 0;

    HRESULT hr{ device->CreateBuffer(&desc, info.data ? &subresource_data : nullptr, &_buffer) };
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create D3D11 Buffer");
    }
    assert(_buffer);

    D3D11_UNORDERED_ACCESS_VIEW_DESC vd{};
    vd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    vd.Buffer.FirstElement = 0;
    vd.Buffer.NumElements = 1;// info.size / info.alignment;

    device->CreateUnorderedAccessView(_buffer, &vd, &_uav);
}

void
uav_clearable_buffer::release()
{
    core::deferred_release(_buffer);
    core::deferred_release(_uav);
}

////////// D3D11 TEXTURE /////////////////////////////////////////////////////
d3d11_texture::d3d11_texture(const d3d11_texture_init_info& info)
{
    auto* const device{ core::device() };
    
    if (info.texture)
    {
        _texture = info.texture;
    }
    else if (info.desc)
    {
        DXCall(device->CreateTexture2D(info.desc, info.initial_data, &_texture));
    }

    assert(_texture);
    if (!_texture) return;
    DXCall(device->CreateShaderResourceView(_texture, info.srv_desc, &_srv));
}

void
d3d11_texture::release()
{
    core::deferred_release(_texture);
    core::deferred_release(_srv);
}

////////// RENDER TEXTURE /////////////////////////////////////////////////////
d3d11_render_texture::d3d11_render_texture(const d3d11_texture_init_info& info)
    : _texture{ info }
{
    assert(info.desc);

    {
        D3D11_TEXTURE2D_DESC desc;
        desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
        resource()->GetDesc(&desc);
        _mip_count = desc.MipLevels;
        assert(_mip_count && _mip_count < d3d11_texture::max_mips);
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Format = info.desc->Format;
    rtv_desc.Texture2D.MipSlice = 0;

    auto* const device{ core::device() };

    for (u32 i{ 0 }; i < _mip_count; ++i)
    {
        device->CreateRenderTargetView(resource(), &rtv_desc, &_rtv[i]);
        ++rtv_desc.Texture2D.MipSlice;
    }
}

void
d3d11_render_texture::release()
{
    for (u32 i{ 0 }; i < _mip_count; ++i)
    {
        core::deferred_release(_rtv[i]);
    }
    _texture.release();
    _mip_count = 0;
}

////////// DEPTH BUFFER /////////////////////////////////////////////////////
d3d11_depth_buffer::d3d11_depth_buffer(d3d11_texture_init_info info)
{
    assert(info.desc);
    const DXGI_FORMAT dsv_format{ info.desc->Format };

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    if (info.desc->Format == DXGI_FORMAT_D32_FLOAT)
    {
        info.desc->Format = DXGI_FORMAT_R32_TYPELESS;
        srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    }

    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Texture2D.MostDetailedMip = 0;

    assert(!info.srv_desc && !info.texture);
    info.srv_desc = &srv_desc;
    _texture = d3d11_texture(info);

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags = 0;
    dsv_desc.Format = dsv_format;
    dsv_desc.Texture2D.MipSlice = 0;

    auto* const device{ core::device() };
    assert(device);
    device->CreateDepthStencilView(resource(), &dsv_desc, &_dsv);
}

void
d3d11_depth_buffer::release()
{
    core::deferred_release(_dsv);
    _texture.release();
}
}

#endif
