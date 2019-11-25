#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>
#include <X11/cursorfont.h>

#include <X11/keysym.h>

#include "cmenu.h"
#include "config.h"

int main(int argc, char *argv[])
{
	if (parse_arguments(argc, argv) != 0)
		return 1;

	/* read stdin */
	int count;
	char **items;
	count = read_stdin(&items);

	if (count < 1)
		return 2;

	struct XValues xv;
	struct WinValues wv;
	struct XftValues xftv;

	if (init_x(&xv) != 0)
		return 3;

	if (init_window(&xv, &wv) != 0)
		return 4;

	if (init_xft(&xv, &wv, &xftv) != 0)
		return 5;

	grab_keyboard(&xv);

	Cursor c;
	c = XCreateFontCursor(xv.display, XC_question_arrow);
	XDefineCursor(xv.display, wv.window, c);

	int px, py;
	get_pointer(&xv, &px, &py);

	menu_run(&xv, &wv, &xftv, items, count);

	while (count--)
		free(items[count]);
	free(items);

	XWarpPointer(xv.display, None, xv.root, 0, 0, 0, 0, px, py);

	terminate_xft(&xv, &xftv);
	terminate_x(&xv, &wv);
	return 0;
}

unsigned parse_arguments(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-v") ||
		    !strcmp(argv[i], "--visible"))
			itemsvisible = True;
		else if (!strcmp(argv[i], "-f") ||
			 !strcmp(argv[i], "--font"))
			fontname = argv[++i];
		else if (!strcmp(argv[i], "-fg") ||
			 !strcmp(argv[i], "--foreground"))
			wincolors[fgcolor] = argv[++i];
		else if (!strcmp(argv[i], "-afg") ||
			 !strcmp(argv[i], "--active-foreground"))
			wincolors[afgcolor] = argv[++i];
		else if (!strcmp(argv[i], "-bg") ||
			 !strcmp(argv[i], "--backgroud"))
			wincolors[bgcolor] = argv[++i];
		else if (!strcmp(argv[i], "-abg") ||
			 !strcmp(argv[i], "--active-background"))
			wincolors[abgcolor] = argv[++i];
		else if (!strcmp(argv[i], "-b") ||
			 !strcmp(argv[i], "--border"))
			borderwidth = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-bc") ||
			 !strcmp(argv[i], "--border-color"))
			wincolors[bordercolor] = argv[++i];
		else if (!strcmp(argv[i], "-p") ||
			 !strcmp(argv[i], "--padding"))
			padding[0] = padding[1] = padding[2] = padding[3] =
			atoi(argv[++i]);
		else if (!strcmp(argv[i], "-pt") ||
			 !strcmp(argv[i], "--padding-top"))
			padding[0] = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-pb") ||
			 !strcmp(argv[i], "--padding-bottom"))
			padding[1] = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-pl") ||
			 !strcmp(argv[i], "--padding-left"))
			padding[2] = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-pr") ||
			 !strcmp(argv[i], "--padding-right"))
			padding[3] = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-pm") ||
			 !strcmp(argv[i], "--prompt"))
			inputprompt = argv[++i];
		else if (!strcmp(argv[i], "-ip") ||
			 !strcmp(argv[i], "--input-prefox"))
			inputprefix = argv[++i];
		else if (!strcmp(argv[i], "-is") ||
			 !strcmp(argv[i], "--input-suffix"))
			inputsuffix = argv[++i];
		else {
			printf("Unknown option: \"%s\"\n", argv[i]);
			fputs("Usage: cmenu [-v] [-f font] [-fg color] [-afg color] [-bg color] [-abg color]\n"
			      "             [-b width] [-bc color] [-p width] [-pr prompt]\n"
			      "             [-ip prefix] [-is suffix]\n", stderr);
			return 1;
		}
	}
	return 0;
}

int read_stdin(char ***lines)
{
	FILE *f;
	char *linebuf;
	size_t size;
	unsigned read, allocated;

	f = stdin;
	size = 0;
	read = 0;

	allocated = BUFSIZE;
	*lines = malloc(allocated * sizeof(char*));

	while (getline(&linebuf, &size, f) > 0) {
		linebuf[strlen(linebuf)-1] = '\0'; /* remove newline */
		*(*lines + read) = strdup(linebuf);
		if (*(*lines + read) == NULL)
			return -1;
		++read;

		if (read == allocated) {
			allocated *= 2;
			*lines = realloc(*lines, allocated * sizeof(char*));
			if (*lines == NULL) {
				fprintf(stderr, "read_stdin: couldn't realloc %d bytes\n",
					allocated);
				return -1;
			}
		}
	}

	free(linebuf);
	return read;
}

int init_x(struct XValues *xv)
{
	xv->display = XOpenDisplay(NULL);
	if (!xv->display) {
		fputs("Could not connect to the X server", stderr);
		return 1;
	}
	xv->screen_num = DefaultScreen(xv->display);
	xv->root = RootWindow(xv->display, xv->screen_num);
	xv->visual = DefaultVisual(xv->display, xv->screen_num);
	xv->colormap = DefaultColormap(xv->display, xv->screen_num);
	xv->screen_height = DisplayHeight(xv->display, xv->screen_num);
	xv->screen_width = DisplayWidth(xv->display, xv->screen_num);

	return 0;
}

void terminate_x(struct XValues *xv, struct WinValues *wv)
{
	XFree(wv->primaryGC);
	XFree(wv->activeGC);
	XDestroyWindow(xv->display, wv->window);
	XCloseDisplay(xv->display);
}

int parse_and_allocate_xcolor(struct XValues *xv, char *name, XColor *color)
{
	if (XParseColor(xv->display, xv->colormap, name, color) == 0) {
		fprintf(stderr, "Could not parse color \"%s\"\n", name);
		return 1;
	}

	if (XAllocColor(xv->display, xv->colormap, color) == 0) {
		fprintf(stderr, "Could not allocate color \"%s\"\n", name);
		return 2;
	}

	return 0;
}

int init_window(struct XValues *xv, struct WinValues *wv)
{
	XColor borderpixel;
	if (parse_and_allocate_xcolor(xv, wincolors[bordercolor],
	                              &borderpixel) != 0) {
		return 1;
	}

	XColor background;
	if (parse_and_allocate_xcolor(xv, wincolors[bgcolor],
	                              &background) != 0) {
		return 1;
	}

	XSetWindowAttributes wa;
	wa.override_redirect = True;
	wa.border_pixel = borderpixel.pixel;
	wa.background_pixel = background.pixel;
	wa.event_mask = ExposureMask | KeyPressMask;

	unsigned long valuemask;
	valuemask = CWOverrideRedirect | CWBackPixel | CWEventMask |
	            CWBorderPixel;

	get_pointer(xv, &wv->xwc.x, &wv->xwc.y);

	wv->window = XCreateWindow(xv->display, xv->root, wv->xwc.x, wv->xwc.y,
	                           1, 1, borderwidth, CopyFromParent,
	                           CopyFromParent, xv->visual, valuemask, &wa);

	/* while we're here, create the graphics contexts */
	XGCValues xgcv;
	xgcv.foreground = background.pixel;
	wv->primaryGC = XCreateGC(xv->display, wv->window, GCForeground, &xgcv);
	XSetBackground(xv->display, wv->primaryGC, background.pixel);

	if (parse_and_allocate_xcolor(xv, wincolors[abgcolor],
	                              &background) != 0) {
		return 1;
	}
	xgcv.foreground = background.pixel;
	wv->activeGC = XCreateGC(xv->display, wv->window, GCForeground, &xgcv);

	return 0;
}

int init_xft(struct XValues *xv, struct WinValues *wv, struct XftValues *xftv)
{
	xftv->font = XftFontOpenName(xv->display, xv->screen_num, fontname);
	if (!xftv->font) {
		fprintf(stderr, "Could not open font \"%s\"\n", fontname);
		return 1;
	}
	if (!XftColorAllocName(xv->display, xv->visual, xv->colormap,
			       wincolors[fgcolor],
			       &xftv->primaryfg)) {
		fprintf(stderr, "Could not allocate color \"%s\"\n",
			wincolors[fgcolor]);
		return 2;
	}
	if (!XftColorAllocName(xv->display, xv->visual, xv->colormap,
			       wincolors[afgcolor],
			       &xftv->activefg)) {
		fprintf(stderr, "Could not allocate color \"%s\"\n",
			wincolors[afgcolor]);
		return 2;
	}
	xftv->draw = XftDrawCreate(xv->display, wv->window, xv->visual,
	                           xv->colormap);
	return 0;
}

void terminate_xft(struct XValues *xv, struct XftValues *xftv)
{
	XftFontClose(xv->display, xftv->font);

	XftColorFree(xv->display, xv->visual, xv->colormap,
	             &xftv->primaryfg);
	XftColorFree(xv->display, xv->visual, xv->colormap,
	             &xftv->activefg);

	XftDrawDestroy(xftv->draw);
}

void get_pointer(struct XValues *xv, int *x, int *y)
{
	/* trash values */
	Window ret_root, ret_child;
	int ret_rootx, ret_rooty;
	unsigned int ret_mask;

	XQueryPointer(xv->display, RootWindow(xv->display, xv->screen_num),
	              &ret_root, &ret_child, &ret_rootx, &ret_rooty,
	              x, y, &ret_mask);
}

int grab_keyboard(struct XValues *xv)
{
	struct timespec interval;
	interval.tv_sec = 0;
	interval.tv_nsec = 1000000;

	for (int i = 0; i < ATTEMPTS; ++i) {
		if (XGrabKeyboard(xv->display, xv->root, True, GrabModeAsync,
				  GrabModeAsync, CurrentTime) == GrabSuccess)
			return 0;
		nanosleep(&interval, NULL);
	}
	fprintf(stderr, "Unable to grab keyboard");
	return 1;
}

void menu_run(struct XValues *xv, struct WinValues *wv, struct XftValues *xftv,
              char **items, int count)
{
	XSelectInput(xv->display, wv->window, ExposureMask |
	                                      KeyPressMask |
	                                      PointerMotionMask |
	                                      ButtonPressMask);

	char input[MAXINPUT];
	input[0] = '\0';

	char *filtered[count+1];
	*filtered = malloc(LENINPUT * sizeof(char));

	if (*filtered == NULL) {
		fprintf(stderr, "Could not allocate bytes\n");
		return;
	}

	XEvent e;
	KeySym keysym;
	int keystatus, offset, subcount;
	int hover, old;

	keystatus = 0; /* do nothing */
	hover = old = 1; /* the mouse has not selected anything */
	for (;;) {
		XMapWindow(xv->display, wv->window);
		XNextEvent(xv->display, &e);

		/* xfilterevent somewhere? */
		switch(e.type) {
		case Expose:
			break;
		case KeyPress:
			keystatus = handle_key(xv, e.xkey, input);
			break;
		case MotionNotify:
			hover = get_index_from_coords(wv, xftv, e.xbutton.x,
			                              e.xbutton.y);
			if (hover > 0 && hover <= subcount && hover != old) {
				highlight_entry(xv, wv, xftv, old,
				                wv->primaryGC);
				draw_string(xftv, filtered[old], old,
				            xftv->primaryfg);
				highlight_entry(xv, wv, xftv, hover,
				                wv->activeGC);
				draw_string(xftv, filtered[hover], hover,
				            xftv->activefg);
				old = hover;
			}
			continue;
		case ButtonPress:
			if (hover > 0 && hover <= subcount) {
				puts(filtered[hover]);
				keystatus = TERM;
			}
			break;
		default:
			continue;
		}

		/* update the menu */
		subcount = filter_input(items, count, filtered+1, input) + 1;

		if (keystatus == SHIFTDOWN || keystatus == SHIFTUP)
			offset += keystatus;

		if (subcount > 1) {
			printf("rotating %d\n", offset);
			rotate_array(filtered+1, subcount-1, offset);
		}

		switch(keystatus) {
		case EXIT:
			puts(subcount > 1 ? filtered[1] : input);
		case TERM:
			free(*filtered);
			return;
  		}

		snprintf(*filtered, LENINPUT, "%s%s%s%s",
		         inputprompt, inputprefix, input, inputsuffix);

		redraw_menu(xv, wv, xftv, filtered,
			    strlen(input) > 0 || itemsvisible ? subcount : 1);
	}
}

int get_index_from_coords(struct WinValues *wv, struct XftValues *xftv,
                          unsigned x, unsigned y)
{
	if (x > wv->xwc.x + wv->xwc.width)
		return -1;
	return floor(y / xftv->font->height);
}

/*
 * a long function, but it evaluates the key pressed and either adds to the
 * input line, subtracts from it, or returns either TERM (exit without output),
 * EXIT (exit normally), or SHIFTUP/SHIFTDOWN to rotate the menu items.
 */
int handle_key(struct XValues *xv, XKeyEvent xkey, char *inputline)
{
	static char *pos = NULL;
	if (pos == NULL)
		pos = inputline;

	KeySym sym;
	sym = XkbKeycodeToKeysym(xv->display, xkey.keycode, 0,
	                         xkey.state & ShiftMask ? 1 : 0);

	if (xkey.state & ControlMask)
		switch(sym) {
		case 'p':
			return SHIFTDOWN;
			break;
		case 'n':
			return SHIFTUP;
			break;
		case 'h':
			if (pos - inputline > 0)
				*--pos = '\0';

			return 0;
			break;
		}

	switch(sym) {
	case XK_Super_L:   case XK_Super_R:
	case XK_Control_L: case XK_Control_R:
	case XK_Shift_R:   case XK_Shift_L:
		break;
	case XK_Escape:
		return TERM;
	case XK_Return:
		return EXIT;
	case XK_Up:
		return SHIFTUP;
	case XK_Down:
		return SHIFTDOWN;
	case XK_BackSpace:
		if (pos - inputline > 0)
			*--pos = '\0';
		break;
	default:
		if (pos - inputline < MAXINPUT - 1) {
			*pos++ = sym;
			*pos = '\0';
		}
		break;
	}

	return 0;
}

unsigned filter_input(char **source, unsigned count, char **out, char *filter)
{
	unsigned filtered;

	filtered = 0;
	for (; count--; ++source)
		if (strlen(filter) && strstr(*source, filter) == *source) {
			*out++ = *source;
			++filtered;
		}

	return filtered;
}

void rotate_array(char **array, unsigned count, int offset)
{

	if (offset > 0) {
		offset = offset % count;

		char *tmp[count];

		memcpy(tmp, array + offset, (count - offset) * sizeof(char*));
		memcpy(tmp + count - offset, array, offset * sizeof(char*));
		memcpy(array, tmp, sizeof(tmp));
	} else if (offset < 0) {
		offset = abs(offset);
		offset = offset % count;

		char *tmp[count];

		memcpy(tmp, array + count - offset, offset * sizeof(char*));
		memcpy(tmp + offset, array, (count - offset) * sizeof(char*));
		memcpy(array, tmp, sizeof(tmp));
	}
}

void redraw_menu(struct XValues *xv, struct WinValues *wv,
                 struct XftValues *xftv, char **items, int count)
{
	count = move_and_resize(xv, wv, xftv, items, count);

	XClearWindow(xv->display, wv->window);
	draw_items(xftv, items, count);

	if (count > 1) {
		highlight_entry(xv, wv, xftv, 1, wv->activeGC);
		draw_string(xftv, *(items + 1), 1, xftv->activefg);
	}
}

int move_and_resize(struct XValues *xv, struct WinValues *wv,
                    struct XftValues *xftv, char **items, unsigned count)
{
	int total_displayed;
	total_displayed = count;

	XGlyphInfo ext;
	unsigned longest;

	longest = 0;
	while (count--) {
		XftTextExtentsUtf8(xv->display, xftv->font, *items,
				   strlen(*items), &ext);
		if (ext.width > longest)
			longest = ext.width;

		++items;
	}
		
	wv->xwc.width = longest + padding[left] + padding[right];
	wv->xwc.height = xftv->font->height * total_displayed +
	                 padding[top] + padding[bottom];

	/* Y axis */
	if (wv->xwc.height + (borderwidth * 2) >
	    xv->screen_height - wv->xwc.y) {
		if (wv->xwc.height + (borderwidth * 2) > xv->screen_height) {
			wv->xwc.y = 0;

			/* clip the menu items */
			total_displayed = (xv->screen_height - wv->xwc.y -
			                   padding[top] - padding[bottom] -
			                   (borderwidth* 2)) /
			                  xftv->font->height;

			wv->xwc.height = xftv->font->height * total_displayed +
			                 padding[top] + padding[bottom];
		} else {
			wv->xwc.y = xv->screen_height - wv->xwc.height -
			            (borderwidth * 2);
		}

		/* mouse follows the window corner */
		XWarpPointer(xv->display, None, xv->root, 0, 0, 0, 0,
			     wv->xwc.x, wv->xwc.y);
	}

	/* X axis */
	if (wv->xwc.width + (borderwidth * 2) > xv->screen_width - wv->xwc.x) {
		if (wv->xwc.width + (borderwidth * 2) > xv->screen_width) {
			wv->xwc.x = 0;
			wv->xwc.width = xv->screen_width;
		} else {
			wv->xwc.x = xv->screen_width - wv->xwc.width -
			            (borderwidth * 2);
		}

		XWarpPointer(xv->display, None, xv->root, 0, 0, 0, 0,
			     wv->xwc.x, wv->xwc.y);
	}

	unsigned long valuemask;
	valuemask = CWX | CWY | CWWidth | CWHeight;

	/* update window location */
	XConfigureWindow(xv->display, wv->window, valuemask, &wv->xwc);

	return total_displayed;
}

void draw_items(struct XftValues *xftv, char **items, unsigned count)
{
	int index;
	for (index = 0; index < count; ++index)
		draw_string(xftv, *items++, index, xftv->primaryfg);
}

void draw_string(struct XftValues *xftv, char *line, int index, XftColor fg)
{
	unsigned lineheight;

	lineheight = xftv->font->ascent + padding[top] +
	             (index * xftv->font->height);

	XftDrawStringUtf8(xftv->draw, &fg, xftv->font, padding[right],
			  lineheight, line, strlen(line));
}

void highlight_entry(struct XValues *xv, struct WinValues *wv,
                     struct XftValues *xftv, int index, GC bg)
{
	unsigned y;

	y = xftv->font->height + padding[top] +
	    ((index - 1) * xftv->font->height);

	XFillRectangle(xv->display, wv->window, bg, padding[right], y,
		       wv->xwc.width, xftv->font->height);
}
