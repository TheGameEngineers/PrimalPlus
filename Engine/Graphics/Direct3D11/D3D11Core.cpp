#include "D3D11Core.h"
#include "D3D11Surface.h"
#include "D3D11Camera.h"
#include "D3D11Shaders.h"
#include "D3D11GPass.h"
#include "D3D11PostProcess.h"
#include "D3D11Content.h"
#include "D3D11Light.h"
#include "D3D11LightCulling.h"
#include "Shaders/SharedTypes.h"

#if PRIMAL_BUILD_D3D11

using namespace Microsoft::WRL;

namespace primal::graphics::d3d11::core {
namespace {
class d3d11_command
{
public:
	d3d11_command() = default;
	DISABLE_COPY_AND_MOVE(d3d11_command)
		explicit d3d11_command(ID3D11Device5* const device)
	{
		HRESULT hr{ S_OK };
		DXCall(hr = device->CreateFence(0, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
		if (FAILED(hr)) return;

		NAME_D3D11_OBJECT(_fence, L"D3D11 Fence");
		ID3D11DeviceContext* def_ctx{ nullptr };
		DXCall(device->CreateDeferredContext(0, &def_ctx));
		if (FAILED(hr)) goto _error;
		def_ctx->QueryInterface(IID_PPV_ARGS(&_context));
		core::release(def_ctx);
		NAME_D3D11_OBJECT(_context, L"GFX Deferred Context");

		_fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		assert(_fence_event);
		if (!_fence_event) goto _error;

		return;

	_error:
		release();
	}

	void begin_frame()
	{
		command_frame& frame{ _cmd_frames[_frame_index] };
		frame.wait(_fence_event, _fence);
		core::release(_cmd_list);
	}

	void end_frame(const d3d11_surface& surface, ID3D11DeviceContext4* imm_ctx)
	{
		_context->FinishCommandList(FALSE, &_cmd_list);
		assert(_cmd_list);
		imm_ctx->ExecuteCommandList(_cmd_list, FALSE);

		surface.present();

		u64& fence_value{ _fence_value };
		++fence_value;
		command_frame& frame{ _cmd_frames[_frame_index] };
		frame.fence_value = fence_value;
		imm_ctx->Signal(_fence, _fence_value);
		_frame_index = (_frame_index + 1) % frame_buffer_count;
	}

	void flush()
	{
		for (u32 i{ 0 }; i < frame_buffer_count; ++i)
		{
			_cmd_frames[i].wait(_fence_event, _fence);
		}

		_frame_index = 0;
		core::release(_cmd_list);
	}

	void release()
	{
		_context->ClearState();
		flush();
		core::release(_fence);
		_fence_value = 0;

		CloseHandle(_fence_event);
		_fence_event = nullptr;

		core::release(_context);
		core::release(_cmd_list);
	}

	_NODISCARD ID3D11DeviceContext4* const context() const { return _context; }
	_NODISCARD constexpr u32 frame_index() const { return _frame_index; }

private:
	struct command_frame
	{
		u64					fence_value{ 0 };

		void wait(HANDLE fence_event, ID3D11Fence* fence)
		{
			assert(fence && fence_event);

			if (fence->GetCompletedValue() < fence_value)
			{
				DXCall(fence->SetEventOnCompletion(fence_value, fence_event));
				WaitForSingleObject(fence_event, INFINITE);
			}
		}
	};

	ID3D11DeviceContext4*	_context{ nullptr };
	ID3D11CommandList*		_cmd_list{ nullptr };
	ID3D11Fence*			_fence{ nullptr };
	HANDLE					_fence_event{ nullptr };
	u64						_fence_value{ 0 };
	u32						_frame_index{ 0 };
	command_frame			_cmd_frames[frame_buffer_count]{};
};

using pfn_create_dxgi_factory_2 = HRESULT(*)(UINT, const IID&, void**);
using surface_collection = utl::free_list<d3d11_surface>;
constexpr D3D_FEATURE_LEVEL		minimum_feature_level{ D3D_FEATURE_LEVEL_10_0 };

HMODULE							d3d11_dll{ nullptr };
HMODULE							dxgi_dll{ nullptr };
#if PRIMAL_D3D11_DLL
PFN_D3D11_CREATE_DEVICE			D3D11CreateDevice{ nullptr };
#endif
IDXGIFactory7*					dxgi_factory{ nullptr };
ID3D11Device5*					main_device{ nullptr };
ID3D11DeviceContext4*			context{ nullptr };
constant_buffer					constant_buffers[frame_buffer_count];
d3d11_command					gfx_command;
surface_collection				surfaces;

utl::vector<IUnknown*>			deferred_releases[frame_buffer_count]{};
u32								deferred_releases_flag[frame_buffer_count]{};
std::mutex						deferred_releases_mutex{};

bool
failed_init()
{
	shutdown();
	return false;
}

IDXGIAdapter4*
determine_main_adapter()
{
	IDXGIAdapter4* adapter{ nullptr };
	D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	for (u32 i{ 0 };
		dxgi_factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		if (SUCCEEDED(D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0,
			0, feature_levels, _countof(feature_levels), D3D11_SDK_VERSION,
			nullptr, nullptr, nullptr)))
		{
			return adapter;
		}

		release(adapter);
	}

	return nullptr;
}

void __declspec(noinline)
process_deferred_releases(u32 frame_idx)
{
	std::lock_guard lock{ deferred_releases_mutex };

	deferred_releases_flag[frame_idx] = 0;

	utl::vector<IUnknown*>& resources{ deferred_releases[frame_idx] };
	if (!resources.empty())
	{
		for (auto& resource : resources)
		{
			release(resource);
		}
		resources.clear();
	}
}

d3d11_frame_info
get_d3d11_frame_info(const frame_info& info, const d3d11_surface& surface,
	u32 frame_idx, f32 delta_time)
{
	camera::d3d11_camera& camera{ camera::get(info.camera_id) };
	camera.update();

	hlsl::GlobalShaderData* datap{ cbuffer().allocate<hlsl::GlobalShaderData>() };
	hlsl::GlobalShaderData& data{ *datap };

	using namespace DirectX;
	XMStoreFloat4x4A(&data.View, camera.view());
	XMStoreFloat4x4A(&data.Projection, camera.projection());
	XMStoreFloat4x4A(&data.InvProjection, camera.inverse_projection());
	XMStoreFloat4x4A(&data.ViewProjection, camera.view_projection());
	XMStoreFloat4x4A(&data.InvViewProjection, camera.inverse_view_projection());
	XMStoreFloat3(&data.CameraPosition, camera.position());
	XMStoreFloat3(&data.CameraDirection, camera.direction());
	data.ViewWidth = surface.viewport().Width;
	data.ViewHeight = surface.viewport().Height;
	data.NumDirectionalLights = light::non_cullable_light_count(info.light_set_key);
	data.DeltaTime = delta_time;

	d3d11_frame_info d3d11_info
	{
		&info,
		&camera,
		cbuffer().offset(datap),
		surface.width(),
		surface.height(),
		surface.light_culling_id(),
		frame_idx,
		delta_time
	};

	return d3d11_info;
}
}//anonymous namespace
namespace detail {
void
deferred_release(IUnknown* resource)
{
	const u32 frame_idx{ current_frame_index() };
	std::lock_guard lock{ deferred_releases_mutex };
	deferred_releases[frame_idx].push_back(resource);
	set_deferred_releases_flag();
}
}//detail namespace

bool
initialize()
{
#if PRIMAL_D3D11_DLL
	d3d11_dll = LoadLibraryEx(L"d3d11.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
	dxgi_dll = LoadLibraryEx(L"dxgi.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);

	if (!(d3d11_dll && dxgi_dll))
		return failed_init();

	pfn_create_dxgi_factory_2 CreateDXGIFactory2 = (pfn_create_dxgi_factory_2)GetProcAddress(dxgi_dll, "CreateDXGIFactory2");
	if (!(CreateDXGIFactory2)) return failed_init();

	D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11_dll, "D3D11CreateDevice");
	if (!D3D11CreateDevice) failed_init();
#endif

	HRESULT hr{ S_OK };
	DXCall(hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_factory)));
	if (FAILED(hr)) return failed_init();

	ID3D11Device* temp_device{ nullptr };
	ID3D11DeviceContext* temp_ctx{ nullptr };
	D3D_FEATURE_LEVEL max_feature_level{ minimum_feature_level };
	D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	u32 device_flags{ 0 };
#if _DEBUG
	device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ComPtr<IDXGIAdapter4> main_adapter;
	main_adapter.Attach(determine_main_adapter());
	if (!main_adapter) return failed_init();

	hr = D3D11CreateDevice(main_adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, 0,
		device_flags, feature_levels, _countof(feature_levels), D3D11_SDK_VERSION,
		&temp_device, &max_feature_level, &temp_ctx);
	if (FAILED(hr)) return failed_init();

	assert(max_feature_level >= minimum_feature_level);
	if (max_feature_level < minimum_feature_level) return failed_init();

	temp_device->QueryInterface(IID_PPV_ARGS(&main_device));
	temp_ctx->QueryInterface(IID_PPV_ARGS(&context));
	release(temp_device);
	release(temp_ctx);

	NAME_D3D11_OBJECT(main_device, L"Main D3D11 Device");
	NAME_D3D11_OBJECT(context, L"Main D3D11 Context");

	new(&gfx_command) d3d11_command(main_device);

#if _DEBUG
	{
		ComPtr<ID3D11InfoQueue> info_queue;
		DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));

		info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
		info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
		info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
	}
#endif

	if (!(shaders::initialize() &&
		gpass::initialize() &&
		fx::initialize() &&
		lightculling::initialize() &&
		content::initialize()))
		return failed_init();

	for (u32 i{ 0 }; i < frame_buffer_count; ++i)
	{
		new (&constant_buffers[i]) constant_buffer{ constant_buffer::get_default_init_info(1024 * 1024), gfx_command.context() };
		NAME_D3D11_OBJECT_INDEXED(constant_buffers[i].buffer(), i, L"Global Constant Buffer");
	}

	return true;
}

void
shutdown()
{
	gfx_command.release();

	for (u32 i{ 0 }; i < frame_buffer_count; ++i)
	{
		process_deferred_releases(i);
	}

	content::shutdown();
	lightculling::shutdown();
	fx::shutdown();
	gpass::shutdown();
	shaders::shutdown();

	for (u32 i{ 0 }; i < frame_buffer_count; ++i)
	{
		constant_buffers[i].release();
	}

	release(dxgi_factory);
	release(context);

	process_deferred_releases(0);

#if _DEBUG
	{
		ComPtr<ID3D11InfoQueue> info_queue;
		DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));

		info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, false);
		info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, false);
		info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, false);

		ComPtr<ID3D11Debug> debug{ nullptr };
		DXCall(main_device->QueryInterface(IID_PPV_ARGS(&debug)));
		debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
	}
#endif

	release(main_device);

#if PRIMAL_D3D11_DLL
	FreeLibrary(d3d11_dll);
	FreeLibrary(dxgi_dll);
#endif
	}

ID3D11Device5* const device()
{
	return main_device;
}

constant_buffer&
cbuffer()
{
	return constant_buffers[current_frame_index()];
}

u32
current_frame_index()
{
	return gfx_command.frame_index();
}

void
set_deferred_releases_flag()
{
	deferred_releases_flag[current_frame_index()] = 1;
}

surface
create_surface(platform::window window)
{
	surface_id id{ surfaces.add(window) };
	surfaces[id].create_swap_chain(dxgi_factory);
	return surface{ id };
}

void
remove_surface(surface_id id)
{
	gfx_command.flush();
	surfaces.remove(id);
}

void
resize_surface(surface_id id, u32, u32)
{
	gfx_command.flush();
	surfaces[id].resize();
}

u32
surface_width(surface_id id)
{
	return surfaces[id].width();
}

u32
surface_height(surface_id id)
{
	return surfaces[id].height();
}

void
render_surface(surface_id id, frame_info info)
{
	gfx_command.begin_frame();

	ID3D11DeviceContext4* ctx{ gfx_command.context() };

	const u32 frame_idx{ current_frame_index() };

	constant_buffer& cbuffer{ constant_buffers[frame_idx] };
	cbuffer.clear();

	if (deferred_releases_flag[frame_idx])
	{
		process_deferred_releases(frame_idx);
	}

	d3d11_surface& surface{ surfaces[id] };
	d3d11_frame_info d3d11_info{ get_d3d11_frame_info(info, surface, frame_idx, 16.7f) };//TODO dt?!?!
	gpass::set_size({ d3d11_info.surface_width, d3d11_info.surface_height });

	ctx->RSSetViewports(1, &surface.viewport());
	ctx->RSSetScissorRects(1, &surface.scissor_rect());

	//Depth Pre-Pass
	gpass::set_render_targets_for_depth_prepass(ctx);
	gpass::depth_prepass(ctx, d3d11_info);

	//Lighting Pass
	light::update_light_buffers(d3d11_info, ctx);
	lightculling::cull_lights(ctx, d3d11_info);

	//Render Pass
	gpass::set_render_targets_for_gpass(ctx);
	gpass::render(ctx, d3d11_info);

	//Post-process Pass
	fx::post_process(ctx, d3d11_info, surface.rtv());

	gfx_command.end_frame(surface, context);
}
}

#endif
