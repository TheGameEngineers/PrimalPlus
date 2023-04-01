#ifdef __linux__

#include "InputLinux.h"
#include "Input.h"

namespace primal::input
{
	
namespace {

std::unordered_map<u32, u32> lk_mapping;

bool fill_keys()
{	
	lk_mapping[platform::key_backspace] = input_code::key_backspace;
	lk_mapping[platform::key_tab] = input_code::key_tab;
	lk_mapping[platform::key_return] = input_code::key_return;
	lk_mapping[platform::key_shift_l] = lk_mapping[platform::key_shift_r] = input_code::key_shift;
	lk_mapping[platform::key_control_l] = lk_mapping[platform::key_control_r] = input_code::key_control;
	lk_mapping[platform::key_alt_l] = lk_mapping[platform::key_alt_r] = input_code::key_alt;
	lk_mapping[platform::key_pause] = input_code::key_pause;
	lk_mapping[platform::key_capslock] = input_code::key_capslock;
	lk_mapping[platform::key_escape] = input_code::key_escape;
	lk_mapping[platform::key_space] = input_code::key_space;
	lk_mapping[platform::key_page_up] = input_code::key_page_up;
	lk_mapping[platform::key_page_down] = input_code::key_page_down;
	lk_mapping[platform::key_home] = input_code::key_home;
	lk_mapping[platform::key_end] = input_code::key_end;
	lk_mapping[platform::key_left] = input_code::key_left;
	lk_mapping[platform::key_up] = input_code::key_up;
	lk_mapping[platform::key_right] = input_code::key_right;
	lk_mapping[platform::key_down] = input_code::key_down;
	lk_mapping[platform::key_print_screen] = input_code::key_print_screen;
	lk_mapping[platform::key_insert] = input_code::key_insert;
	lk_mapping[platform::key_delete] = input_code::key_delete;

	lk_mapping[platform::key_0] = input_code::key_0;
	lk_mapping[platform::key_1] = input_code::key_1;
	lk_mapping[platform::key_2] = input_code::key_2;
	lk_mapping[platform::key_3] = input_code::key_3;
	lk_mapping[platform::key_4] = input_code::key_4;
	lk_mapping[platform::key_5] = input_code::key_5;
	lk_mapping[platform::key_6] = input_code::key_6;
	lk_mapping[platform::key_7] = input_code::key_7;
	lk_mapping[platform::key_8] = input_code::key_8;
	lk_mapping[platform::key_9] = input_code::key_9;

	lk_mapping[platform::key_a] = lk_mapping[platform::key_A] = input_code::key_a;
	lk_mapping[platform::key_b] = lk_mapping[platform::key_B] = input_code::key_b;
	lk_mapping[platform::key_c] = lk_mapping[platform::key_C] = input_code::key_c;
	lk_mapping[platform::key_d] = lk_mapping[platform::key_D] = input_code::key_d;
	lk_mapping[platform::key_e] = lk_mapping[platform::key_E] = input_code::key_e;
	lk_mapping[platform::key_f] = lk_mapping[platform::key_F] = input_code::key_f;
	lk_mapping[platform::key_g] = lk_mapping[platform::key_G] = input_code::key_g;
	lk_mapping[platform::key_h] = lk_mapping[platform::key_H] = input_code::key_h;
	lk_mapping[platform::key_i] = lk_mapping[platform::key_I] = input_code::key_i;
	lk_mapping[platform::key_j] = lk_mapping[platform::key_J] = input_code::key_j;
	lk_mapping[platform::key_k] = lk_mapping[platform::key_K] = input_code::key_k;
	lk_mapping[platform::key_l] = lk_mapping[platform::key_L] = input_code::key_l;
	lk_mapping[platform::key_m] = lk_mapping[platform::key_M] = input_code::key_m;
	lk_mapping[platform::key_n] = lk_mapping[platform::key_N] = input_code::key_n;
	lk_mapping[platform::key_o] = lk_mapping[platform::key_O] = input_code::key_o;
	lk_mapping[platform::key_p] = lk_mapping[platform::key_P] = input_code::key_p;
	lk_mapping[platform::key_q] = lk_mapping[platform::key_Q] = input_code::key_q;
	lk_mapping[platform::key_r] = lk_mapping[platform::key_R] = input_code::key_r;
	lk_mapping[platform::key_s] = lk_mapping[platform::key_S] = input_code::key_s;
	lk_mapping[platform::key_t] = lk_mapping[platform::key_T] = input_code::key_t;
	lk_mapping[platform::key_u] = lk_mapping[platform::key_U] = input_code::key_u;
	lk_mapping[platform::key_v] = lk_mapping[platform::key_V] = input_code::key_v;
	lk_mapping[platform::key_w] = lk_mapping[platform::key_W] = input_code::key_w;
	lk_mapping[platform::key_x] = lk_mapping[platform::key_X] = input_code::key_x;
	lk_mapping[platform::key_y] = lk_mapping[platform::key_Y] = input_code::key_y;
	lk_mapping[platform::key_z] = lk_mapping[platform::key_Z] = input_code::key_z;

	lk_mapping[platform::key_numpad_0] = input_code::key_numpad_0;
	lk_mapping[platform::key_numpad_1] = input_code::key_numpad_1;
	lk_mapping[platform::key_numpad_2] = input_code::key_numpad_2;
	lk_mapping[platform::key_numpad_3] = input_code::key_numpad_3;
	lk_mapping[platform::key_numpad_4] = input_code::key_numpad_4;
	lk_mapping[platform::key_numpad_5] = input_code::key_numpad_5;
	lk_mapping[platform::key_numpad_6] = input_code::key_numpad_6;
	lk_mapping[platform::key_numpad_7] = input_code::key_numpad_7;
	lk_mapping[platform::key_numpad_8] = input_code::key_numpad_8;
	lk_mapping[platform::key_numpad_9] = input_code::key_numpad_9;

	lk_mapping[platform::key_multiply] = input_code::key_multiply;
	lk_mapping[platform::key_add] = input_code::key_add;
	lk_mapping[platform::key_subtract] = input_code::key_subtract;
	lk_mapping[platform::key_decimal] = input_code::key_decimal;
	lk_mapping[platform::key_divide] = input_code::key_divide;

	lk_mapping[platform::key_f1] = input_code::key_f1;
	lk_mapping[platform::key_f2] = input_code::key_f2;
	lk_mapping[platform::key_f3] = input_code::key_f3;
	lk_mapping[platform::key_f4] = input_code::key_f4;
	lk_mapping[platform::key_f5] = input_code::key_f5;
	lk_mapping[platform::key_f6] = input_code::key_f6;
	lk_mapping[platform::key_f7] = input_code::key_f7;
	lk_mapping[platform::key_f8] = input_code::key_f8;
	lk_mapping[platform::key_f9] = input_code::key_f9;
	lk_mapping[platform::key_f10] = input_code::key_f10;
	lk_mapping[platform::key_f11] = input_code::key_f11;
	lk_mapping[platform::key_f12] = input_code::key_f12;

	lk_mapping[platform::key_numlock] = input_code::key_numlock;
	lk_mapping[platform::key_scrolllock] = input_code::key_scrollock;
	lk_mapping[platform::key_tilde] = input_code::key_tilde;

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
		right_alt = 0x04,
	};
};

// Initialize lk_mapping key map once when everything else is loading
bool keymap_ready{ fill_keys() };
u8 modifier_keys_state{ 0 };

void
set_modifier_input(const platform::event* const ev, platform::key_code check_mod, input_code::code code, modifier_flags::flags flags)
{
	if (ev->event_type == platform::event::key_press && (platform::key_code)ev->key_sym == check_mod)
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
set_modifier_inputs(input_code::code code, const platform::event* const ev)
{
	if (code == input_code::key_shift)
	{
		set_modifier_input(ev, platform::key_shift_l, input_code::key_left_shift, modifier_flags::left_shift);
		set_modifier_input(ev, platform::key_shift_r, input_code::key_right_shift, modifier_flags::right_shift);
	}
	else if (code == input_code::key_control)
	{
		set_modifier_input(ev, platform::key_control_l, input_code::key_left_control, modifier_flags::left_control);
		set_modifier_input(ev, platform::key_control_r, input_code::key_right_control, modifier_flags::right_control);
	}
	else if (code == input_code::key_alt)
	{
		set_modifier_input(ev, platform::key_alt_l, input_code::key_left_alt, modifier_flags::left_alt);
		set_modifier_input(ev, platform::key_alt_r, input_code::key_right_alt, modifier_flags::right_alt);
	}
}
} // anonymous namespace

u32
get_key(u32 key_sym)
{
	assert(keymap_ready);

	return lk_mapping.count(key_sym) ? lk_mapping[key_sym] : u32_invalid_id;
}

void
process_input_message(const platform::event* const ev, Display* display)
{
	switch (ev->event_type)
	{		
	case platform::event::key_press:
	{
		const input_code::code code{ get_key(ev->key_sym) };
		if (code != u32_invalid_id)
		{
			set(input_source::keyboard, code, { 1.f, 0.f, 0.f });
			set_modifier_inputs(code, ev);
		}
	}
	break;
	case platform::event::key_release:
	{				
		const input_code::code code{ get_key(ev->key_sym) };
		if (code != u32_invalid_id)
		{
			set(input_source::keyboard, code, { 0.f, 0.f, 0.f });
			set_modifier_inputs(code, ev);
		}
	}
	break;
	case platform::event::motion_notify:
	{
		set(input_source::mouse, input_code::mouse_position_x, { ev->mouse_x, 0.f, 0.f });
		set(input_source::mouse, input_code::mouse_position_y, { ev->mouse_y, 0.f, 0.f });
		set(input_source::mouse, input_code::mouse_position, { ev->mouse_x, ev->mouse_y, 0.f });
	}
	break;
	// Scroll wheel is also a button press/release event.
	case platform::event::button_press:
	{
		using namespace platform;
		const input_code::code code{ ev->button == event::button_1 ? input_code::mouse_left : ev->button == event::button_2 ? input_code::mouse_middle : 
									 ev->button == event::button_3 ? input_code::mouse_right : input_code::mouse_wheel };
		if (code != input_code::mouse_wheel)
		{
			set(input_source::mouse, code, { ev->mouse_x, ev->mouse_y, 1.f });
		}
		else
		{
			set(input_source::mouse, input_code::mouse_wheel, { ev->wheel_delta, 0.f, 0.f });
		}
	}
	break;
	case platform::event::button_release:
	{
		using namespace platform;
		const input_code::code code{ ev->button == event::button_1 ? input_code::mouse_left : ev->button == event::button_2 ? input_code::mouse_middle : 
									 ev->button == event::button_3 ? input_code::mouse_right : input_code::mouse_wheel };
		
		// NOTE: mouse wheel will generate a button_press followed by an immediate button_release event. We can safely ignore button_release
		if (code != input_code::mouse_wheel)
		{
			set(input_source::mouse, code, { ev->mouse_x, ev->mouse_y, 0.f });
		}
	}
	break;
	}
}

}

#endif // __linux__