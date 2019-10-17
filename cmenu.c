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
			itemsvisible = True;
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
		else if (!strcmp(argv[i], "-pr"))
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
			      "                      [-pr string]\n", stderr);
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

	/* create the window */
	wv.window = XCreateWindow(xv.display,
	                          RootWindow(xv.display, xv.screen_num),
	                          wv.xwc.x, wv.xwc.y, 1, 1, borderwidth,
	                          CopyFromParent, CopyFromParent, xv.visual,
	                          CWOverrideRedirect | CWBackPixel |
	                          CWEventMask | CWBorderPixel, &wa);

	if (init_xft(&xv, &wv, &xftv) != 0)
		return 1;

	grab_keyboard(&xv);
	XGrabPointer(xv.display, wv.window, False, 0, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

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

	/* filtered[0] is user input + decorations */
	*filtered = malloc(INPUTLEN * sizeof(char));

	XSelectInput(xv->display, wv->window, ExposureMask |
	                                      KeyPressMask |
	                                      PointerMotionMask |
	                                      ButtonPressMask);

	XEvent e;
	KeySym keysym;
	int status, selected, location;

	status = 0; /* do nothing */
	selected = 0; /* initially, the mouse has not selected anything */
	for (;;) {
		XMapWindow(xv->display, wv->window);
		XNextEvent(xv->display, &e);

		switch(e.type) {
		case Expose:
			break;
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
		case ButtonPress:
			if (count == 1 || e.xbutton.y < xftv->font->ascent +
			    xftv->font->descent)
				continue;

			selected = (e.xbutton.y - xftv->font->ascent) /
			           xftv->font->height;

			if (selected >= count-1) /* out of bounds */
				break;

			/* mouse mode exits here */
			puts(filtered[selected+1]);
			free(*filtered);
			return;
			break;

		/* mouse mode */
		case MotionNotify:
			location = (e.xbutton.y - xftv->font->ascent - padding);

			if (e.xbutton.y < xftv->font->ascent +
			    xftv->font->descent || count == 1 ||
			    location / xftv->font->height == selected )
				continue;

			count = filter_input(items, *items, filtered+1) + 1;
			rotate_array(filtered+1, count-1, 0);

			/* hide old selection */
			draw_selected(xv, wv, xftv, filtered[selected+1],
			              selected, 1); /* hide old selection */

			selected = location / xftv->font->height;

			if (location < 0 || selected >= count-1) {
				selected = 0;
				continue;
			}

			/* draw new selection */
			draw_selected(xv, wv, xftv, filtered[selected+1],
			              selected, 0);
			continue;
			break;
		case KeyRelease:
			continue;
			break;
		}

		/* add the prompt, prefix, and suffix */
		snprintf(*filtered, INPUTLEN, "%s%s%s%s",
		         inputprompt, inputprefix, *items, inputsuffix);

		count = filter_input(items, *items, filtered+1) + 1;

		if (count > 1)
			rotate_array(filtered+1, count-1, status);

		/* status is used to determine the shift amount */
		if (itemsvisible)
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
	case XK_Control_L: case XK_Control_R:
	case XK_Shift_R: case XK_Shift_L:
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

int filter_input(char **source, char *filter, char **out)
{
	char **sourcep, **outp;
	outp = out;
	for (char **sourcep = source+1; *sourcep; ++sourcep)
		if ((strlen(*source) || itemsvisible) &&
		    strstr(*sourcep, *source) == *sourcep)
			*outp++ = *sourcep;
	return outp - out;
}

void rotate_array(char **array, int count, int shift)
{
	static int offset = 0;
	offset = (offset + shift) % count;
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
	XftDrawRect(xftv->draw, &xftv->colors[bgcolor], 0, 0, wv->xwc.width,
	            wv->xwc.height);
	draw_items(xftv, items, count);

	if (count > 1)
		draw_selected(xv, wv, xftv, items[1], 0, 0);
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

	//Cursor c;
	//c = XCreateFontCursor(xv->display, XC_question_arrow);
	//XDefineCursor(xv->display, wv->window, c);

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
	for (line = ascent + padding; count--; line += height, ++items) {
		XftDrawStringUtf8(xftv->draw, &xftv->colors[textcolor],
		                  xftv->font, padding, line, *items,
		                  strlen(*items));
	}
}

void draw_selected(struct XValues *xv, struct WinValues *wv,
                   struct XftValues *xftv, char *line, int index, short swap)
{
	int y, rheight, lheight;

	y = xftv->font->ascent + xftv->font->descent + padding;
	lheight = xftv->font->ascent + xftv->font->height + padding;
	rheight = y - padding;

	/* calculate offsets */
	y += xftv->font->height * index;
	lheight += xftv->font->height * index;

	XftDrawRect(xftv->draw, &xftv->colors[!swap ? sbgcolor : bgcolor],
	            0, y, wv->xwc.width, rheight);

	XftDrawStringUtf8(xftv->draw,
	                  &xftv->colors[!swap ? stextcolor : textcolor],
	                  xftv->font, padding, lheight, line, strlen(line));
}
