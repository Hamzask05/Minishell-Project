#include <string.h>
#include "parser.h"

int parse_line(char *line, char *argv[], int max_args)
{
    int argc = 0;

    char *token = strtok(line, " ");

    while (token != NULL && argc < max_args - 1)
    {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    argv[argc] = NULL;

    return argc;
}