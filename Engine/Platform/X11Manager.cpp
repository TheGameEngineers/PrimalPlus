#include "X11Manager.h"

namespace primal::platform {
    
namespace {

Display*                                disp{ nullptr };
u32                                     screen;
Window                                  dummy_window;
Atom                                    wm_delete_window{ u32_invalid_id };
Atom                                    quit_msg { u32_invalid_id };
XEvent                                  xev{};
utl::free_list<lwin_proc>               win_procs;
utl::vector<u32>                        removed_procs;
utl::vector<u32>                        active_procs;
utl::vector<window_type>                window_ids;
std::unordered_map<window_type, u32>    window_map;

void
remove_inactive_listeners()
{
    for (u32 i{ 0 }; i < removed_procs.size(); ++i)
    {
        u32 index = removed_procs[i];
        win_procs.remove(index);
        active_procs[index] = u32_invalid_id;
    }
    removed_procs.clear();
}

} // anonymous namespace

bool
display()
{
    XInitThreads();
    
    disp = XOpenDisplay(nullptr);
    if (disp == nullptr) return false;

    screen = DefaultScreen(disp);
    dummy_window = XCreateSimpleWindow(disp, DefaultRootWindow(disp), 0, 0, 1, 1, 0, 0, 0); // Used to send quit messages, since windows may all be closed
    
    wm_delete_window = XInternAtom(get_display(), "WM_DELETE_WINDOW", false);
    quit_msg = XInternAtom(get_display(), "QUIT_MSG", false);

    return true;
}

s32
display_width()
{
    return (s32)DisplayWidth(disp, screen);
}

s32
display_height()
{
    return (s32)DisplayHeight(disp, screen);
}

void
close_display()
{
    XCloseDisplay(disp);
}

window_type
create_linux_window(window_handle parent, s32 left, s32 top, s32 width, s32 height, const char* const name)
{
    window_handle parent_window{ parent ? parent : &(DefaultRootWindow(disp)) };
    assert(parent_window != nullptr);

    // Setup the visual and colormap
    Visual* visual{ DefaultVisual(disp, screen) };
    Colormap colormap{ XCreateColormap(disp, DefaultRootWindow(disp), visual, AllocNone) };

    // Define attributes for the window
    XSetWindowAttributes attributes;
    attributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask |
                            ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    attributes.colormap = colormap;

    Window wnd{ XCreateWindow(disp, *parent_window, left, top, width, height, 0,
                              DefaultDepth(disp, screen), InputOutput, visual,
                              CWColormap | CWEventMask, &attributes) };

    // Set custom window manager event for closing a window
    XSetWMProtocols(get_display(), wnd, &wm_delete_window, 1);

    // Show window
    XStoreName(get_display(), wnd, name);
    XMapWindow(get_display(), wnd);

    return wnd;
}

void
close_window(window_handle wnd)
{
    XDestroyWindow(disp, *wnd);
}

void
clear_window(window_handle wnd)
{
    // NOTE: this is not currently working how I would like... I expected the window to redraw
    //		 itself with the XClearWindow() call. Eventually, the graphics API will be drawing
    //		 to the window anyway, so I will not pursue it further, unless it becomes an issue.
    XClearWindow(disp, *wnd);
}

void
name_window(window_handle wnd, const char* const name)
{
    XStoreName(disp, *wnd, name);
}

bool
pending_events()
{
    return XPending(disp) > 0;
}

void
next_event(event* ev)
{
    XNextEvent(disp, &xev);

    // Populate high level event
    ev->event_type = (event::type)xev.type;
    ev->window = xev.xany.window;
    ev->width = (u32)xev.xconfigure.width;
    ev->height = (u32)xev.xconfigure.height;
    ev->mod_key = (u32)xev.xkey.state;
    ev->keycode = (u32)xev.xkey.keycode;
    ev->key_sym = XLookupKeysym(&xev.xkey, 0);
    ev->button = (event::button_type)xev.xbutton.button;
    ev->mouse_x = (f32)xev.xmotion.x;
    ev->mouse_y = (f32)xev.xmotion.y;
    // NOTE: Button4 is scroll wheel up, Button5 is scroll wheel down. There can be Button6 and Button7 which are scroll wheel push left and right,
    //		 respectively, so to insulate aginst any extended buttons here, we will record a delta of 0. X11 doesn't have a GET_WHEEL_DELTA function
    //		 like Win32, so it lacks the ability to handle precision wheel scrolling. A wheel delta of 120.f was chosen to indicate one click scroll up,
    //		 and a delta of -120.f was chosen to indicate one click scroll down, because WHEEL_DELTA in Win32 are multiples of 120 for up, and -120 for down.
    //		 This allows the engine to treat the wheel delta in the same fashion between OSes.
    ev->wheel_delta = xev.xbutton.button == Button4 ? 120.f : xev.xbutton.button == Button5 ? -120.f : 0.f;
    ev->msg = (Atom)xev.xclient.data.l[0];
}

u32
add_listener(lwin_proc callback, window_type wnd)
{
    u32 index = win_procs.add((lwin_proc)callback);
    active_procs.emplace_back(index);
    window_ids.emplace_back(wnd);
    return index;
}

void
remove_listener(u32 id)
{
    removed_procs.emplace_back(id);
}

void
dispatch_event(const event* const ev)
{
    remove_inactive_listeners();
    
    for (u32 i{ 0 }; i < active_procs.size(); ++i)
    {
        u32 index = active_procs[i];
        // If the active procedure points to an invalid (removed) lwin_proc, skip
        if (index == u32_invalid_id) continue;
        assert(win_procs[index]);

        // Only dispatch the event to window the event triggered from
        if (ev->window == window_ids[i])
            win_procs[index](ev);
    }
}

void
send_fullscreen_event(window_handle wnd, bool is_fullscreen)
{
    XEvent xev;
    Atom wm_state{ XInternAtom(disp, "_NET_WM_STATE", false) };
    Atom fullscreen{ XInternAtom(disp, "_NET_WM_STATE_FULLSCREEN", false) };
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = *wnd;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = is_fullscreen ? 1 : 0; // _NET_WM_STATE_ADD
    xev.xclient.data.l[1] = fullscreen;
    xev.xclient.data.l[2] = 0;
    XSendEvent(disp, DefaultRootWindow(disp), false,
                SubstructureNotifyMask | SubstructureRedirectMask, &xev);
}

void
send_quit_event()
{
    XEvent close;
    close.xclient.type = ClientMessage;
    close.xclient.serial = dummy_window;
    close.xclient.send_event = true;
    close.xclient.message_type = XInternAtom(disp, "QUIT_MSG", false);
    close.xclient.format = 32;
    close.xclient.window = 0;
    close.xclient.data.l[0] = XInternAtom(disp, "QUIT_MSG", false);
    XSendEvent(disp, dummy_window, false, NoEventMask, &close);
}

bool
quit_msg_received(const event* const ev)
{
    return (ev->msg == quit_msg);
}

bool
window_close_received(const event* const ev)
{
    return (ev->msg == wm_delete_window);
}

Display*
get_display()
{
    assert(disp);
    return disp;
}

void
add_id(window_type wnd, u32 id)
{
    if (window_map.count(wnd)) return;
    window_map[wnd] = id;
}

u32
fetch_id(window_type wnd)
{
    return window_map.count(wnd) ? window_map[wnd] : u32_invalid_id;
}

}