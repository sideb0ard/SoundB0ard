#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "defjams.h"
#include "cmdinterpreter.h"
#include "mixer.h"

extern mixer *mixr;

void cmd_loopy(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf(ANSI_COLOR_MAGENTA "SB#> " ANSI_COLOR_RESET);
        line = cmd_read_line();
        args = cmd_split_line(line);
        status = cmd_execute(args);

        free (line);
        free (args);
    } while (status);
}

#define CMD_RL_BUFSIZE 1024
char *cmd_read_line(void)
{
    int bufsize = CMD_RL_BUFSIZE;
    int position = 0;
    char *buffer = calloc(1, sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "cmd: allocation errrrror\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();
        if ( c == '\n' ) {
            buffer[position] = '\0';
            return buffer;
        } else if ( c == EOF ) {
            printf("\nBeat it, ya val jerk...\n");
            exit(EXIT_SUCCESS);
        } else {
            buffer[position] = c;
        }
        position++;

        if ( position >= bufsize ) {
            bufsize += CMD_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "cmd:  realloc errrror!\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define CMD_TOK_BUFSIZE 64
#define CMD_TOK_DELIM " \t\r\n\a"
char **cmd_split_line(char *line)
{
    int bufsize = CMD_TOK_BUFSIZE, position = 0;
    char **tokens = calloc(1, bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "cmd: cmd_split_line alloc error!\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, CMD_TOK_DELIM);
    while ( token != NULL ) {
        tokens[position] = token;
        position++;

        if ( position >= bufsize ) {
            bufsize += CMD_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if ( !tokens ) {
                fprintf(stderr, "cmd: realloc split line errrr!\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, CMD_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

char *builtin_str[] = {
    "cd",
    "help",
    "ps",
    "osc",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &cmd_cd,
    &cmd_help,
    &cmd_ps,
    &cmd_osc,
    &cmd_exit
};

int cmd_ps(char **args) 
{
  mixer_ps(mixr);
  return 1;
}

int cmd_osc(char **args) 
{
  add_osc(mixr, 440);
  return 1;
}

int cmd_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int cmd_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "cmd: expected argument to \"cd\"\n");
    } else {
        if ( chdir(args[1]) != 0 ) {
            perror("cmd cd");
        }
    }
    return 1;
}

int cmd_help(char **args)
{
    int i;
    printf("Soundb0ard Shell\n");
    printf("Type commands etc.......zzzzz\n");

    for ( i = 0; i < cmd_num_builtins(); i++ ) {
        printf("  %s\n", builtin_str[i]);
    }
    return 1;
}

int cmd_exit(char **args)
{
    printf("\nBeat it, ya val jerk...\n");
    return 0;
}

int cmd_execute(char **args)
{
    int i;

    if ( args[0] == NULL ) 
        return 1;

    for ( i = 0; i < cmd_num_builtins(); i++ )
        if (!strcmp(args[0], builtin_str[i]))
            return (*builtin_func[i]) (args);

    return 1;
}
