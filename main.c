#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void shloopy(void);
char *sbsh_read_line(void);
char **sbsh_split_line(char *line);
int sbsh_execute(char **args);

int main(int argc, char **argv)
{
    shloopy();
    return 0;
}

void shloopy(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("SB#> ");
        line = sbsh_read_line();
        args = sbsh_split_line(line);
        status = sbsh_execute(args);

        free (line);
        free (args);
    } while (status);
}

#define SBSH_RL_BUFSIZE 1024
char *sbsh_read_line(void)
{
    int bufsize = SBSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "sbsh: allocation errrrror\n");
        exit(-1);
    }

    while (1) {
        c = getchar();
        if ( c == EOF || c == '\n' ) {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if ( position >= bufsize ) {
            bufsize += SBSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "sbsh:  realloc errrror!\n");
                exit(-1);
            }
        }
    }
}

#define SBSH_TOK_BUFSIZE 64
#define SBSH_TOK_DELIM " \t\r\n\a"
char **sbsh_split_line(char *line)
{
    int bufsize = SBSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "sbsh: sbsh_split_line alloc error!\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, SBSH_TOK_DELIM);
    while ( token != NULL ) {
        tokens[position] = token;
        position++;

        if ( position >= bufsize ) {
            bufsize += SBSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if ( !tokens ) {
                fprintf(stderr, "sbsh: realloc split line errrr!\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, SBSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int sbsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if ( pid == 0 ) { // child
        if ( execvp(args[0], args) == -1 )
            perror("sbsh");
        exit(EXIT_FAILURE);
    } else if ( pid  < 0 ) {
        perror("forkn failure");
    } else { // Parental unit
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

// function declarations for shell builtins

int sbsh_cd(char **args);
int sbsh_help(char **args);
int sbsh_exit(char **args);

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &sbsh_cd,
    &sbsh_help,
    &sbsh_exit
};

int sbsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int sbsh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "sbsh: expected argument to \"cd\"\n");
    } else {
        if ( chdir(args[1]) != 0 ) {
            perror("sbsh cd");
        }
    }
    return 1;
}

int sbsh_help(char **args)
{
    int i;
    printf("Thor Sideburn's SBSH\n");
    printf("(Ripped off from Stephen Brennan's LSH)\n");
    printf("Type commands etc.......zzzzz\n");

    for ( i = 0; i < sbsh_num_builtins(); i++ ) {
        printf("  %s\n", builtin_str[i]);
    }
    return 1;
}

int sbsh_exit(char **args)
{
    return 0;
}

int sbsh_execute(char **args)
{
    int i;

    if ( args[0] == NULL ) 
        return 1;

    for ( i = 0; i < sbsh_num_builtins(); i++ ) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i]) (args);
        }
    }
    return sbsh_launch(args);
}
