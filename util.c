#include "util.h"

/* Read in and store arbitrary number of lines from stdin. Null terminated.
 * Returns the number of lines read.
 */
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
	if ((*lines = malloc(allocated * sizeof(char*))) == NULL) {
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

	(*lines)[read] = NULL;
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

int filter(char **in, unsigned count, char **out, char *filter)
{
	unsigned fcount;

	fcount = 0;
	for (; count--; ++in)
		if (strstr(*in, filter) == *in) {
			*out++ = *in;
			++fcount;
		}

	return fcount;
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
