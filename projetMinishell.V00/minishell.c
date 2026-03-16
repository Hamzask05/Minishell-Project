#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include "parser.h"

#define MAX_LINE 1024
#define MAX_ARGS 64

void execute_command(int argc, char *argv[])
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execvp(argv[0], argv);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        wait(NULL);
    }
    else
    {
        perror("fork failed");
    }
}

int main()
{
    char line[MAX_LINE];
    char *argv[MAX_ARGS];
    int argc;

    while (1)
    {
        /* prompt */
        printf("minishell > ");
        fflush(stdout);

        /* lecture */
        if (fgets(line, MAX_LINE, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        /* supprimer \n */
        line[strcspn(line, "\n")] = '\0';

        /* parser */
        argc = parse_line(line, argv, MAX_ARGS);

        if (argc == 0)
            continue;

        /* exit */
        if (strcmp(argv[0], "echo") == 0){
            printf(argv[1]);
        }
            break;

        if (strcmp(argv[0], "exit") == 0)
            break;

        /* execution */
        execute_command(argc, argv);
    }

    return 0;

}