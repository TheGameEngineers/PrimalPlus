#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
struct d3d11_buffer_init_info
{
    const void* const			data;
    u32							size{ 0 };
    u32							alignment{ D3D11_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE };
    u32							cpu_access_flags{ 0 };
    u32							bind_flags{ 0 };
    u32							misc_flags{ 0 };
    D3D11_USAGE					usage{ D3D11_USAGE_DEFAULT };
};

class d3d11_buffer
{
public:
    d3d11_buffer() = default;
    DISABLE_COPY(d3d11_buffer);  
    explicit d3d11_buffer(const d3d11_buffer_init_info& info);
    constexpr d3d11_buffer(d3d11_buffer&& o) noexcept
        : _buffer{ o._buffer }, _size{ o._size }
    {
        o.reset();
    }

    constexpr d3d11_buffer& operator=(d3d11_buffer&& o) noexcept
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }

    ~d3d11_buffer() { release(); }

    void release();

    _NODISCARD constexpr ID3D11Buffer* const buffer() const { return _buffer; }
    _NODISCARD constexpr u32 size() const { return _size; }

private:
    constexpr void move(d3d11_buffer& o)
    {
        _buffer = o._buffer;
        _size = o._size;
        o.reset();
    }

    constexpr void reset()
    {
        _buffer = nullptr;
        _size = 0;
    }

    ID3D11Buffer*		_buffer{ nullptr };
    u32					_size{ 0 };
};

class constant_buffer
{
public:
    constant_buffer() = default;
    explicit constant_buffer(const d3d11_buffer_init_info& info, ID3D11DeviceContext4* const ctx);
    DISABLE_COPY_AND_MOVE(constant_buffer);
    ~constant_buffer() { release(); }

    void release()
    {
        _buffer.release();
        _cpu_address = nullptr;
        _cpu_offset = 0;
    }

    constexpr void clear() { _cpu_offset = 0; }
    _NODISCARD u8* const allocate(u32 size);

    template<typename T>
    _NODISCARD T* const allocate()
    {
        return (T* const)allocate(sizeof(T));
    }

    _NODISCARD constexpr ID3D11Buffer* const buffer() const { return _buffer.buffer(); }
    _NODISCARD constexpr u32 size() const { return _buffer.size(); }
    _NODISCARD constexpr u8* const cpu_address() const { return _cpu_address; }

    template<typename T>
    _NODISCARD constexpr UINT offset(T* const allocation)
    {
        std::lock_guard lock{ _mutex };
        assert(_cpu_address);
        if (!_cpu_address) return {};
        const u8* const address{ (const u8* const)allocation };
        assert(address <= _cpu_address + _cpu_offset);
        assert(address >= _cpu_address);
        const UINT offset{ (UINT)(address - _cpu_address) };
        return offset / 16;
    }

    _NODISCARD constexpr static d3d11_buffer_init_info get_default_init_info(u32 size)
    {
        assert(size);
        d3d11_buffer_init_info info{};
        info.size = size;
        info.bind_flags = D3D11_BIND_CONSTANT_BUFFER;
        info.cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        info.usage = D3D11_USAGE_DYNAMIC;
        info.alignment = PRIMAL_D3D11_CONSTANT_BUFFER_ALIGNMENT;
        return info;
    }

private:
    d3d11_buffer						_buffer{};
    u8*									_cpu_address{ nullptr };
    u32									_cpu_offset{ 0 };
    std::mutex							_mutex{};
};

class uav_clearable_buffer
{
public:
    uav_clearable_buffer() = default;
    DISABLE_COPY(uav_clearable_buffer);
    explicit uav_clearable_buffer(const d3d11_buffer_init_info& info);
    constexpr uav_clearable_buffer(uav_clearable_buffer&& o) noexcept
        : _buffer{ o._buffer }, _uav{ o._uav }
    {
        o.reset();
    }

    constexpr uav_clearable_buffer& operator=(uav_clearable_buffer&& o) noexcept
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }

    ~uav_clearable_buffer() { release(); }

    void release();
    void clear_uav(ID3D11DeviceContext4* const ctx, const u32* const values) const
    {
        assert(_buffer);
        assert(_uav);
        ctx->ClearUnorderedAccessViewUint(_uav, values);
    }

    void clear_uav(ID3D11DeviceContext4* const ctx, const f32* const values) const
    {
        assert(_buffer);
        assert(_uav);
        ctx->ClearUnorderedAccessViewFloat(_uav, values);
    }

    _NODISCARD constexpr ID3D11Buffer* const buffer() const { return _buffer; }
    _NODISCARD constexpr ID3D11UnorderedAccessView* const uav() const { return _uav; }
    _NODISCARD constexpr static d3d11_buffer_init_info get_default_init_info(u32 size, u32 alignment)
    {
        assert(size);
        d3d11_buffer_init_info info{};
        info.size = size;
        info.bind_flags = D3D11_BIND_UNORDERED_ACCESS;
        info.misc_flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
        info.alignment = alignment;
        return info;
    }

private:
    constexpr void move(uav_clearable_buffer& o)
    {
        _buffer = o._buffer;
        _uav = o._uav;
        o.reset();
    }

    constexpr void reset()
    {
        _buffer = nullptr;
        _uav = nullptr;
    }

    ID3D11Buffer*				_buffer{ nullptr };
    ID3D11UnorderedAccessView*	_uav{ nullptr };
};

struct texture_dimension {
    enum dimension : u32 {
        texture_1d = 0,
        texture_2d,
        texture_3d,
    };
};

struct d3d11_texture_init_info
{
	ID3D11Texture2D*					texture{ nullptr };
	D3D11_TEXTURE2D_DESC*				desc{ nullptr };
    texture_dimension::dimension        dimension{};
    union {
        ID3D11Texture1D*                texture1d;
        ID3D11Texture2D*                texture2d{ nullptr };
        ID3D11Texture3D*                texture3d;
    };
    union {
        D3D11_TEXTURE1D_DESC*				desc1d;
        D3D11_TEXTURE2D_DESC*				desc2d{ nullptr };
        D3D11_TEXTURE3D_DESC*				desc3d;
    };
    D3D11_SHADER_RESOURCE_VIEW_DESC*	srv_desc{ nullptr };
    D3D11_SUBRESOURCE_DATA*				initial_data{ nullptr };
};

class d3d11_texture
{
public:
    constexpr static u32 max_mips{ 14 };
    d3d11_texture() = default;
    DISABLE_COPY(d3d11_texture);
    explicit d3d11_texture(const d3d11_texture_init_info& info);
    constexpr d3d11_texture(d3d11_texture&& o) noexcept
		: _texture(o._texture), _srv(o._srv), _dimension(o._dimension)
    {
        if (_dimension == texture_dimension::texture_1d)
        {
            _texture1d = o._texture1d;
        }
        else if (_dimension == texture_dimension::texture_2d)
        {
            _texture2d = o._texture2d;
        }
        else
        {
            _texture3d = o._texture3d;
        }

        o.reset();
    }

    constexpr d3d11_texture& operator=(d3d11_texture&& o) noexcept
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }

    ~d3d11_texture() { release(); }

    void release();

    _NODISCARD constexpr texture_dimension::dimension const dimension() const { return _dimension; }
    _NODISCARD constexpr ID3D11ShaderResourceView* const srv() const { return _srv; }
    _NODISCARD constexpr void* const resource() const
    {
        //Most common first
        if (_dimension == texture_dimension::texture_2d)
        {
            return _texture2d;
        }
        else if (_dimension == texture_dimension::texture_3d)
        {
            return _texture3d;
        }
        else
        {
            return _texture1d;
        }
    }

private:
    constexpr void move(d3d11_texture& o)
    {
        if (_dimension == texture_dimension::texture_1d)
        {
            _texture1d = o._texture1d;
        }
        else if (_dimension == texture_dimension::texture_2d)
        {
            _texture2d = o._texture2d;
        }
        else
        {
            _texture3d = o._texture3d;
        }
      
        _srv = o._srv;
        o.reset();
    }

    constexpr void reset()
    {
        _texture1d = nullptr;
        _texture2d = nullptr;
        _texture3d = nullptr;
        _srv = nullptr;
    }

	ID3D11Texture2D*			_texture{ nullptr };
	ID3D11ShaderResourceView*	_srv{ nullptr };
    union {
        ID3D11Texture1D*			_texture1d;
        ID3D11Texture2D*			_texture2d{ nullptr };
        ID3D11Texture3D*			_texture3d;
    };
    texture_dimension::dimension    _dimension{};
    ID3D11ShaderResourceView*	    _srv{ nullptr };
};

class d3d11_render_texture
{
public:
    d3d11_render_texture() = default;
    DISABLE_COPY(d3d11_render_texture);
    explicit d3d11_render_texture(const d3d11_texture_init_info& info);

    constexpr d3d11_render_texture(d3d11_render_texture&& o) noexcept
        : _texture(std::move(o._texture))
    {
        for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = o._rtv[i];
        o.reset();
    }

    constexpr d3d11_render_texture& operator=(d3d11_render_texture&& o) noexcept
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }

    ~d3d11_render_texture() { release(); }

    void release();

    _NODISCARD constexpr texture_dimension::dimension const dimension() const { return _texture.dimension(); }
    _NODISCARD constexpr ID3D11Texture2D* const resource() const { return (ID3D11Texture2D* const)_texture.resource(); }
    _NODISCARD constexpr ID3D11ShaderResourceView* const srv() const { return _texture.srv(); }
    _NODISCARD constexpr ID3D11RenderTargetView* const rtv(u32 mip_index) const { assert(mip_index <= _mip_count); return _rtv[mip_index]; }

private:
    constexpr void move(d3d11_render_texture& o)
    {
        _texture = std::move(o._texture);
        _mip_count = o._mip_count;
        for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = o._rtv[i];
        o.reset();
    }

    constexpr void reset()
    {
        for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = nullptr;
        _mip_count = 0;
    }

    d3d11_texture				_texture;
    ID3D11RenderTargetView*		_rtv[d3d11_texture::max_mips]{ nullptr };
    u32							_mip_count{ 0 };
};

class d3d11_depth_buffer
{
public:
    d3d11_depth_buffer() = default;
    explicit d3d11_depth_buffer(d3d11_texture_init_info info);
    DISABLE_COPY(d3d11_depth_buffer);
    constexpr d3d11_depth_buffer(d3d11_depth_buffer&& o) noexcept
        : _texture{ std::move(o._texture) }, _dsv{ o._dsv }
    {
        o._dsv = {};
    }

    constexpr d3d11_depth_buffer& operator=(d3d11_depth_buffer&& o) noexcept
    {
        assert(this != &o);
        if (this != &o)
        {
            _texture = std::move(o._texture);
            _dsv = o._dsv;
            o._dsv = {};
        }
        return *this;
    }

    ~d3d11_depth_buffer() { release(); }

    void release();

    _NODISCARD constexpr texture_dimension::dimension const dimension() const { return _texture.dimension(); }
    _NODISCARD constexpr ID3D11Texture2D* const resource() const { return (ID3D11Texture2D* const)_texture.resource(); }
    _NODISCARD constexpr ID3D11ShaderResourceView* const srv() const { return _texture.srv(); }
    _NODISCARD constexpr ID3D11DepthStencilView* const dsv() const { return _dsv; }

private:
    d3d11_texture			_texture{};
    ID3D11DepthStencilView* _dsv{};
};
}

#endif
