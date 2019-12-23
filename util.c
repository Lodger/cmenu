#include "util.h"

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

	if (*lines == NULL) {
		fprintf(stderr, "read_stdin: couldn't allocate %u bytes\n",
		        allocated);
		return -1;
	}

	while (getline(&linebuf, &size, f) > 0) {
		linebuf[strlen(linebuf)-1] = '\0'; /* remove newline */
		(*lines)[read] = strdup(linebuf);
		if ((*lines)[read] == NULL)
			return -1;
		++read;

		if (read == allocated) {
			allocated *= 2;
			*lines = realloc(*lines, allocated * sizeof(char*));
			if (*lines == NULL) {
				fprintf(stderr, "read_stdin: couldn't realloc %u bytes\n",
					allocated);
				return -1;
			}
		}
	}

	(*lines)[read] = '\0';
	free(linebuf);
	return read;
}

void free_lines(char **lines, int count)
{
	char **lp;

	lp = lines;
	while (count-- && *lines)
		free(*lp++);
	free(lines);
}

int grab_keyboard(Display *display, Window root)
{
	struct timespec interval;
	interval.tv_sec = 0;
	interval.tv_nsec = 1000000;

	for (int i = 0; i < 200; ++i) {
		if (XGrabKeyboard(display, root, True, GrabModeAsync,
				  GrabModeAsync, CurrentTime) == GrabSuccess)
			return 0;
		nanosleep(&interval, NULL);
	}
	fprintf(stderr, "Unable to grab keyboard");
	return 1;
}
