#define MAXINPUT 100
#define MAXLINE  30
#define INPUTLEN (MAXINPUT +\
                  strlen(inputprefix) +\
                  strlen(inputsuffix) +\
                  strlen(inputprompt))
#define EXIT 2

#define textcolor 0
#define stextcolor 1
#define sbgcolor 2

struct XValues {
	Display *display;
	int screen_num;
	Visual *visual;
	Colormap colormap;
};

struct XftValues {
	char *fontname;
	XftFont *font;
	XftDraw *draw;

	/* text color, selected text color, selected background color */
	XftColor colors[3];
};

struct WinValues {
	Window window;
	unsigned width;
	unsigned height;
	XWindowChanges xwc;
};

/* X function prototypes */
int init_x(struct XValues *xv);
void terminate_x(struct XValues *xv, struct WinValues *wv);

void get_pointer(struct XValues *xv, int *x, int *y);


/* Xft function prototypes */
int init_xft(struct XValues *xv, struct WinValues *wv,
             struct XftValues *xftv);
void terminate_xft(struct XValues *xv, struct XftValues *xftv);


/* menu function prototypes */
void menu_run(struct XValues *xv, struct WinValues *wv,
              struct XftValues *xftv, char *items[], int count);

int handle_key(KeySym keysym, int state, char *line);

void draw_menu(struct XValues *xv, struct WinValues *wv,
               struct XftValues *xftv, char *items[], int count, int shift);

void draw_selected(struct XValues *xv, struct WinValues *wv,
                      struct XftValues *xftv, char *line);

void move_and_resize(struct XValues *xv, struct WinValues *wv,
                     struct XftValues *xftv, char *items[], int count);

void draw_items(struct XftValues *xftv, char *items[], int count);

void rotate_array(char **array, int count, int dir);
