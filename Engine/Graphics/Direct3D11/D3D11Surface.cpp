#include "D3D11Surface.h"
#include "D3D11Core.h"
#include "D3D11LightCulling.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11 {
namespace {
constexpr DXGI_FORMAT
to_non_srgb(DXGI_FORMAT format)
{
	if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) return DXGI_FORMAT_R8G8B8A8_UNORM;
	return format;
}

__declspec(noinline) void
handle_dxgi_error()
{
	HRESULT hr = core::device()->GetDeviceRemovedReason();
	hr = DXGI_ERROR_DEVICE_HUNG;
	char error_msg[128]{};
	sprintf_s(error_msg, 100, "D3D11 Device was removed! DXGI error code: 0x%X\n", hr);
	MessageBoxA(0, error_msg, "Error!", MB_ICONERROR);
	__debugbreak();
}
}//anonymous namespace

bool
d3d11_surface::create_swap_chain(IDXGIFactory7* factory)
{
	auto* const device{ core::device() };

	release();

	if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_allow_tearing, sizeof(u32))) && _allow_tearing)
	{
		_present_flags = DXGI_PRESENT_ALLOW_TEARING;
	}

	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.BufferCount = buffer_count;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Flags = _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	desc.Format = to_non_srgb(default_back_buffer_format);
	desc.Width = _window.width();
	desc.Height = _window.height();
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Stereo = false;

	IDXGISwapChain1* swap_chain{ nullptr };
	HWND hwnd{ (HWND)_window.handle() };
	DXCall(factory->CreateSwapChainForHwnd(device, hwnd, &desc, nullptr, nullptr, &swap_chain));
	DXCall(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
	DXCall(swap_chain->QueryInterface(IID_PPV_ARGS(&_swap_chain)));

	core::release(swap_chain);

	finalize();
	assert(!id::is_valid(_light_culling_id));
	_light_culling_id = lightculling::add_culler();

	return _swap_chain;
}

void
d3d11_surface::present() const
{
	assert(_swap_chain);
	HRESULT hr{ S_OK };
	DXCall(hr = _swap_chain->Present(0, _present_flags));
	if (hr != S_OK) handle_dxgi_error();//DXGI_STATUS_OCCLUDED won't be returned since we have a flip model swapchain!
}

void
d3d11_surface::resize()
{
	assert(_swap_chain);
	core::release(_rtv);
	const u32 flags{ _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0ul };
	HRESULT hr{ S_OK };
	DXCall(hr = _swap_chain->ResizeBuffers(buffer_count, 0, 0, DXGI_FORMAT_UNKNOWN, flags));
	if (hr != S_OK) handle_dxgi_error();

	finalize();

	DEBUG_OP(OutputDebugString(L"::D3D11 Surface Resized.\n"));
}

void
d3d11_surface::finalize()
{
	ID3D11Resource* buffer{ nullptr };
	DXCall(_swap_chain->GetBuffer(0, IID_PPV_ARGS(&buffer)));
	assert(buffer);

	D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};
	rtv_desc.Format = default_back_buffer_format;
	rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	DXCall(core::device()->CreateRenderTargetView(buffer, &rtv_desc, &_rtv));
	assert(_rtv);
	core::release(buffer);

	DXGI_SWAP_CHAIN_DESC desc{};
	DXCall(_swap_chain->GetDesc(&desc));
	const u32 width{ desc.BufferDesc.Width };
	const u32 height{ desc.BufferDesc.Height };
	assert(_window.width() == width && _window.height() == height);

	_viewport.TopLeftX = 0.f;
	_viewport.TopLeftY = 0.f;
	_viewport.Width = (f32)width;
	_viewport.Height = (f32)height;
	_viewport.MinDepth = 0.f;
	_viewport.MaxDepth = 1.f;

	_scissor_rect = { 0, 0, (s32)width, (s32)height };
}

void
d3d11_surface::release()
{
	if (id::is_valid(_light_culling_id)) lightculling::remove_culler(_light_culling_id);

	core::release(_rtv);
	core::release(_swap_chain);
}
}

#endif
