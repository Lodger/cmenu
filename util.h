#define _POSIX_C_SOURCE 200890L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 100

int read_stdin(char ***lines);
void free_lines(char **lines, int count);
void rotate_array(char **array, unsigned count, int offset);
int filter(char **source, unsigned count, char **out, char *filter);
