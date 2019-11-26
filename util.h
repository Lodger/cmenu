#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>

#define BUFSIZE 100

int read_stdin(char ***lines);
int grab_keyboard(Display *display, Window root);
