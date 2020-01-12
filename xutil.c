#include "xutil.h"

int parse_and_allocate_xcolor(Display *display, Colormap colormap, char *name,
                              XColor *color)
{
	if (!XParseColor(display, colormap, name, color)) {
		fprintf(stderr, "Could not parse color \"%s\"\n", name);
		return 1;
	}

	if (!XAllocColor(display, colormap, color)) {
		fprintf(stderr, "Could not allocate color \"%s\"\n", name);
		return 1;
	}

	return 0;
}

int grab_keyboard(Display *display, Window root)
{
	struct timespec interval;
	interval.tv_sec = 0;
	interval.tv_nsec = 1000000;

	for (int i = 0; i < 200; ++i) {
		if (XGrabKeyboard(display, root, True, GrabModeAsync,
				  GrabModeAsync, CurrentTime) == GrabSuccess)
			return 0; 
		nanosleep(&interval, NULL);
	}
	fprintf(stderr, "Unable to grab keyboard");
	return 1;
}

void get_pointer_location(Display *display, Window root, int *x, int *y)
{
	/* trash values */
	Window ret_root, ret_child;
	int ret_rootx, ret_rooty;
	unsigned int ret_mask;

	XQueryPointer(display, root, &ret_root, &ret_child, &ret_rootx,
	              &ret_rooty, x, y, &ret_mask);
}
