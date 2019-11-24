#include <stdio.h>
#include <string.h>
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
//	if (parse_arguments(argc, argv) != 0)
//		return 1;

	/* read stdin */
	int count;
	char **items;
	count = read_stdin(&items);

	struct XValues xv;
	struct WinValues wv;
	struct XftValues xftv;

	if (init_x(&xv) != 0)
		return 2;

	if (init_window(&xv, &wv) != 0)
		return 2;

	if (init_xft(&xv, &wv, &xftv) != 0)
		return 4;

	grab_keyboard(&xv);

//	Cursor c;
//	c = XCreateFontCursor(xv.display, XC_question_arrow);
//	XDefineCursor(xv.display, wv.window, c);

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
	char *linebuf, *tmp, **realloctmp;
	size_t size;
	unsigned read, allocated;

	f = stdin;
	size = 0;
	read = 0;

	allocated = BUFSIZE;
	*lines = malloc(allocated * sizeof(char*));

	while (getline(&linebuf, &size, f) > 0) {
		linebuf[strlen(linebuf)-1] = '\0'; /* remove newline */
		tmp = strdup(linebuf);
		if (tmp == NULL)
			return -1;
		*(*lines + read) = tmp;
		++read;

		if (read == allocated) {
			allocated *= 2;
			realloctmp = realloc(*lines, allocated * sizeof(char*));
			if (realloctmp == NULL) {
				fprintf(stderr, "read_stdin: couldn't realloc %d bytes\n",
					allocated);
				return -1;
			}
			*lines = realloctmp;
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
	XFree(wv->gc);
	XDestroyWindow(xv->display, wv->window);
	XCloseDisplay(xv->display);
}

int parse_and_allocate_xcolor(struct XValues *xv, char *name, XColor *color)
{
	if (XParseColor(xv->display, xv->colormap, name, color) == 0) {
		fprintf(stderr, "Could not parse color \"%s\"\n", name);
		return -1;
	}

	if (XAllocColor(xv->display, xv->colormap, color) == 0) {
		fprintf(stderr, "Could not allocate color \"%s\"\n", name);
		return -2;
	}

	return 0;
}

int init_window(struct XValues *xv, struct WinValues *wv)
{
	XColor borderpixel;
	parse_and_allocate_xcolor(xv, wincolors[bordercolor], &borderpixel);

	XColor background;
	parse_and_allocate_xcolor(xv, wincolors[bgcolor], &background);

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

	/* while we're here, create the graphics context */
	XGCValues xgcv;
	xgcv.background = background.pixel;
	wv->gc = XCreateGC(xv->display, wv->window, GCBackground, &xgcv);

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
			       &xftv->primaryfg)) {
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
	/* set items[0] to user input */
	char *filtered[count];

	/* filtered[0] is user input + decorations */
	*filtered = malloc(LENINPUT * sizeof(char));
	XSelectInput(xv->display, wv->window, ExposureMask |
	                                      KeyPressMask |
	                                      PointerMotionMask |
	                                      ButtonPressMask);

	char input[MAXINPUT];

	XEvent e;
	KeySym keysym;
	int keystatus, selected, location;

	keystatus = 0; /* do nothing */
	selected = 0; /* initially, the mouse has not selected anything */
	for (;;) {
		XMapWindow(xv->display, wv->window);
		XNextEvent(xv->display, &e);

		/* xfilterevent somewhere? */
		/* handle event */
		switch(e.type) {
		case Expose:
			break;
		case KeyPress:
			keystatus = handle_key(xv, e.xkey, input);
			break;
		case KeyRelease:
			continue;
		default:
			continue;
		}

		/* update the menu */
		snprintf(*filtered, LENINPUT, "%s%s%s%s",
		         inputprompt, inputprefix, *items, inputsuffix);

		count = filter_input(items, filtered+1, input) + 1;

		switch(keystatus) {
		case EXIT:
			puts(count > 1 ? filtered[1] : input);
		case TERM:
			free(*filtered);
			break;
		case SHIFTUP:
		case SHIFTDOWN:
			if (count > 1)
				rotate_array(filtered+1, count-1, keystatus);
		}

		/* will it segfault if I remove this if? */
//		if (count > 1)
//			rotate_array(filtered+1, count-1, keystatus);

		/* status is used to determine the shift amount */
		draw_menu(xv, wv, xftv, filtered,
			  strlen(input) > 0 || itemsvisible ? count : 1,
			  keystatus);
	}
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
			return SHIFTUP;
			break;
		case 'n':
			return SHIFTDOWN;
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

int filter_input(char **source, char **out, char *filter)
{
	unsigned count;

	for (; *source; ++source) {
		if (strlen(*source) && strstr(*source, filter) == *source) {
			*out++ = *source;
			++count;
		}
	}

	return count;
}

void rotate_array(char **array, int count, int delta)
{
	static int offset = 0;
	offset = (offset + delta) % count;
	if (offset > 0) {
		char *tmp[count];

		memcpy(tmp, array + offset, (count - offset) * sizeof(char*));
		memcpy(tmp + count - offset, array, offset * sizeof(char*));
		memcpy(array, tmp, sizeof(tmp));
	} else if (offset < 0) {
		char *tmp[count];
		offset = abs(offset);

		memcpy(tmp, array + count - offset, offset * sizeof(char*));
		memcpy(tmp + offset, array, (count - offset) * sizeof(char*));
		memcpy(array, tmp, sizeof(tmp));

		offset *= -1;
	}
}

void draw_menu(struct XValues *xv, struct WinValues *wv, struct XftValues *xftv,
               char *items[], int count, int shift)
{
	count = move_and_resize(xv, wv, xftv, items, count);
//	XftDrawRect(xftv->draw, &xftv->colors[bgcolor], 0, 0, wv->xwc.width,
//	            wv->xwc.height);
	draw_items(xftv, items, count);

	if (count > 1)
		draw_string(xv, wv, xftv, items[1], 0, 0);
}

int move_and_resize(struct XValues *xv, struct WinValues *wv,
                    struct XftValues *xftv, char *items[], int count)
{

	int total_displayed;
	total_displayed = count;

	XGlyphInfo ext;

	int longest;
	for (longest = 0; count--; items++) {
		XftTextExtentsUtf8(xv->display, xftv->font, *items,
				   strlen(*items), &ext);

		if (ext.width > longest)
			longest = ext.width;
	}
		
	wv->xwc.width = longest + padding[2] + padding[3];
	wv->xwc.height = xftv->font->height * total_displayed
	                 + padding[0] + padding[1];

	/* Y axis */
	if (wv->xwc.height > xv->screen_height - wv->xwc.y) {
		/* if the window height is greater than the screen height */
		if (wv->xwc.height > xv->screen_height) {
			wv->xwc.y = 0;

			/* clip the menu items */
			total_displayed = (xv->screen_height - wv->xwc.y
			                  - padding[0] + padding[1])
			                  / xftv->font->height;

			wv->xwc.height = xftv->font->height * total_displayed +
			                 padding[0] + padding[1];
		} else {
		/* if they menu needs to move up */
			wv->xwc.y = xv->screen_height - wv->xwc.height;
		}

		/* mouse follows the window corner */
		XWarpPointer(xv->display, None, xv->root, 0, 0, 0, 0,
			     wv->xwc.x, wv->xwc.y);
	}

	/* X axis */
	if (wv->xwc.width > xv->screen_width - wv->xwc.x) {
		if (wv->xwc.width > xv->screen_width) {
			wv->xwc.x = 0;
			wv->xwc.width = xv->screen_width;
		} else {
			wv->xwc.x = xv->screen_width - wv->xwc.width;
		}

		XWarpPointer(xv->display, None, xv->root, 0, 0, 0, 0,
			     wv->xwc.x, wv->xwc.y);
	}

	/* update window location */
	XConfigureWindow(xv->display, wv->window,
	                 CWX | CWY | CWWidth | CWHeight, &wv->xwc);
	return total_displayed;
}

void draw_items(struct XftValues *xftv, char *items[], int count)
{
	int ascent, height;
	ascent = xftv->font->ascent;
	height = xftv->font->height;

	int line;
	for (line = ascent + padding[0]; count--; line += height, ++items) {
//		XftDrawStringUtf8(xftv->draw, &xftv->colors[fgcolor],
//		                  xftv->font, padding[2], line, *items,
//		                  strlen(*items));
	}
}

void draw_string(struct XValues *xv, struct WinValues *wv,
                 struct XftValues *xftv, char *line, int index, short swap)
{
	int y, rheight, lheight;

	y = xftv->font->ascent + xftv->font->descent + padding[0];
	lheight = xftv->font->ascent + xftv->font->height + padding[0];
	rheight = y - padding[0];

	/* calculate offsets */
	y += xftv->font->height * index;
	lheight += xftv->font->height * index;

//	XftDrawRect(xftv->draw, &xftv->colors[!swap ? abgcolor : bgcolor],
//	            0, y, wv->xwc.width, rheight);

//	XftDrawStringUtf8(xftv->draw,
//	                  &xftv->colors[!swap ? afgcolor : fgcolor],
//	                  xftv->font, padding[2], lheight, line, strlen(line));
}
