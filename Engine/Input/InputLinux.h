// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace primal::input {
    
void process_input_message(XEvent xev, Display* display);

}

#endif // !__linux__