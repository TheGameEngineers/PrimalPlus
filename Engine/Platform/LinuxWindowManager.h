#pragma once
#include "CommonHeaders.h"

#ifdef PLATFORM_WAYLAND
// TODO: Implement Wayland support
#error Wayland not implemented
#else
#include "X11Manager.h"
#endif

#include <stdlib.h>

namespace primal::platform {   
    
bool display();
s32 display_width();
s32 display_height();
void close_display();
window_type create_linux_window(window_handle parent, s32 left, s32 top, s32 width, s32 height, const char* const name);
void close_window(window_handle wnd);
void clear_window(window_handle wnd);
void name_window(window_handle wnd, const char* const name);
bool pending_events();
void next_event(event* ev);
u32 add_listener(lwin_proc callback);
void remove_listener(u32 id);
void dispatch_event(const event* const ev);
void send_fullscreen_event(window_handle wnd, bool is_fullscreen);
void send_quit_event();
bool quit_msg_received(const event* const ev);
bool window_close_received(const event* const ev);

}