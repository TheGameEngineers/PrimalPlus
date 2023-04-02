#ifdef __linux__

#include "Platform.h"
#include "PlatformTypes.h"
#include "Input/InputLinux.h"

namespace primal::platform {
	
namespace {
	
DEFINE_TYPED_ID(lwin_proc_id);

// Linux OS specific window info
struct window_info
{
	window_type 	wnd{};
	window_id		wnd_id { u32_invalid_id };
	lwin_proc_id	internal_id { u32_invalid_id };
	lwin_proc_id	extra_id { u32_invalid_id };
	s32 			left;
	s32 			top;
	s32 			width;
	s32 			height;
	bool 			is_fullscreen{ false };
	bool 			is_closed{ false };
};

utl::free_list<window_info> windows;

window_info&
get_from_id(window_id id)
{
	assert(windows[id].wnd);
	return windows[id];
}

window_info&
get_from_handle(window_type wnd)
{
	return get_from_id((window_id)fetch_id(wnd));
}

void
resize_window(window_id id, u32 width, u32 height)
{
	window_info& info{ get_from_id(id) };
	info.width = width;
	info.height = height;
	clear_window(&info.wnd);
}

void
set_window_fullscreen(window_id id, bool is_fullscreen)
{
	window_info& info{ get_from_id(id) };
	if (info.is_fullscreen != is_fullscreen)
	{
		info.is_fullscreen = is_fullscreen;

		send_fullscreen_event(&info.wnd, is_fullscreen);
	}
}

bool
is_window_fullscreen(window_id id)
{
	return get_from_id(id).is_fullscreen;
}

window_handle
get_window_handle(window_id id)
{
	return &get_from_id(id).wnd;
}

void internal_lwin_proc(const event* const ev)
{
	switch (ev->event_type)
	{
		case event::client_message:
		{
			get_from_handle(ev->window).is_closed = true;
		}
		break;
		case event::configure_notify:
		{	
			if (ev->width != (u32)get_from_handle(ev->window).width || ev->height != (u32)get_from_handle(ev->window).height)
			{
				resize_window((window_id)fetch_id(ev->window), ev->width, ev->height);
			}
		}
	}
	
	input::process_input_message(ev, get_display());
}

void
set_window_caption(window_id id, const wchar_t* caption)
{
	window_info& info{ get_from_id(id) };
	size_t outSize = (sizeof(caption) * sizeof(wchar_t)) + 1;
	char title[outSize];
	wcstombs(title, caption, outSize);
	name_window(&info.wnd, title);
}

math::u32v4
get_window_size(window_id id)
{
	window_info& info{ get_from_id(id) };
	return { (u32)info.left, (u32)info.top, (u32)info.width - (u32)info.left, (u32)info.height - (u32)info.top };
}

bool
is_window_closed(window_id id)
{
	return get_from_id(id).is_closed;
}

void
set_window_closed(window_id id)
{
	get_from_id(id).is_closed = true;
}

} // anonymous namespace

window
create_window(const window_init_info* const init_info /*= nullptr*/)
{
	// Create an instance of window_info
	window_info info{};
	info.left = (init_info && init_info->left) ? init_info->left : 0; // generally, Linux window managers override
	info.top = (init_info && init_info->top) ? init_info->top : 0;	   // the starting top left coords, so default is 0,0
	info.width = (init_info && init_info->width) ? init_info->width : display_width();
	info.height = (init_info && init_info->height) ? init_info->height : display_height();

	// check for initial info, use defaults if none given
	window_handle parent{ init_info ? init_info->parent : nullptr };
	const wchar_t* caption{ (init_info && init_info->caption) ? init_info->caption : L"Primal Game" };
	size_t out_size = (sizeof(caption) * sizeof(wchar_t)) + 1;
	char title[out_size];
	wcstombs(title, caption, out_size);

	info.wnd = create_linux_window(parent, info.left, info.top, info.width, info.height, title);

	// Add listener functions to event dispatcher
	info.internal_id = (lwin_proc_id)add_listener(internal_lwin_proc, info.wnd);
	if (init_info->callback)
		info.extra_id = (lwin_proc_id)add_listener((lwin_proc)init_info->callback, info.wnd);

	const window_id id{ windows.add(info) };
	get_from_id(id).wnd_id = id;
	add_id(info.wnd, id);
	return window{ id };
}

void
remove_window(window_id id)
{
	window_info& info{ get_from_id(id) };
	if (info.internal_id != u32_invalid_id) remove_listener((u32)info.internal_id);
	if (info.extra_id != u32_invalid_id) remove_listener((u32)info.extra_id);
	close_window(&info.wnd);
	windows.remove(id);
}

}

#include "IncludeWindowCpp.h"

#endif // __linux__