#pragma once

#include <X11/Xlib.h>

[[nodiscard]]
Window get_root_window(Display *disp);

[[nodiscard]]
bool is_window(Display *disp, Window window);
