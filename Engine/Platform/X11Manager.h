#pragma once
#include "CommonHeaders.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

namespace primal::platform {       
    
using window_type = Window;
using window_handle = Window*;

enum key_code
{
    key_backspace = XK_BackSpace,
    key_tab = XK_Tab,
    key_return = XK_Return,
    key_shift_l = XK_Shift_L,
    key_shift_r = XK_Shift_R,
    key_control_l = XK_Control_L,
    key_control_r = XK_Control_R,
    key_alt_l = XK_Alt_L,
    key_alt_r = XK_Alt_R,
    key_pause = XK_Pause,
    key_capslock = XK_Caps_Lock,
    key_escape = XK_Escape,
    key_space = XK_space,
    key_page_up = XK_Page_Up,
    key_page_down = XK_Page_Down,
    key_home = XK_Home,
    key_end = XK_End,
    key_left = XK_Left,
    key_up = XK_Up,
    key_right = XK_Right,
    key_down = XK_Down,
    key_print_screen = XK_Print,
    key_insert = XK_Insert,
    key_delete = XK_Delete,
    key_0 = XK_0,
    key_1 = XK_1,
    key_2 = XK_2,
    key_3 = XK_3,
    key_4 = XK_4,
    key_5 = XK_5,
    key_6 = XK_6,
    key_7 = XK_7,
    key_8 = XK_8,
    key_9 = XK_9,
    key_a = XK_a,
    key_b = XK_b,
    key_c = XK_c,
    key_d = XK_d,
    key_e = XK_e,
    key_f = XK_f,
    key_g = XK_g,
    key_h = XK_h,
    key_i = XK_i,
    key_j = XK_j,
    key_k = XK_k,
    key_l = XK_l,
    key_m = XK_m,
    key_n = XK_n,
    key_o = XK_o,
    key_p = XK_p,
    key_q = XK_q,
    key_r = XK_r,
    key_s = XK_s,
    key_t = XK_t,
    key_u = XK_u,
    key_v = XK_v,
    key_w = XK_w,
    key_x = XK_x,
    key_y = XK_y,
    key_z = XK_z,
    key_A = XK_A,
    key_B = XK_B,
    key_C = XK_C,
    key_D = XK_D,
    key_E = XK_E,
    key_F = XK_F,
    key_G = XK_G,
    key_H = XK_H,
    key_I = XK_I,
    key_J = XK_J,
    key_K = XK_K,
    key_L = XK_L,
    key_M = XK_M,
    key_N = XK_N,
    key_O = XK_O,
    key_P = XK_P,
    key_Q = XK_Q,
    key_R = XK_R,
    key_S = XK_S,
    key_T = XK_T,
    key_U = XK_U,
    key_V = XK_V,
    key_W = XK_W,
    key_X = XK_X,
    key_Y = XK_Y,
    key_Z = XK_Z,
    key_numpad_0 = XK_KP_0,
    key_numpad_1 = XK_KP_1,
    key_numpad_2 = XK_KP_2,
    key_numpad_3 = XK_KP_3,
    key_numpad_4 = XK_KP_4,
    key_numpad_5 = XK_KP_5,
    key_numpad_6 = XK_KP_6,
    key_numpad_7 = XK_KP_7,
    key_numpad_8 = XK_KP_8,
    key_numpad_9 = XK_KP_9,
    key_multiply = XK_multiply,
    key_add = XK_KP_Add,
    key_subtract = XK_KP_Subtract,
    key_decimal = XK_KP_Decimal,
    key_divide = XK_KP_Divide,
    key_f1 = XK_F1,
    key_f2 = XK_F2,
    key_f3 = XK_F3,
    key_f4 = XK_F4,
    key_f5 = XK_F5,
    key_f6 = XK_F6,
    key_f7 = XK_F7,
    key_f8 = XK_F8,
    key_f9 = XK_F9,
    key_f10 = XK_F10,
    key_f11 = XK_F11,
    key_f12 = XK_F12,
    key_numlock = XK_Num_Lock,
    key_scrolllock = XK_Scroll_Lock,
};

struct event
{
    enum type : u32
    {
        key_press           = 2U,
        key_release         = 3U,
        button_press        = 4U,
        button_release      = 5U,
        motion_notify       = 6U,
        configure_notify    = 22U,
        client_message      = 33U,
    };

    enum button_type : u32
    {
        button_1 = 1U,
        button_2,
        button_3,
        button_4,
        button_5,
    };
    
    type            event_type;
    window_type     window;
    u32             width;
    u32             height;
    u32             mod_key;
    u32             keycode;
    KeySym          key_sym;
    button_type     button;
    f32             mouse_x;
    f32             mouse_y;
    f32             wheel_delta;
    Atom            msg;
};

using lwin_proc = void(*)(const event* const);

Display* get_display();

}