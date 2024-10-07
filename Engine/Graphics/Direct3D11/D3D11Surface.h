#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
class d3d11_surface
{
public:
    constexpr static u32 buffer_count{ 3 };
    constexpr static DXGI_FORMAT default_back_buffer_format{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };

    explicit d3d11_surface(platform::window window)
        : _window(window)
    {
        assert(_window.handle());
    }

#if USE_STL_VECTOR
    DISABLE_COPY(d3d11_surface)
        constexpr d3d11_surface(d3d11_surface&& o) noexcept
        : _swap_chain(o._swap_chain), _rtv(o._rtv), _window(o._window),
        _allow_tearing(o._allow_tearing), _present_flags(o._present_flags),
        _viewport(o._viewport), _scissor_rect(o._scissor_rect), _light_culling_id(o._light_culling_id)
    {
        o.reset();
    }

    constexpr d3d11_surface& operator=(d3d11_surface&& o) noexcept
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }
#else
    DISABLE_COPY_AND_MOVE(d3d11_surface)
#endif

    ~d3d11_surface() { release(); }

    bool create_swap_chain(IDXGIFactory7* factory);
    void present() const;
    void resize();

    [[nodiscard]] constexpr u32 width() const { return(u32)_viewport.Width; }
    [[nodiscard]] constexpr u32 height() const { return(u32)_viewport.Height; }
    [[nodiscard]] constexpr ID3D11RenderTargetView*& rtv() { return _rtv; }
    [[nodiscard]] constexpr const D3D11_VIEWPORT& viewport() const { return _viewport; }
    [[nodiscard]] constexpr const D3D11_RECT& scissor_rect() const { return _scissor_rect; }
    [[nodiscard]] constexpr id::id_type light_culling_id() const { return _light_culling_id; }

private:
    void finalize();
    void release();

#if USE_STL_VECTOR
    constexpr void move(d3d11_surface& o)
    {
        _swap_chain = o._swap_chain;
        _rtv = o._rtv;
        _window = o._window;
        _allow_tearing = o._allow_tearing;
        _present_flags = o._present_flags;
        _viewport = o._viewport;
        _scissor_rect = o._scissor_rect;
        _light_culling_id = o._light_culling_id;

        o.reset();
    }

    constexpr void reset()
    {
        _swap_chain = nullptr;
        _rtv = {};
        _window = {};
        _allow_tearing = 0;
        _present_flags = 0;
        _viewport = {};
        _scissor_rect = {};
        _light_culling_id = id::invalid_id;
    }
#endif

    IDXGISwapChain4*		_swap_chain{ nullptr };
    ID3D11RenderTargetView*	_rtv{ nullptr };
    platform::window		_window{};
    u32						_allow_tearing{ 0 };
    u32						_present_flags{ 0 };
    D3D11_VIEWPORT			_viewport{};
    D3D11_RECT				_scissor_rect{};
    id::id_type				_light_culling_id{ id::invalid_id };
};
}

#endif
