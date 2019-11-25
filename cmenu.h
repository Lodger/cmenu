#define MAXINPUT 100
#define BUFSIZE  100
#define LENINPUT (MAXINPUT +\
                  strlen(inputprefix) +\
                  strlen(inputsuffix) +\
                  strlen(inputprompt))
#define ATTEMPTS 150

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

	XftColor primaryfg;
	XftColor activefg;
};

struct WinValues {
	Window window;
	XWindowChanges xwc;

	GC primaryGC;
	GC activeGC;
};

unsigned parse_arguments(int argc, char **argv);
int read_stdin(char ***lines);

/* X function prototypes */
int init_x(struct XValues *xv);
void terminate_x(struct XValues *xv, struct WinValues *wv);
int grab_keyboard(struct XValues *xv);
int init_window(struct XValues *xv, struct WinValues *wv);
int parse_and_allocate_xcolor(struct XValues *xv, char *name, XColor *color);

/* Xft function prototypes */
int init_xft(struct XValues *xv, struct WinValues *wv,
             struct XftValues *xftv);
void terminate_xft(struct XValues *xv, struct XftValues *xftv);

/* cmenu function prototypes */
void menu_run(struct XValues *xv, struct WinValues *wv,
              struct XftValues *xftv, char *items[], int count);
int handle_key(struct XValues *xv, XKeyEvent xkey, char *inputline);
void redraw_menu(struct XValues *xv, struct WinValues *wv,
                 struct XftValues *xftv, char **items, int count);
int move_and_resize(struct XValues *xv, struct WinValues *wv,
                    struct XftValues *xftv, char **items, unsigned count);
void draw_items(struct XftValues *xftv, char **items, unsigned count);
void draw_string(struct XftValues *xftv, char *line, int index, XftColor fg);
int get_index_from_coords(struct WinValues *wv, struct XftValues *xftv, unsigned x,
                          unsigned y);

/* utility functions */
unsigned filter_input(char **source, unsigned count, char **out, char *filter);
void get_pointer(struct XValues *xv, int *x, int *y);
void rotate_array(char **array, unsigned count, int dir);
void highlight_entry(struct XValues *xv, struct WinValues *wv,
                     struct XftValues *xftv, int index, GC bg);
