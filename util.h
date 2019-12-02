#define _POSIX_C_SOURCE 200890L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>

#define BUFSIZE 100

int read_stdin(char ***lines);
void free_lines(char **lines, int count);
int grab_keyboard(Display *display, Window root);
