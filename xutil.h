#include <stdio.h>
#include <time.h>

#include <X11/Xlib.h>

int parse_and_allocate_xcolor(Display *display, Colormap colormap, char *name,
                              XColor *color);
int grab_keyboard(Display *display, Window root);
void get_pointer_location(Display *display, Window root, int *x, int *y);
