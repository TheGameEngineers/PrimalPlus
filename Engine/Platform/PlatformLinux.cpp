#ifdef __linux__
#include "Platform.h"
#include "PlatformTypes.h"

namespace primal::platform {

namespace {
// Linux OS specific window info
struct window_info
{
    Window wnd{};
    Display *display{ nullptr };
    s32 left;
    s32 top;
    s32 width;
    s32 height;
    bool is_fullscreen{ false };
    bool is_closed{ false };
};

utl::free_list<window_info> windows;

window_info&
get_from_id(window_id id)
{
    assert(windows[id].wnd);
    return windows[id];
}

// Linux specific window class functions
void
resize_window(window_id id, u32 width, u32 height)
{
    window_info& info{ get_from_id(id) };
    info.width = width;
    info.height = height;
    // NOTE: this is not currently working how I would like... I expected the window to redraw
    //		 itself with the XClearWindow() call. Eventually, the graphics API will be drawing
    //		 to the window anyway, so I will not pursue it further, unless it becomes an issue.
    XClearWindow(info.display, info.wnd);
}

void
set_window_fullscreen(window_id id, bool is_fullscreen)
{
    window_info &info{ get_from_id(id) };
    if (info.is_fullscreen != is_fullscreen)
    {
        info.is_fullscreen = is_fullscreen;

        if (is_fullscreen)
        {
            XEvent xev;
            Atom wm_state{ XInternAtom(info.display, "_NET_WM_STATE", false) };
            Atom fullscreen{ XInternAtom(info.display, "_NET_WM_STATE_FULLSCREEN", false) };
            memset(&xev, 0, sizeof(xev));
            xev.type = ClientMessage;
            xev.xclient.window = info.wnd;
            xev.xclient.message_type = wm_state;
            xev.xclient.format = 32;
            xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
            xev.xclient.data.l[1] = fullscreen;
            xev.xclient.data.l[2] = 0;
            XSendEvent(info.display, DefaultRootWindow(info.display), false,
                SubstructureNotifyMask | SubstructureRedirectMask, &xev);
        }
        else
        {
            XEvent xev;
            Atom wm_state{ XInternAtom(info.display, "_NET_WM_STATE", false) };
            Atom fullscreen{ XInternAtom(info.display, "_NET_WM_STATE_FULLSCREEN", false) };
            memset(&xev, 0, sizeof(xev));
            xev.type = ClientMessage;
            xev.xclient.window = info.wnd;
            xev.xclient.message_type = wm_state;
            xev.xclient.format = 32;
            xev.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
            xev.xclient.data.l[1] = fullscreen;
            xev.xclient.data.l[2] = 0;
            XSendEvent(info.display, DefaultRootWindow(info.display), false,
                SubstructureNotifyMask | SubstructureRedirectMask, &xev);
        }
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

Display*
get_display(window_id id)
{
    return get_from_id(id).display;
}

void
set_window_caption(window_id id, const wchar_t* caption)
{
    window_info& info{ get_from_id(id) };
    size_t out_size = (sizeof(caption) * sizeof(wchar_t)) + 1;
    char title[out_size];
    wcstombs(title, caption, out_size);
    XStoreName(info.display, info.wnd, title);
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
    window_info& info{ get_from_id(id) };
    get_from_id(id).is_closed = true;
    XDestroyWindow(info.display, info.wnd);
}
} // anonymous namespace

Window
create_window(const window_init_info* const init_info /*= nullptr*/, void* disp /*= nullptr*/)
{
    // Cache a casted pointer of the display to save on casting later
    Display* display{ (Display *)disp };

    window_handle parent{ init_info ? init_info->parent : &(DefaultRootWindow(display)) };
    if (parent == nullptr)
    {
        parent = &(DefaultRootWindow(display));
    }
    assert(parent != nullptr);

    // Setup the screen, visual, and colormap
    int screen{ DefaultScreen(display) };
    Visual* visual{ DefaultVisual(display, screen) };
    Colormap colormap{ XCreateColormap(display, DefaultRootWindow(display),
                                        visual, AllocNone) };

    // Define attributes for the window
    XSetWindowAttributes attributes;
    attributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask |
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    attributes.colormap = colormap;

    // Create an instance of window_info
    window_info info{};
    info.left = (init_info && init_info->left) ? init_info->left : 0; // generally, the X window manager overrides
    info.top = (init_info && init_info->top) ? init_info->top : 0;	   // the starting top left coords, so default is 0,0
    info.width = (init_info && init_info->width) ? init_info->width : DisplayWidth(display, DefaultScreen(display));
    info.height = (init_info && init_info->height) ? init_info->height : DisplayHeight(display, DefaultScreen(display));
    info.display = display;

    // check for initial info, use defaults if none given
    const wchar_t* caption{ (init_info && init_info->caption) ? init_info->caption : L"Havana Game" };
    size_t out_size = (sizeof(caption) * sizeof(wchar_t)) + 1;
    char title[out_size];
    wcstombs(title, caption, out_size);

    Window wnd{ XCreateWindow(display, *parent, info.left, info.top, info.width, info.height, 0,
                                    DefaultDepth(display, screen), InputOutput, visual,
                                    CWColormap | CWEventMask, &attributes) };
    info.wnd = wnd;

    // Set custom window manager event for closing a window
    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(display, wnd, &wm_delete_window, 1);

    // Show window
    XMapWindow(display, wnd);
    XStoreName(display, wnd, title);

    const window_id id{ windows.add(info) };
    return window{ id };
}

void
remove_window(window_id id)
{
    window_info &info{ get_from_id(id) };
    get_from_id(id).is_closed = true;
    XDestroyWindow(info.display, info.wnd);
    windows.remove(id);
}
}

#include "IncludeWindowCpp.h"

#endif // __linux__