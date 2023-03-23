// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#ifdef __linux__

#include "InputLinux.h"
#include "Input.h"

namespace primal::input {
namespace {
	
std::unordered_map<u32, u32> xk_mapping;

bool fill_keys()
{
	xk_mapping[XK_BackSpace] = input_code::code::key_backspace;
	xk_mapping[XK_Tab] = input_code::code::key_tab;
	xk_mapping[XK_Return] = input_code::code::key_return;
	xk_mapping[XK_Shift_L] = xk_mapping[XK_Shift_R] = input_code::code::key_shift;
	xk_mapping[XK_Control_L] = xk_mapping[XK_Control_R] = input_code::code::key_control;
	xk_mapping[XK_Alt_L] = xk_mapping[XK_Alt_R] = input_code::code::key_alt;
	xk_mapping[XK_Pause] = input_code::code::key_pause;
	xk_mapping[XK_Caps_Lock] = input_code::code::key_capslock;
	xk_mapping[XK_Escape] = input_code::code::key_escape;
	xk_mapping[XK_space] = input_code::code::key_space;
	xk_mapping[XK_Page_Up] = input_code::code::key_page_up;
	xk_mapping[XK_Page_Down] = input_code::code::key_page_down;
	xk_mapping[XK_Home] = input_code::code::key_home;
	xk_mapping[XK_End] = input_code::code::key_end;
	xk_mapping[XK_Left] = input_code::code::key_left;
	xk_mapping[XK_Up] = input_code::code::key_up;
	xk_mapping[XK_Right] = input_code::code::key_right;
	xk_mapping[XK_Down] = input_code::code::key_down;
	xk_mapping[XK_Print] = input_code::code::key_print_screen;
	xk_mapping[XK_Insert] = input_code::code::key_insert;
	xk_mapping[XK_Delete] = input_code::code::key_delete;

	xk_mapping[XK_0] = input_code::code::key_0;
	xk_mapping[XK_1] = input_code::code::key_1;
	xk_mapping[XK_2] = input_code::code::key_2;
	xk_mapping[XK_3] = input_code::code::key_3;
	xk_mapping[XK_4] = input_code::code::key_4;
	xk_mapping[XK_5] = input_code::code::key_5;
	xk_mapping[XK_6] = input_code::code::key_6;
	xk_mapping[XK_7] = input_code::code::key_7;
	xk_mapping[XK_8] = input_code::code::key_8;
	xk_mapping[XK_9] = input_code::code::key_9;

	xk_mapping[XK_a] = xk_mapping[XK_A] = input_code::code::key_a;
	xk_mapping[XK_b] = xk_mapping[XK_B] = input_code::code::key_b;
	xk_mapping[XK_c] = xk_mapping[XK_C] = input_code::code::key_c;
	xk_mapping[XK_d] = xk_mapping[XK_D] = input_code::code::key_d;
	xk_mapping[XK_e] = xk_mapping[XK_E] = input_code::code::key_e;
	xk_mapping[XK_f] = xk_mapping[XK_F] = input_code::code::key_f;
	xk_mapping[XK_g] = xk_mapping[XK_G] = input_code::code::key_g;
	xk_mapping[XK_h] = xk_mapping[XK_H] = input_code::code::key_h;
	xk_mapping[XK_i] = xk_mapping[XK_I] = input_code::code::key_i;
	xk_mapping[XK_j] = xk_mapping[XK_J] = input_code::code::key_j;
	xk_mapping[XK_k] = xk_mapping[XK_K] = input_code::code::key_k;
	xk_mapping[XK_l] = xk_mapping[XK_L] = input_code::code::key_l;
	xk_mapping[XK_m] = xk_mapping[XK_M] = input_code::code::key_m;
	xk_mapping[XK_n] = xk_mapping[XK_N] = input_code::code::key_n;
	xk_mapping[XK_o] = xk_mapping[XK_O] = input_code::code::key_o;
	xk_mapping[XK_p] = xk_mapping[XK_P] = input_code::code::key_p;
	xk_mapping[XK_q] = xk_mapping[XK_Q] = input_code::code::key_q;
	xk_mapping[XK_r] = xk_mapping[XK_R] = input_code::code::key_r;
	xk_mapping[XK_s] = xk_mapping[XK_S] = input_code::code::key_s;
	xk_mapping[XK_t] = xk_mapping[XK_T] = input_code::code::key_t;
	xk_mapping[XK_u] = xk_mapping[XK_U] = input_code::code::key_u;
	xk_mapping[XK_v] = xk_mapping[XK_V] = input_code::code::key_v;
	xk_mapping[XK_w] = xk_mapping[XK_W] = input_code::code::key_w;
	xk_mapping[XK_x] = xk_mapping[XK_X] = input_code::code::key_x;
	xk_mapping[XK_y] = xk_mapping[XK_Y] = input_code::code::key_y;
	xk_mapping[XK_z] = xk_mapping[XK_Z] = input_code::code::key_z;

	xk_mapping[XK_KP_0] = input_code::code::key_numpad_0;
	xk_mapping[XK_KP_1] = input_code::code::key_numpad_1;
	xk_mapping[XK_KP_2] = input_code::code::key_numpad_2;
	xk_mapping[XK_KP_3] = input_code::code::key_numpad_3;
	xk_mapping[XK_KP_4] = input_code::code::key_numpad_4;
	xk_mapping[XK_KP_5] = input_code::code::key_numpad_5;
	xk_mapping[XK_KP_6] = input_code::code::key_numpad_6;
	xk_mapping[XK_KP_7] = input_code::code::key_numpad_7;
	xk_mapping[XK_KP_8] = input_code::code::key_numpad_8;
	xk_mapping[XK_KP_9] = input_code::code::key_numpad_9;

	xk_mapping[XK_multiply] = input_code::code::key_multiply;
	xk_mapping[XK_KP_Add] = input_code::code::key_add;
	xk_mapping[XK_KP_Subtract] = input_code::code::key_subtract;
	xk_mapping[XK_KP_Decimal] = input_code::code::key_decimal;
	xk_mapping[XK_KP_Divide] = input_code::code::key_divide;

	xk_mapping[XK_F1] = input_code::code::key_f1;
	xk_mapping[XK_F2] = input_code::code::key_f2;
	xk_mapping[XK_F3] = input_code::code::key_f3;
	xk_mapping[XK_F4] = input_code::code::key_f4;
	xk_mapping[XK_F5] = input_code::code::key_f5;
	xk_mapping[XK_F6] = input_code::code::key_f6;
	xk_mapping[XK_F7] = input_code::code::key_f7;
	xk_mapping[XK_F8] = input_code::code::key_f8;
	xk_mapping[XK_F9] = input_code::code::key_f9;
	xk_mapping[XK_F10] = input_code::code::key_f10;
	xk_mapping[XK_F11] = input_code::code::key_f11;
	xk_mapping[XK_F12] = input_code::code::key_f12;

	xk_mapping[XK_Num_Lock] = input_code::code::key_numlock;
	xk_mapping[XK_Scroll_Lock] = input_code::code::key_scrollock;

	return true;
}

struct modifier_flags
{
	enum flags : u8
	{
		left_shift = 0x10,
		left_control = 0x20,
		left_alt = 0x40,

		right_shift = 0x01,
		right_control = 0x02,
		right_alt = 0x03,
	};
};

// Initialize xk_mapping key map once when everything else is loading
bool keymap_ready{ fill_keys() };
u8 modifier_keys_state{ 0 };

void
set_modifier_input(u32 keypress_type, KeySym key_sym, KeySym check_key, input_code::code code, modifier_flags::flags flags)
{
	if (keypress_type == KeyPress && key_sym == check_key)
	{
		set(input_source::keyboard, code, { 1.f, 0.f, 0.f });
		modifier_keys_state |= flags;
	}
	else if (modifier_keys_state & flags)
	{
		set(input_source::keyboard, code, { 0.f, 0.f, 0.f });
		modifier_keys_state &= ~flags;
	}
}

void
set_modifier_inputs(input_code::code code, KeySym key_sym, u32 keypress_type)
{
	if (code == input_code::key_shift)
	{
		set_modifier_input(keypress_type, key_sym, XK_Shift_L, input_code::key_left_shift, modifier_flags::left_shift);
		set_modifier_input(keypress_type, key_sym, XK_Shift_R, input_code::key_right_shift, modifier_flags::right_shift);
	}
	else if (code == input_code::key_control)
	{
		set_modifier_input(keypress_type, key_sym, XK_Control_L, input_code::key_left_control, modifier_flags::left_control);
		set_modifier_input(keypress_type, key_sym, XK_Control_R, input_code::key_right_control, modifier_flags::right_control);
	}
	else if (code == input_code::key_alt)
	{
		set_modifier_input(keypress_type, key_sym, XK_Alt_L, input_code::key_left_alt, modifier_flags::left_alt);
		set_modifier_input(keypress_type, key_sym, XK_Alt_R, input_code::key_right_alt, modifier_flags::right_alt);
	}
}
} // anonymous namespace

u32
get_key(u32 xk_keysym)
{
	assert(keymap_ready);

	if (xk_mapping.find(xk_keysym) == xk_mapping.end())
		return u32_invalid_id;
	else
		return xk_mapping[xk_keysym];
}

void
process_input_message(XEvent xev, Display* display)
{
	switch (xev.type)
	{
		// KeyPress and KeyRelease are the same event.
		case KeyPress:
		case KeyRelease:
		{
			const bool is_press { xev.xkey.type == KeyPress };
			const KeySym key_sym = XKeycodeToKeysym(display, xev.xkey.keycode, 0);
			const input_code::code code{ get_key(key_sym) };
			if (code != u32_invalid_id)
			{
				set(input_source::keyboard, code, { is_press ? 1.f : 0.f, 0.f, 0.f });
				set_modifier_inputs(code, key_sym, is_press ? KeyPress : KeyRelease);
			}
			
		}
		break;
		case MotionNotify:
		{
			set(input_source::mouse, input_code::mouse_position_x, { xev.xmotion.x, 0.f, 0.f });
			set(input_source::mouse, input_code::mouse_position_y, { xev.xmotion.y, 0.f, 0.f });
			set(input_source::mouse, input_code::mouse_position, { xev.xmotion.x, xev.xmotion.y, 0.f });
		}
		break;
		// ButtonPress and ButtonRelease are the same event. Scroll wheel is also a button press event.
		case ButtonPress:
		case ButtonRelease:
		{
			const bool is_press { xev.xbutton.type == ButtonPress };
			const input_code::code code{ xev.xbutton.button == Button1 ? input_code::mouse_left : xev.xbutton.button == Button2 ? input_code::mouse_middle : 
											xev.xbutton.button == Button3 ? input_code::mouse_right : input_code::mouse_wheel };
			
			if (code != input_code::mouse_wheel)
			{
				set(input_source::mouse, code, { xev.xbutton.x, xev.xbutton.y, is_press ? 1.f : 0.f });
			}
			else if (is_press) // Mouse wheel will generate a ButtonPress followed by an immediate ButtonRelease event. We can safely ignore ButtonRelease
			{
				// NOTE: Button4 is scroll wheel up, Button5 is scroll wheel down. There can be Button6 and Button7 which are scroll wheel push left and right,
				//		 respectively, so to insulate aginst any extended buttons here, we will record a delta of 0. X11 doesn't have a GET_WHEEL_DELTA function
				//		 like Win32, so it lacks the ability to handle precision wheel scrolling. A wheel delta of 120.f was chosen to indicate one click scroll up,
				//		 and a delta of -120.f was chosen to indicate one click scroll down, because WHEEL_DELTA in Win32 are multiples of 120 for up, and -120 for down.
				//		 This allows the engine to treat the wheel delta in the same fashion between OSes.
				const f32 wheel_delta = xev.xbutton.button == Button4 ? 120.f : xev.xbutton.button == Button5 ? -120.f : 0.f;
				set(input_source::mouse, input_code::mouse_wheel, { wheel_delta, 0.f, 0.f });
			}
		}
		break;
	}
}

}

#endif // __linux__