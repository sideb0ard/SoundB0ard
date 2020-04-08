#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include <readline/history.h>
#include <readline/readline.h>

#include <iostream>

#include <algo_cmds.h>
#include <cmdloop.h>
#include <midi_cmds.h>
#include <mixer.h>
#include <mixer_cmds.h>
#include <new_item_cmds.h>
#include <obliquestrategies.h>
#include <stepper_cmds.h>
#include <synth_cmds.h>
#include <tsqueue.hpp>
#include <utils.h>
#include <value_generator_cmds.h>

extern mixer *mixr;
extern Tsqueue<std::string> g_command_queue;
extern Tsqueue<std::string> g_reply_queue;

extern char *key_names[NUM_KEYS];

extern wtable *wave_tables[5];

#define READLINE_SAFE_MAGENTA "\001\x1b[35m\002"
#define READLINE_SAFE_RESET "\001\x1b[0m\002"
#define MAXLINE 128

char const *prompt = READLINE_SAFE_MAGENTA "SB#> " READLINE_SAFE_RESET;
static char last_line[MAXLINE] = {};
static bool active{true};

int event_hook()
{
    while (auto reply = g_reply_queue.try_pop())
    {
        if (reply)
        {
            std::cout << reply->data();
            rl_line_buffer[0] = '\0';
            rl_done = 1;
        }
    }
    return 0;
}

void *loopy()
{
    std::cout << get_string_logo();
    read_history(NULL);
    setlocale(LC_ALL, "");

    char *line;
    rl_event_hook = event_hook;
    rl_set_keyboard_input_timeout(500);
    while ((line = readline(prompt)) != NULL && active)
    {
        if (line && *line)
        {
            if (strncmp(last_line, line, MAXLINE) != 0)
            {
                add_history(line);
                strncpy(last_line, line, MAXLINE);
            }
            g_command_queue.push(line);
        }
        free(line);
    }
    exxit();

    return NULL;
}

int exxit()
{
    printf(COOL_COLOR_PINK
           "\nBeat it, ya val jerk!\n" ANSI_COLOR_RESET); // Thrashin' reference
    write_history(NULL);

    pa_teardown();

    active = false;

    //      return 0;
    exit(0);
}
