#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include "util.h"
#include "xutil.h"

#define MAXINPUT 100
#define BUFSIZE 100
#define LENINPUT (MAXINPUT +\
                  strlen(inputprefix) +\
                  strlen(inputsuffix) +\
                  strlen(inputprompt))

/* default theme */
char *fontname = "fixed-10:style=bold";

char *inputprompt = "menu";
char *inputprefix = "\xc2\xbb";
char *inputsuffix = "\xc2\xab";

/* top, bottom, left, right */
unsigned padding[] = {0, 0, 0, 0};
unsigned borderwidth = 2;

/* textcolor, stextcolor, bgcolor, sbgcolor, bordercolor */
char *wincolors[] = {"#000000",
                     "#ffffff",
                     "#ffffff",
                     "#000000",
                     "#000000"};

int itemsvisible = 0;

enum handle_key_exits {SHIFTDOWN = -1, SHIFTUP = 1, EXIT, TERM};
enum padding_names {TOP, BOTTOM, LEFT, RIGHT};
enum colors {PRIMARYFG, ACTIVEFG, PRIMARYBG, ACTIVEBG, BORDER};

struct XValues {
	Display *display;
	int screen_num;
	Window root;
	unsigned screen_width;
	unsigned screen_height;
	Visual *visual;
	Colormap colormap;
};

struct XftValues {
	XftFont *font;
	XftDraw *draw;

	XftColor primaryFG;
	XftColor activeFG;
};

struct WinValues {
	Window window;
	XWindowChanges xwc;

	GC primaryGC;
	GC activeGC;
};

unsigned parse_arguments(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-v") ||
		    !strcmp(argv[i], "--visible"))
			itemsvisible = 1;
		else if (!strcmp(argv[i], "-f") ||
			 !strcmp(argv[i], "--font"))
			fontname = argv[++i];
		else if (!strcmp(argv[i], "-fg") ||
			 !strcmp(argv[i], "--foreground"))
			wincolors[PRIMARYFG] = argv[++i];
		else if (!strcmp(argv[i], "-afg") ||
			 !strcmp(argv[i], "--active-foreground"))
			wincolors[ACTIVEFG] = argv[++i];
		else if (!strcmp(argv[i], "-bg") ||
			 !strcmp(argv[i], "--backgroud"))
			wincolors[PRIMARYBG] = argv[++i];
		else if (!strcmp(argv[i], "-abg") ||
			 !strcmp(argv[i], "--active-background"))
			wincolors[ACTIVEBG] = argv[++i];
		else if (!strcmp(argv[i], "-bw") ||
			 !strcmp(argv[i], "--border-width"))
			borderwidth = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-bc") ||
			 !strcmp(argv[i], "--border-color"))
			wincolors[BORDER] = argv[++i];
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
			 !strcmp(argv[i], "--input-prefix"))
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

void highlight_entry(struct XValues *xv, struct WinValues *wv,
                     struct XftValues *xftv, int index, GC bg)
{
	unsigned y;

	y = xftv->font->height + padding[TOP] +
	    ((index - 1) * xftv->font->height);

	XFillRectangle(xv->display, wv->window, bg, 0, y,
		       wv->xwc.width, xftv->font->height);
}

void draw_string(struct XftValues *xftv, char *line, int index, XftColor fg)
{
	unsigned lineheight;

	lineheight = xftv->font->ascent + padding[TOP] +
	             (index * xftv->font->height);

	XftDrawStringUtf8(xftv->draw, &fg, xftv->font, padding[RIGHT],
			  lineheight, (FcChar8 *) line, strlen(line));
}

void draw_items(struct XftValues *xftv, char **items, unsigned count)
{
	int index;
	for (index = 0; index < count; ++index)
		draw_string(xftv, *items++, index, xftv->primaryFG);
}

int move_and_resize(struct XValues *xv, struct WinValues *wv,
                    struct XftValues *xftv, char **items, unsigned count)
{
	int available;
	available = count;

	XGlyphInfo ext;
	unsigned longest;

	longest = 0;
	while (count--) {
		XftTextExtentsUtf8(xv->display, xftv->font, (FcChar8 *) *items,
				   strlen(*items), &ext);
		if (ext.width > longest)
			longest = ext.width;

		++items;
	}

	wv->xwc.width = longest + padding[LEFT] + padding[RIGHT];
	wv->xwc.height = xftv->font->height * available +
	                 padding[TOP] + padding[BOTTOM];

	/* Y axis */
	if (wv->xwc.height + (borderwidth * 2) >
	    xv->screen_height - wv->xwc.y) {
		if (wv->xwc.height + (borderwidth * 2) > xv->screen_height) {
			wv->xwc.y = 0;

			/* clip the menu items */
			available = (xv->screen_height - wv->xwc.y -
			             padding[TOP] - padding[BOTTOM] -
			             (borderwidth* 2)) /
			            xftv->font->height;

			wv->xwc.height = xftv->font->height * available +
			                 padding[TOP] + padding[BOTTOM];
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

	return available;
}

void redraw_menu(struct XValues *xv, struct WinValues *wv,
                 struct XftValues *xftv, char **items, int count)
{
	count = move_and_resize(xv, wv, xftv, items, count);

	XClearWindow(xv->display, wv->window);
	draw_items(xftv, items, count);

	if (count > 1) {
		highlight_entry(xv, wv, xftv, 1, wv->activeGC);
		draw_string(xftv, *(items + 1), 1, xftv->activeFG);
	}
}

int item_index_from_coords(struct WinValues *wv, struct XftValues *xftv,
                           int x, int y)
{
	if (x < 0 || y < 0)
		return -1;

	if (x > wv->xwc.x + wv->xwc.width)
		return -1;

	return floor(y / xftv->font->height);
}

unsigned handle_motion(struct XValues *xv, struct WinValues *wv,
                       struct XftValues *xftv, char **items, unsigned count,
                       int x, int y)
{
	static unsigned current = 0;
	static unsigned prev = 0;

	current = item_index_from_coords(wv, xftv, x, y);

	if (current > 0 && current < count && current != prev) {
		highlight_entry(xv, wv, xftv, prev, wv->primaryGC);
		draw_string(xftv, items[prev], prev, xftv->primaryFG);
		highlight_entry(xv, wv, xftv, current, wv->activeGC);
		draw_string(xftv, items[current], current, xftv->activeFG);

		prev = current;
	}

	return current;
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
	case XK_Alt_L:     case XK_Alt_R:
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
	int action, offset, subcount;
	unsigned hover; /* mouse selection */

	action = 0; /* do nothing */
	hover = 0;
	offset = subcount = 0;
	for (;;) {
		XMapWindow(xv->display, wv->window);
		XNextEvent(xv->display, &e);

		switch(e.type) {
		case Expose:
			break;
		case KeyPress:
			action = handle_key(xv, e.xkey, input);
			break;
		case MotionNotify:
			hover = handle_motion(xv, wv, xftv, filtered, subcount,
			                      e.xbutton.x, e.xbutton.y);
			continue;
		case ButtonPress:
			if (hover > 0 && hover <= subcount) {
				puts(filtered[hover]);
				action = TERM;
			}
			break;
		default:
			continue;
		}

		/* update the contents */
		if (itemsvisible && !strlen(input))
			subcount = filter_input(items, count, filtered+1,
			                        NULL) + 1;
		else
			subcount = filter_input(items, count, filtered+1,
			                        input) + 1;

		/* shift the contents */
		if (action == SHIFTDOWN || action == SHIFTUP)
			offset += action;

		if (subcount > 1)
			rotate_array(filtered+1, subcount-1, offset);

		/* exit (maybe) */
		switch(action) {
		case EXIT:
			puts(subcount > 1 ? filtered[1] : input);
		case TERM:
			free(*filtered);
			return;
  		}

		/* redraw the menu */
		snprintf(*filtered, LENINPUT, "%s%s%s%s",
		         inputprompt, inputprefix, input, inputsuffix);

		redraw_menu(xv, wv, xftv, filtered, subcount);
	}
}

void terminate_xft(struct XValues *xv, struct XftValues *xftv)
{
	XftFontClose(xv->display, xftv->font);

	XftColorFree(xv->display, xv->visual, xv->colormap,
	             &xftv->primaryFG);
	XftColorFree(xv->display, xv->visual, xv->colormap,
	             &xftv->activeFG);

	XftDrawDestroy(xftv->draw);
}


void terminate_x(struct XValues *xv, struct WinValues *wv)
{
	XFree(wv->primaryGC);
	XFree(wv->activeGC);
	XDestroyWindow(xv->display, wv->window);
	XCloseDisplay(xv->display);
}

int init_xft(struct XValues *xv, struct WinValues *wv, struct XftValues *xftv)
{
	xftv->font = XftFontOpenName(xv->display, xv->screen_num, fontname);
	if (!xftv->font) {
		fprintf(stderr, "Could not open font \"%s\"\n", fontname);
		return 1;
	}
	if (!XftColorAllocName(xv->display, xv->visual, xv->colormap,
	                       wincolors[PRIMARYFG],
	                       &xftv->primaryFG)) {
		fprintf(stderr, "Could not allocate color \"%s\"\n",
			wincolors[PRIMARYFG]);
		return 2;
	}
	if (!XftColorAllocName(xv->display, xv->visual, xv->colormap,
	                       wincolors[ACTIVEFG],
	                       &xftv->activeFG)) {
		fprintf(stderr, "Could not allocate color \"%s\"\n",
			wincolors[ACTIVEFG]);
		return 2;
	}
	xftv->draw = XftDrawCreate(xv->display, wv->window, xv->visual,
	                           xv->colormap);
	return 0;
}

int init_window(struct XValues *xv, struct WinValues *wv)
{
	XColor borderpixel;
	if (parse_and_allocate_xcolor(xv->display, xv->colormap,
	                              wincolors[BORDER], &borderpixel) != 0) {
		return 1;
	}

	XColor background;
	if (parse_and_allocate_xcolor(xv->display, xv->colormap,
	                              wincolors[PRIMARYBG], &background) != 0) {
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

	get_pointer_location(xv->display, xv->root, &wv->xwc.x, &wv->xwc.y);

	wv->window = XCreateWindow(xv->display, xv->root, wv->xwc.x, wv->xwc.y,
	                           1, 1, borderwidth, CopyFromParent,
	                           CopyFromParent, xv->visual, valuemask, &wa);

	/* while we're here, create the graphics contexts */
	XGCValues xgcv;
	xgcv.foreground = background.pixel;
	wv->primaryGC = XCreateGC(xv->display, wv->window, GCForeground, &xgcv);
	XSetBackground(xv->display, wv->primaryGC, background.pixel);

	if (parse_and_allocate_xcolor(xv->display, xv->colormap,
	                              wincolors[ACTIVEBG], &background) != 0) {
		return 1;
	}
	xgcv.foreground = background.pixel;
	wv->activeGC = XCreateGC(xv->display, wv->window, GCForeground, &xgcv);

	return 0;
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

int main(int argc, char *argv[])
{
	if (argc > 1 && parse_arguments(argc, argv) != 0)
		return 1;

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
		return 3;

	if (init_xft(&xv, &wv, &xftv) != 0)
		return 4;

	grab_keyboard(xv.display, xv.root);

	Cursor c;
	c = XCreateFontCursor(xv.display, XC_question_arrow);
	XDefineCursor(xv.display, wv.window, c);

	int px, py;
	get_pointer_location(xv.display, xv.root, &px, &py);

	menu_run(&xv, &wv, &xftv, items, count);

	XWarpPointer(xv.display, None, xv.root, 0, 0, 0, 0, px, py);

	free_lines(items, count);
	terminate_xft(&xv, &xftv);
	terminate_x(&xv, &wv);
	return 0;
}
