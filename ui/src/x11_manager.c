#include "x11_manager.h"
#include <X11/Xlib.h>
#include <stdio.h>


bool is_window(Display *disp, Window window) {
	XWindowAttributes wa;
	if (XGetWindowAttributes(disp, window, &wa) == 0) {
		fprintf(stderr, "Window is corrupted or not exist./n");
		return false;
	}
	return true;
}

Window get_root_window(Display *disp) {
	int screen = DefaultScreen(disp);
	Window root = RootWindow(disp, screen);
	if (!is_window(disp, root)) {
		return None;
	}

	return root;
}
