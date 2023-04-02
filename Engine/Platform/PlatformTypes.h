#pragma once
#include "CommonHeaders.h"

#ifdef _WIN64

#ifndef WIN_32_LEAN_AND_MEAN
#define WIN_32_LEAN_AND_MEAN
#endif // WIN_32_LEAN_AND_MEAN

#include <Windows.h>

namespace primal::platform {
	
using window_proc = LRESULT(*)(HWND, UINT, WPARAM, LPARAM);
using window_handle = HWND;

struct window_init_info
{
	window_proc		callback{ nullptr };
	window_handle	parent{ nullptr };
	const wchar_t*	caption{ nullptr };
	s32				left{ 0 };
	s32				top{ 0 };
	s32				width{ 1580 };
	s32				height{ 950 };
};

}
#endif // _WIN64

#ifdef __linux__
#include "LinuxWindowManager.h"

namespace primal::platform {	
struct window_init_info
{
	lwin_proc		callback{ nullptr };
	window_handle	parent{ nullptr };
	const wchar_t*	caption{ nullptr };
	s32				left{ 0 };
	s32				top{ 0 };
	s32				width{ 1580 };
	s32				height{ 950 };
};

}
#endif // __linux__