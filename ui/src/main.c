#include "utils.h"
#include "ui_defines.h"
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <stdlib.h>

[[nodiscard]]
int main(void) {
	Display *disp = XOpenDisplay(nullptr);
	if (disp == nullptr) {
		fatal("Cannot open Display");
		return EXIT_FAILURE;
	}

	int shape_event_base, shape_error_base;
	if (!XShapeQueryExtension(disp, &shape_event_base, &shape_error_base)) {
		fprintf(stderr, "X Shape extension does not supported.\n");
		XCloseDisplay(disp);
		return EXIT_FAILURE;
	}

	int screen = DefaultScreen(disp);
	Window root_window = RootWindow(disp, screen);
	
	Window window = XCreateSimpleWindow(disp, root_window,
										100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BORDER,
										BlackPixel(disp, screen), WhitePixel(disp, screen)
									);

	XSetWindowAttributes attrs;
	attrs.override_redirect = True;
	XChangeWindowAttributes(disp, window, CWOverrideRedirect, &attrs);
	
	Atom wm_delete_window = XInternAtom(disp, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(disp, window, &wm_delete_window, 1);

	/**
	* Cut a rectangled window to triangled,
	* The rectangled is Zero Opacity.
	*/
	Pixmap shape_mask = XCreatePixmap(disp, window, WINDOW_WIDTH, WINDOW_HEIGHT, 1);
	GC shape_gc = XCreateGC(disp, shape_mask, 0, nullptr);
	XSetForeground(disp, shape_gc, 0);
	XFillRectangle(disp, shape_mask, shape_gc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	XSetForeground(disp, shape_gc, 1);
	XPoint points[] = {
		{WINDOW_WIDTH / 2, 0},
		{0, WINDOW_HEIGHT},
		{WINDOW_WIDTH, WINDOW_HEIGHT - 100},
		{WINDOW_WIDTH / 2, 0}
	};

	XFillPolygon(disp, shape_mask, shape_gc, points, 3, Complex, CoordModeOrigin);
	XShapeCombineMask(
		disp, window,
		ShapeBounding,
		0, 0,
		shape_mask,
		ShapeSet
	);

	GC window_gc = XCreateGC(disp, window, 0, nullptr);
	XSelectInput(disp, window, ExposureMask | KeyPressMask);
	XMapWindow(disp, window);

	XEvent event;
	bool running = true;

	while (running) {
		XNextEvent(disp, &event);
		switch (event.type) {
			case Expose:
				XSetForeground(disp, window_gc, WhitePixel(disp, screen));
				XFillRectangle(disp, window, window_gc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
				break;
			case KeyPress:
				running = false;
				break;
			case ClientMessage:
				if ((Atom)event.xclient.data.l[0] == wm_delete_window) {
					running = false;
				}
				break;
		}
	}
	
	XFreeGC(disp, shape_gc);
	XFreeGC(disp, window_gc);
	XFreePixmap(disp, shape_mask);
	XCloseDisplay(disp);
	return EXIT_SUCCESS;
}
