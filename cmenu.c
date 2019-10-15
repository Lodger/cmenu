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
	/* parse argv */
	for (int i = 1; i < argc; ++i)
		if (!strcmp(argv[i], "-v"))
			visible = True;
		else if (!strcmp(argv[i], "-bg"))
			wincolors[bgcolor] = argv[++i];
		else if (!strcmp(argv[i], "-sbg"))
			wincolors[sbgcolor] = argv[++i];
		else if (!strcmp(argv[i], "-t"))
			wincolors[textcolor] = argv[++i];
		else if (!strcmp(argv[i], "-st"))
			wincolors[stextcolor] = argv[++i];
		else if (!strcmp(argv[i], "-b"))
			wincolors[bordercolor] = argv[++i];
		else if (!strcmp(argv[i], "-bw"))
			borderwidth = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-p"))
			padding = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-f"))
			fontname = argv[++i];
		else if (!strcmp(argv[i], "-ip"))
			inputprefix = argv[++i];
		else if (!strcmp(argv[i], "-is"))
			inputsuffix = argv[++i];
		else if (!strcmp(argv[i], "-p"))
			inputprompt = argv[++i];
		else {
			printf("Unknown option: \"%s\"\n", argv[i]);
			fputs("Usage: <args> | cmenu [-v]\n"
			      "                      [-bg color] [-sbg color]\n"
			      "                      [-t color] [-st color]\n"
			      "                      [-b color] [-bw int]\n"
			      "                      [ -p int]\n"
			      "                      [-f font]\n"
			      "                      [-ip string]\n"
			      "                      [-is string]\n"
			      "                      [-p string]\n", stderr);
			return 1;
		}

	/* read stdin */
	int count;
	char *items[MAXITEMS+1];
	count = read_stdin(items+1);
	++count;

	/* setup */
	int pointerx, pointery;

	struct XValues xv;
	struct WinValues wv;
	struct XftValues xftv;

	if (init_x(&xv) != 0)
		return 1;

	/* setup window values */
	get_pointer(&xv, &pointerx, &pointery);
	wv.xwc.x = pointerx;
	wv.xwc.y = pointery;

	XColor borderpixel;
	XParseColor(xv.display, xv.colormap, wincolors[bordercolor],
	            &borderpixel);
	XAllocColor(xv.display, xv.colormap, &borderpixel);

	XSetWindowAttributes wa;
	wa.override_redirect = True;
	wa.background_pixel = WhitePixel(xv.display, xv.screen_num);
	wa.border_pixel = borderpixel.pixel;
	wa.event_mask = ExposureMask | KeyPressMask;

	unsigned long valuemask = CWOverrideRedirect | CWBackPixel |
	                          CWEventMask | CWBorderPixel;

	/* create the window */
	wv.window = XCreateWindow(xv.display,
	                          RootWindow(xv.display, xv.screen_num),
	                          wv.xwc.x, wv.xwc.y, 1, 1, borderwidth,
	                          CopyFromParent, CopyFromParent, xv.visual,
	                          valuemask, &wa);

	if (init_xft(&xv, &wv, &xftv) != 0)
		return 1;

	grab_keyboard(&xv);
	XGrabPointer(xv.display, wv.window, False, 0, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

	Cursor c;
	c = XCreateFontCursor(xv.display, XC_question_arrow);
	XDefineCursor(xv.display, wv.window, c);

	menu_run(&xv, &wv, &xftv, items, count);

	while (count--)
		free(items[count]);

	XWarpPointer(xv.display, None, xv.root, 0, 0, 0, 0, pointerx, pointery);

	terminate_xft(&xv, &xftv);
	terminate_x(&xv, &wv);
	return 0;
}

unsigned read_stdin(char **lines)
{
	FILE *f;
	f = stdin;

	int count;
	char buf[BUFSIZE];
	for (count = 0; fgets(buf, BUFSIZE, f); ++count) {
		buf[strlen(buf)-1] = '\0'; /* remove newline */
		*lines = malloc((sizeof(buf) + 1) * sizeof(char));
		strcpy(*lines, buf);
		++lines;
	}
	return count;
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

	for (int i = 0; i < 150; ++i) {
		if (XGrabKeyboard(xv->display,
		    RootWindow(xv->display, xv->screen_num),
		    True, GrabModeAsync, GrabModeAsync, CurrentTime)
		    == GrabSuccess)
			return 0;
		nanosleep(&interval, NULL);
	}

	fputs("Could not grab keyboard", stderr);
	return 1;
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
	xv->screen_width = DisplayWidth(xv->display, xv->screen_num);
	xv->screen_height = DisplayHeight(xv->display, xv->screen_num);
	xv->visual = DefaultVisual(xv->display, xv->screen_num);
	xv->colormap = DefaultColormap(xv->display, xv->screen_num);

	return 0;
}

void terminate_x(struct XValues *xv, struct WinValues *wv)
{
	XDestroyWindow(xv->display, wv->window);
	XCloseDisplay(xv->display);
}

int init_xft(struct XValues *xv, struct WinValues *wv, struct XftValues *xftv)
{
	xftv->fontname = fontname;
	xftv->font = XftFontOpenName(xv->display, xv->screen_num, fontname);
	if (!xftv->font) {
		fprintf(stderr, "Could not open font \"%s\"\n", fontname);
		return 1;
	}

	for (int index = textcolor; index <= sbgcolor; ++index)
		if (!XftColorAllocName(xv->display, xv->visual, xv->colormap,
				       wincolors[index],
		                       &xftv->colors[index])) {
			fprintf(stderr, "Could not allocate color \"%s\"\n",
				wincolors[index]);
			return 1;
		}

	xftv->draw = XftDrawCreate(xv->display, wv->window, xv->visual,
	                           xv->colormap);
	return 0;
}

void terminate_xft(struct XValues *xv, struct XftValues *xftv)
{
	XftFontClose(xv->display, xftv->font);
	for (int index = 0; index <= sbgcolor; ++index)
		XftColorFree(xv->display, xv->visual, xv->colormap,
			     &xftv->colors[index]);
	XftDrawDestroy(xftv->draw);
}

void menu_run(struct XValues *xv, struct WinValues *wv, struct XftValues *xftv,
              char *items[], int count)
{
	/* set items[0] to user input */
	*items = malloc(BUFSIZE * sizeof(char));
	**items = '\0';

	char *filtered[count];
	char **filteredp;

	/* filtered[0] is user input + decorations */
	*filtered = malloc(INPUTLEN * sizeof(char));

	XSelectInput(xv->display, wv->window, ExposureMask | KeyPressMask);

	XEvent e;
	KeySym keysym;
	int status;

	status = 0;
	for (;;) {
		XMapWindow(xv->display, wv->window);
		XNextEvent(xv->display, &e);

		switch(e.type) {
		case KeyPress:
			keysym = XkbKeycodeToKeysym(xv->display,
			                            e.xkey.keycode, 0,
			                            (e.xkey.state & ShiftMask)
			                            ? 1 : 0);
			status = handle_key(keysym, e.xkey.state, *items);

			/* program exits here */
			switch(status)
			case EXIT: {
				puts(count > 1 ? filtered[1] : *items);
			case TERM:
				free(*filtered);
				return;
				break;
			}
			break;
		case KeyRelease:
			continue;
			break;
		}

		/* add the prompt, prefix, and suffix */
		snprintf(*filtered, INPUTLEN, "%s%s%s%s",
		         inputprompt, inputprefix, *items, inputsuffix);

		/* filter everything in items through user input*/
		filteredp = filtered+1;
		for (char **itemsp = items+1; *itemsp; ++itemsp)
			if ((strlen(*items) || visible) &&
			    strstr(*itemsp, *items) == *itemsp)
				*filteredp++ = *itemsp;
		count = filteredp - filtered;

		/* status is used to determine the shift amount */
		if (visible)
			draw_menu(xv, wv, xftv, filtered, count, status);
		else
			draw_menu(xv, wv, xftv, filtered,
				  (strlen(*items) > 0) ? count : 1, status);
	}
}

int handle_key(KeySym keysym, int state, char *line)
{
	static char *pos = NULL;
	if (pos == NULL)
		pos = line;

	if (state & ControlMask)
		switch(keysym) {
		case 'p':
			return -1;
			break;
		case 'n':
			return 1;
			break;
		case'h':
			if (pos - line > 0)
				*--pos = '\0';
			return 0;
			break;
		}

	switch(keysym) {
	case XK_Escape:
		return TERM;
		break;
	case XK_Return:
		return EXIT;
		break;
	case XK_Super_L:
	case XK_Control_L:
	case XK_Control_R:
	case XK_Shift_R:
	case XK_Shift_L:
		break;
	case XK_Up:
		return -1;
		break;
	case XK_Down:
		return 1;
		break;
	case XK_BackSpace:
		if (pos - line > 0)
			*--pos = '\0';
		break;
	default:
		if (pos - line < BUFSIZE) {
			*pos++ = keysym;
			*pos = '\0';
		}
		break;
	}
	return 0;
}

void draw_menu(struct XValues *xv, struct WinValues *wv, struct XftValues *xftv,
               char *items[], int count, int shift)
{
	static int offset = 0;
	if (count > 1) {
		offset = (offset + shift) % (count-1);
		for (int i = abs(offset); i > 0; --i)
			rotate_array(items+1, count-1, offset);
	}

	count = move_and_resize(xv, wv, xftv, items, count);

	XftDrawRect(xftv->draw, &xftv->colors[bgcolor], 0, 0, wv->xwc.width,
	            wv->xwc.height);
	draw_items(xftv, items, count);

	if (count > 1)
		draw_selected(xv, wv, xftv, *(items+1));
}

void rotate_array(char **array, int count, int dir)
{
	int i;
	char *tmp;

	if (dir > 0) {
		tmp = *array;
		for (i = 0; i < count-1; ++i)
			array[i] = array[i+1];
		array[i] = tmp;
	} else if (dir < 0) {
		tmp = array[count-1];
		for (i = count-1; i > 0; --i)
			array[i] = array[i-1];
		*array = tmp;
	}
}

int move_and_resize(struct XValues *xv, struct WinValues *wv,
                    struct XftValues *xftv, char *items[], int count)
{
	int total_displayed;
	total_displayed = count;

	wv->xwc.height = xftv->font->height * count + padding * 2;

	XGlyphInfo ext;

	int longest;
	for (longest = 0; count--; ++items) {
		XftTextExtentsUtf8(xv->display, xftv->font, *items,
				   strlen(*items), &ext);

		if (ext.width > longest)
			longest = ext.width;
	}
		
	wv->xwc.width = longest + padding * 2;

	/* We can always count on the top left corner of the window also being
	   the location of the pointer, so (wv->xwc.x, wv->xwc.y), is also the
	   the coordinates of the pointer. The screen height - mouse location
	   is the current (old) height of the window. */
	if (wv->xwc.height > xv->screen_height - wv->xwc.y) {
		if (wv->xwc.height > xv->screen_height) {
			wv->xwc.y = 0;

			/* clip the menu if it extends beyond the screen */
			count = (xv->screen_height - wv->xwc.y) /
			        xftv->font->height;
		} else {
			wv->xwc.y = xv->screen_height - wv->xwc.height;
		}

		XWarpPointer(xv->display, None, xv->root, 0, 0, 0, 0,
			     wv->xwc.x, wv->xwc.y);
	}

	/* X axis */
	if (wv->xwc.width > xv->screen_width - wv->xwc.x) {
		if (wv->xwc.width > xv->screen_width)
			wv->xwc.x = 0;
		else
			wv->xwc.x = xv->screen_width - wv->xwc.width;

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
	int ascent, descent, height;
	ascent = xftv->font->ascent;
	descent = xftv->font->descent;
	height = xftv->font->height;

	int line;
	for (line = ascent + padding; count--; line += height, ++items) {
		XftDrawStringUtf8(xftv->draw, &xftv->colors[textcolor],
		                  xftv->font, padding, line, *items,
		                  strlen(*items));
	}
}

void draw_selected(struct XValues *xv, struct WinValues *wv,
                   struct XftValues *xftv, char *line)
{
	int y, lheight;

	y = xftv->font->ascent + xftv->font->descent + padding;
	lheight = xftv->font->ascent + xftv->font->height + padding;

	XftDrawRect(xftv->draw, &xftv->colors[sbgcolor], 0, y, wv->xwc.width,
	            y - padding);

	XftDrawStringUtf8(xftv->draw, &xftv->colors[stextcolor], xftv->font,
	                  padding, lheight, line, strlen(line));
}
