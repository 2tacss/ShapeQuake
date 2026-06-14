#ifndef SQ_UI_X11_MANAGER_H_
#define SQ_UI_X11_MANAGER_H_

#include <X11/Xlib.h>

[[nodiscard]]
Window get_root_window(Display *disp);

[[nodiscard]]
bool is_window(Display *disp, Window window);

#endif
