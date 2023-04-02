#pragma once

#ifdef __linux__

#include "Test.h"
#include "Platforms/PlatformTypes.h"
#include "Platforms/Platform.h"

using namespace primal;

platform::window windows[4];

void
win_proc(const platform::event* const ev)
{
	switch (ev->event_type)
	{
	case platform::event::configure_notify:
	{
		// Check if any of the windows resized
		for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
		{
			if (!_surfaces[i].window.is_valid()) continue;
			if (*((platform::window_handle)_surfaces[i].window.handle()) == ev->window)
			{
				if (ev->width != _surfaces[i].window.width() || ev->height != _surfaces[i].window.height())
				{
					_surfaces[i].window.resize(ev->width, ev->height);
				}
			}
		}
	}
	break;
	case platform::event::client_message:
	{
		if (platform::window_close_received(ev))
		{
			// Find which window was sent the close event, and call function
			for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
			{
				if (!_surfaces[i].window.is_valid()) continue;
				if (*((platform::window_handle)_surfaces[i].window.handle()) == ev->window)
				{
					destroy_render_surface(_surfaces[i]);
					break;
				}
			}

			// Check if all windows are closed, and exit application if so
			bool all_closed{ true };
			for (u32 i{ 0 }; i < _countof(_surfaces); i++)
			{
				if (!_surfaces[i].window.is_valid()) continue;
				if (!_surfaces[i].window.is_closed())
				{
					all_closed = false;
				}
			}
			if (all_closed)
			{
				platform::send_quit_event();
			}
		}
	}
	break;
	case platform::event::key_press:
	{
		// NOTE: This represents "alt + enter"
		if (ev->mod_key == 0x18 && ev->keycode == 36)
		{
			for (u32 i{ 0 }; i < _countof(_surfaces); i++)
			{
				if (!_surfaces[i].window.is_valid()) continue;
				if (*((platform::window_handle)_surfaces[i].window.handle()) == ev->window)
				{
					_surfaces[i].window.set_fullscreen(!_surfaces[i].window.is_fullscreen());
				}
			}
		}
	}
	}
}

class engine_test : public test
{
public:
	bool initialize() override
	{
		platform::window_init_info info[]
		{
			{ win_proc, nullptr, L"Test Window 1", 100, 100, 400, 800 },
			{ win_proc, nullptr, L"Test Window 2", 150, 150, 800, 400 },
			{ win_proc, nullptr, L"Test Window 3", 200, 200, 400, 400 },
			{ win_proc, nullptr, L"Test Window 4", 250, 250, 800, 600 },
		};

		static_assert(_countof(info) == _countof(windows));

		for (u32 i{ 0 }; i < _countof(windows); i++)
		{
			windows[i] = platform::create_window(&info[i], platform::get_display());
		}

		return true;
	}

	void run() override
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	void shutdown() override
	{
		for (u32 i{ 0 }; i < _countof(windows); ++i)
		{
			platform::remove_window(windows[i].get_id());
		}
	}
};

#endif // __linux__