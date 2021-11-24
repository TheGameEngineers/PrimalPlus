#pragma once
#include "CommonHeaders.h"

#ifdef _WIN64
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace primal::platform {

using window_proc = LRESULT(*)(HWND, UINT, WPARAM, LPARAM);
using window_handle = HWND;

struct window_init_info
{
    window_proc     callback{ nullptr };
    window_handle   parent{ nullptr };
    const wchar_t*  caption{ nullptr };
    s32             left{ 0 };
    s32             top{ 0 };
    s32             width{ 1920 };
    s32             height{ 1080 };
};
}
#endif // _WIN64

#ifdef __linux__
#include <X11/Xlib.h>
#include <stdlib.h>

namespace primal::platform {
	
using window_handle = Window*;

struct window_init_info
{
	void*			callback{ nullptr };
	window_handle	parent{ nullptr };
	const wchar_t*	caption{ nullptr };
	s32				left{ 0 };
	s32				top{ 0 };
	s32				width{ 1920 };
	s32				height{ 1080 };
};
}
#endif // __linux__