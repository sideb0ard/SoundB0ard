#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "defjams.h"
#include "cmdinterpreter.h"
#include "mixer.h"

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

#define cmd_RL_BUFSIZE 1024
char *cmd_read_line(void)
{
    int bufsize = cmd_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
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
            printf("\nLater, ya val jerk...\n");
            exit(EXIT_SUCCESS);
        } else {
            buffer[position] = c;
        }
        position++;

        if ( position >= bufsize ) {
            bufsize += cmd_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "cmd:  realloc errrror!\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define cmd_TOK_BUFSIZE 64
#define cmd_TOK_DELIM " \t\r\n\a"
char **cmd_split_line(char *line)
{
    int bufsize = cmd_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "cmd: cmd_split_line alloc error!\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, cmd_TOK_DELIM);
    while ( token != NULL ) {
        tokens[position] = token;
        position++;

        if ( position >= bufsize ) {
            bufsize += cmd_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if ( !tokens ) {
                fprintf(stderr, "cmd: realloc split line errrr!\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, cmd_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

// int cmd_launch(char **args)
// {
//     pid_t pid, wpid;
//     int status;
// 
//     pid = fork();
//     if ( pid == 0 ) { // child
//         if ( execvp(args[0], args) == -1 )
//             perror("cmd");
//         exit(EXIT_FAILURE);
//     } else if ( pid  < 0 ) {
//         perror("forkn failure");
//     } else { // Parental unit
//         do {
//             wpid = waitpid(pid, &status, WUNTRACED);
//         } while (!WIFEXITED(status) && !WIFSIGNALED(status));
//     }
// 
//     return 1;
// }

char *builtin_str[] = {
    "cd",
    "help",
    "mixer",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &cmd_cd,
    &cmd_help,
    &cmd_newmixer,
    &cmd_exit
};

int cmd_newmixer(char **args)
{
  mixer *mixr = new_mixer();
  if (mixr == NULL) {
    printf("Oh, mixer is NULL :(\n");
    return -1;
  } 
  printf("mixer is YAY!\n");
  pthread_t mixer_thread;
  if (pthread_create(&mixer_thread, NULL, &mixer_run, NULL)) {
    printf("Barfed i think!\n");
    return -1;
  }
  printf("All good!\n");
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
    printf("Thor Sideburn's cmd\n");
    printf("(Ripped off from Stephen Brennan's LSH)\n");
    printf("Type commands etc.......zzzzz\n");

    for ( i = 0; i < cmd_num_builtins(); i++ ) {
        printf("  %s\n", builtin_str[i]);
    }
    return 1;
}

int cmd_exit(char **args)
{
    printf("\nLater, ya val jerk...\n");
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

    //return cmd_launch(args);
    return 1;
}
