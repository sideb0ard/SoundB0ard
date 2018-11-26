#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/history.h>
#include <readline/readline.h>

#include <cmdloop.h>
#include <mixer.h>
#include <obliquestrategies.h>
#include <utils.h>

#include <algo_cmds.h>
#include <fx_cmds.h>
#include <looper_cmds.h>
#include <midi_cmds.h>
#include <mixer_cmds.h>
#include <new_item_cmds.h>
#include <pattern_generator_cmds.h>
#include <stepper_cmds.h>
#include <synth_cmds.h>
#include <value_generator_cmds.h>

extern mixer *mixr;
extern char *key_names[NUM_KEYS];

extern wtable *wave_tables[5];

#define READLINE_SAFE_MAGENTA "\001\x1b[35m\002"
#define READLINE_SAFE_RESET "\001\x1b[0m\002"
#define MAXLINE 128
char const *prompt = READLINE_SAFE_MAGENTA "SB#> " READLINE_SAFE_RESET;

void *loopy(void *arg)
{
    static char last_line[MAXLINE] = {};

    read_history(NULL);
    setlocale(LC_ALL, "");

    print_logo();

    char *line;
    while ((line = readline(prompt)) != NULL)
    {
        if (strlen(line) != 0)
        {
            if (strncmp(last_line, line, MAXLINE) != 0)
            {
                add_history(line);
                strncpy(last_line, line, MAXLINE);
            }

            interpret(line);
        }
        free(line);
    }
    write_history(NULL);
    exxit();

    return NULL;
}

static bool _is_meta_cmd(char *line)
{
    if (strncmp("every", line, 5) == 0 || strncmp("over", line, 4) == 0 ||
        strncmp("for", line, 3) == 0)
        return true;

    return false;
}

void interpret(char *line)
{
    char wurds[NUM_WURDS][SIZE_OF_WURD] = {};

    if (_is_meta_cmd(line))
    {
        int num_wurds = parse_wurds_from_cmd(wurds, line);
        algorithm *a = new_algorithm(num_wurds, wurds);
        if (a)
            mixer_add_algorithm(mixr, a);
    }

    char *cmd, *last_s;
    char const *sep = ";";
    char tmp[1024] = {};
    for (cmd = strtok_r(line, sep, &last_s); cmd;
         cmd = strtok_r(NULL, sep, &last_s))
    {
        strncpy((char *)tmp, cmd, 127);
        int num_wurds = parse_wurds_from_cmd(wurds, tmp);

        //////////////////////////////////////////////////////////////////////

        if (strncmp("help", wurds[0], 4) == 0)
            oblique_strategy();

        else if (strncmp("quit", wurds[0], 4) == 0 ||
                 strncmp("exit", wurds[0], 4) == 0)
            exxit();

        else if (strncmp("print", wurds[0], 5) == 0)
            printf("%s\n", wurds[1]);

        else if (parse_mixer_cmd(num_wurds, wurds))
            continue;

        else if (parse_algo_cmd(num_wurds, wurds))
            continue;

        else if (parse_fx_cmd(num_wurds, wurds))
            continue;

        else if (parse_looper_cmd(num_wurds, wurds))
            continue;

        else if (parse_midi_cmd(num_wurds, wurds))
            continue;

        else if (parse_new_item_cmd(num_wurds, wurds))
            continue;

        else if (parse_pattern_generator_cmd(num_wurds, wurds))
            continue;

        else if (parse_value_generator_cmd(num_wurds, wurds))
            continue;

        else if (parse_synth_cmd(num_wurds, wurds))
            continue;

        else if (parse_stepper_cmd(num_wurds, wurds))
            continue;
    }
}

int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line)
{
    memset(wurds, 0, NUM_WURDS * SIZE_OF_WURD);
    int num_wurds = 0;
    char const *sep = " ";
    char *tok, *last_s;
    for (tok = strtok_r(line, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s))
    {
        strncpy(wurds[num_wurds++], tok, SIZE_OF_WURD);
        if (num_wurds == NUM_WURDS)
            break;
    }
    return num_wurds;
}

int exxit()
{
    printf(COOL_COLOR_PINK
           "\nBeat it, ya val jerk!\n" ANSI_COLOR_RESET); // Thrashin' reference
    pa_teardown();
    exit(0);
}
