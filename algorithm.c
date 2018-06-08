#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algorithm.h"
#include "cmdloop.h"
#include "mixer.h"
#include "utils.h"
#include <looper.h>

extern mixer *mixr;

const char *s_event_type[] = {"midi", "32nd", "16th", "8th", "4th", "bar"};
const char *s_process_type[] = {"every", "over"};
const char *s_var_select_type[] = {"rand", "osc", "step"};
const char *s_env_type[] = {"LIST", "STEP"};

algorithm *new_algorithm(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    algorithm *a = (algorithm *)calloc(1, sizeof(algorithm));

    if (!extract_cmds_from_line(a, num_wurds, wurds))
    {
        printf("Couldn't parse commands from line\n");
        free(a);
        return NULL;
    }

    a->active = true;
    a->process_step_counter = 0;
    a->debug = false;

    return a;
}

static void handle_command(algorithm *a)
{
    if (a->process_step_counter % a->process_step == 0)
    {
        algorithm_replace_vars_in_cmd(a);
        interpret(a->runnable_command);
    }

    a->process_step_counter++;
}

void algorithm_event_notify(void *self, unsigned int event_type)
{
    algorithm *a = (algorithm *)self;

    if (!a->active)
        return;

    if (event_type == a->event_type)
        handle_command(a);
}

static bool extract_list_elements(algorithm *a, char *list)
{
    bool result = true;
    char const *sep = " ";
    char *tok, *last_tok;
    for (tok = strtok_r(list, sep, &last_tok); tok;
         tok = strtok_r(NULL, sep, &last_tok))
    {
        strncpy(a->env.variable_list_vals[a->env.variable_list_len++], tok,
                MAX_VAR_VAL_LEN);
        if (a->env.variable_list_len >= MAX_LIST_ITEMS)
        {
            result = false;
            break;
        }
    }
    return result;
}

int algorithm_get_var_select_type_from_string(char *wurd)
{
    int var_select_type = -1;
    if (strncmp(wurd, "rand", 4) == 0)
        var_select_type = VAR_RAND;
    else if (strncmp(wurd, "osc", 3) == 0)
        var_select_type = VAR_OSC;
    else
        var_select_type = VAR_STEP; // default

    return var_select_type;
}

static bool extract_and_validate_environment(algorithm *a, char *line)
{
    bool result = true;

    strncpy(a->input_line, line, MAX_CMD_LEN - 1);

    printf("LINE! %s\n", line);
    regmatch_t env_match_group[4];
    regex_t env_rgx;
    regcomp(&env_rgx, "^[[:space:]]*([[:alnum:]]*)*[[:space:]]*\"(.*)\"(.*)",
            REG_EXTENDED | REG_ICASE);
    if (regexec(&env_rgx, line, 4, env_match_group, 0) == 0)
    {
        a->has_env = true;
        int var_select_len =
            env_match_group[1].rm_eo - env_match_group[1].rm_so;
        if (var_select_len == 0)
        {
            a->var_select_type = VAR_STEP;
        }
        else
        {
            char var_select_type[var_select_len + 1];
            var_select_type[var_select_len] = '\0';
            strncpy(var_select_type, line + env_match_group[1].rm_so,
                    var_select_len);

            a->var_select_type =
                algorithm_get_var_select_type_from_string(var_select_type);
        }
        printf("VAR PROC TYPE:%s\n", s_var_select_type[a->var_select_type]);

        int var_list_len = env_match_group[2].rm_eo - env_match_group[2].rm_so;
        char var_list[var_list_len + 1];
        var_list[var_list_len] = '\0';
        strncpy(var_list, line + env_match_group[2].rm_so, var_list_len);
        strncpy(a->env.variable_list_string, var_list, MAX_STATIC_STRING_SZ);
        printf("VARKLISTSTRNG is %s or list? %s\n", a->env.variable_list_string,
               var_list);

        int cmd_len = env_match_group[3].rm_eo - env_match_group[3].rm_so;
        if (cmd_len <= MAX_CMD_LEN)
            strncpy(a->command, line + env_match_group[3].rm_so, cmd_len);
        else
            result = false;

        printf("CMD: %s\n", a->command);

        if (!extract_list_elements(a, var_list))
            result = false;
    }
    else
    {
        a->has_env = false;
        printf("Nae environment!\n");
        printf("CMD: %s\n", line);
        if (strlen(line) <= MAX_CMD_LEN)
            strcpy(a->command, line);
        else
            result = false;
    }

    regfree(&env_rgx);
    return result;
}

void print_algo_help()
{
    printf("Usage: <every|over> n <bar|4th|8th|16th|32nd> [(<step=\"x y "
           "z..\"|rand=\"x y z..\"|osc=\"lo hi\">)] process \%%s\n");
}

int algorithm_get_event_type_from_string(char *wurd)
{
    int event_type = -1;

    if (strncmp(wurd, "loop", 4) == 0 || strncmp(wurd, "bar", 3) == 0)
        event_type = TIME_START_OF_LOOP_TICK;
    else if (strncmp(wurd, "midi", 4) == 0)
        event_type = TIME_MIDI_TICK;
    else if (strncmp(wurd, "4th", 3) == 0 || strncmp(wurd, "quart", 5) == 0)
        event_type = TIME_QUARTER_TICK;
    else if (strncmp(wurd, "8th", 4) == 0)
        event_type = TIME_EIGHTH_TICK;
    else if (strncmp(wurd, "16th", 4) == 0)
        event_type = TIME_SIXTEENTH_TICK;
    else if (strncmp(wurd, "32nd", 4) == 0)
        event_type = TIME_THIRTYSECOND_TICK;

    return event_type;
}
bool extract_cmds_from_line(algorithm *a, int num_wurds,
                            char wurds[][SIZE_OF_WURD])
{

    if (strncmp(wurds[0], "every", 5) == 0 || strncmp(wurds[0], "over", 4) == 0)
    {
        a->process_type = EVERY;
    }
    else
    {
        printf("Need a process type - 'every' or 'over'\n");
        return false;
    }

    int step = atoi(wurds[1]);
    if (step == 0)
    {
        printf("don't be daft, cannae dae 0 times.\n");
        return false;
    }
    a->process_step = step;

    a->event_type = algorithm_get_event_type_from_string(wurds[2]);

    if (a->event_type == -1)
    {
        printf("Need a time period\n");
        return false;
    }

    char line[MAX_CMD_LEN];
    memset(line, 0, MAX_CMD_LEN);

    int len_of_cmd_string = 1; // keep space for null terminator
    for (int i = 3; i < num_wurds; i++)
        len_of_cmd_string += strlen(wurds[i]) + 1;
    if (len_of_cmd_string < MAX_CMD_LEN)
    {
        for (int i = 3; i < num_wurds; i++)
        {
            strcat(line, wurds[i]);
            if (i != (num_wurds - 1))
                strcat(line, " ");
        }
    }
    // printf("CMD! %s\n", line);
    if (!extract_and_validate_environment(a, line))
        return false;

    return true;
}

void algorithm_replace_vars_in_cmd(algorithm *a)
{
    char replacement_value[MAX_VAR_VAL_LEN] = {};

    if (a->var_select_type == VAR_STEP)
    {
        strncpy(replacement_value,
                a->env.variable_list_vals[a->env.variable_list_idx++],
                MAX_VAR_VAL_LEN - 1);
        if (a->env.variable_list_idx >= a->env.variable_list_len)
            a->env.variable_list_idx = 0;
        // printf("CUR VAL! %s\n", cur_var_value);
    }
    else if (a->var_select_type == VAR_RAND)
    {
        strncpy(replacement_value,
                a->env.variable_list_vals[rand() % a->env.variable_list_len],
                MAX_VAR_VAL_LEN - 1);
    }

    memset(a->runnable_command, 0, MAX_CMD_LEN);

    char orig_cmd[MAX_CMD_LEN] = {};
    strcpy(orig_cmd, a->command);

    char *toke, *last_toke;
    char const *sep = " ";
    for (toke = strtok_r(orig_cmd, sep, &last_toke); toke;
         toke = strtok_r(NULL, sep, &last_toke))
    {

        if (strncmp("%s", toke, MAX_VAR_KEY_LEN) == 0)
            strcat(a->runnable_command, replacement_value);
        else
            strcat(a->runnable_command, toke);

        strcat(a->runnable_command, " ");
    }
}

void algorithm_status(void *self, wchar_t *status_string)
{
    algorithm *a = (algorithm *)self;
    char *ALGO_COLOR = ANSI_COLOR_RESET;
    if (a->active)
        ALGO_COLOR = COOL_COLOR_PINK;

    swprintf(status_string, MAX_STATIC_STRING_SZ,
             WANSI_COLOR_WHITE "%sprocess:" WANSI_COLOR_WHITE "%s"
                               "%s step:" WANSI_COLOR_WHITE "%d"
                               "%s event:" WANSI_COLOR_WHITE "%s"
                               "%s var_select:" WANSI_COLOR_WHITE "%s"
                               "%s var_list:" WANSI_COLOR_WHITE "%s\n"
                               "%s         cmd:" WANSI_COLOR_WHITE "%s",
             ALGO_COLOR, s_process_type[a->process_type], ALGO_COLOR,
             a->process_step, ALGO_COLOR, s_event_type[a->event_type],
             ALGO_COLOR, s_var_select_type[a->var_select_type], ALGO_COLOR,
             a->env.variable_list_string, ALGO_COLOR, a->command);
    wcscat(status_string, WANSI_COLOR_RESET);
}

void algorithm_start(algorithm *a) { a->active = true; }
void algorithm_stop(algorithm *a) { a->active = false; }
