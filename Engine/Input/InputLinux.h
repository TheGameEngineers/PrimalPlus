#pragma once
#ifdef __linux__

#include "../Platform/LinuxWindowManager.h"

namespace primal::input {

void process_input_message(const platform::event* const ev, Display* display);

}

#endif // !__linux__