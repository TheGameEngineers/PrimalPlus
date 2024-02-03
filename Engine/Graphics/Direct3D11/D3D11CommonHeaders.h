#pragma once
#include "CommonHeaders.h"
#include "Graphics/Renderer.h"

#ifndef PRIMAL_BUILD_D3D11
#if __has_include("d3d11_4.h") && __has_include("dxgi1_6.h")
#define PRIMAL_BUILD_D3D11 1
#else
#define PRIMAL_BUILD_D3D11 0
#endif
#endif

#if PRIMAL_BUILD_D3D11 && !(__has_include("d3d11_4") || __has_include("dxgi1_6.h"))
#pragma warning("Warning: DirectX 11 Build was explicitly specfied but couldn't be found!")
#endif

#if PRIMAL_BUILD_D3D11
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef PRIMAL_D3D11_DLL
#define PRIMAL_D3D11_DLL 1
#endif

#ifndef PRIMAL_D3D11_CONSTANT_BUFFER_ALIGNMENT
#define PRIMAL_D3D11_CONSTANT_BUFFER_ALIGNMENT 256
#endif

#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <wrl.h>

#if !PRIMAL_D3D11_DLL
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#endif

#pragma comment(lib, "dxguid.lib")

namespace primal::graphics::d3d11 {
constexpr u32 frame_buffer_count{ 3 };

struct d3d11_pipeline_state
{
	ID3D11VertexShader*			vs{ nullptr };
	ID3D11HullShader*			hs{ nullptr };
	ID3D11DomainShader*			ds{ nullptr };
	ID3D11GeometryShader*		gs{ nullptr };
	ID3D11PixelShader*			ps{ nullptr };

	//Maybe, just maybe, we might not even be needing this stuff
	ID3D11BlendState1*			blend_state{ nullptr };
	ID3D11RasterizerState2*		rasterizer_state{ nullptr };
	ID3D11SamplerState*			sampler_state{ nullptr };
};
}

#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)								\
if(FAILED(x)) {									\
	char line_number[32];						\
	sprintf_s(line_number, "%u", __LINE__);		\
	OutputDebugStringA("Error in: ");			\
	OutputDebugStringA(__FILE__);				\
	OutputDebugStringA("\nLine ");				\
	OutputDebugStringA(line_number);			\
	OutputDebugStringA("\n");					\
	OutputDebugStringA(#x);						\
	OutputDebugStringA("\n");					\
	__debugbreak();								\
}
#endif
#else
#ifndef DXCall
#define DXCall(x) x
#endif
#endif

#ifdef _DEBUG
#define NAME_D3D11_OBJECT(obj, name) obj->SetPrivateData(WKPDID_D3DDebugObjectNameW, (UINT)(wcslen(name) + 1), name); \
	OutputDebugString(L"::D3D11 Object Created: "); OutputDebugString(name); OutputDebugString(L"\n");
#define NAME_D3D11_OBJECT_INDEXED(obj, idx, name)												\
{																								\
	wchar_t full_name[128];																		\
	if(swprintf_s(full_name, L"%s[%llu]", name, (u64)idx) > 0) {								\
	obj->SetPrivateData(WKPDID_D3DDebugObjectNameW, (UINT)(wcslen(full_name) + 1), full_name);	\
	OutputDebugString(L"::D3D11 Object Created: ");												\
	OutputDebugString(full_name);																\
	OutputDebugString(L"\n");																	\
}}
#else
#define NAME_D3D11_OBJECT(obj, name)
#define NAME_D3D11_OBJECT_INDEXED(obj, idx, name)
#endif


#include "D3D11Resources.h"
#include "D3D11Helpers.h"

#endif
