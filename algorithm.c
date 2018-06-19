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
    if (((a->process_type == EVERY) &&
         (a->process_step_counter % a->process_step == 0)) ||
        a->process_type == OVER)
    {
        if (a->has_env)
        {
            algorithm_replace_vars_in_cmd(a);
            interpret(a->runnable_command);
        }
        else
            interpret(a->command);
    }

    a->process_step_counter++;
}

void algorithm_event_notify(void *self, unsigned int event_type)
{
    algorithm *a = (algorithm *)self;

    if (!a->active)
        return;

    if (event_type == a->event_type ||
        (event_type == TIME_MIDI_TICK && a->process_type == OVER))
        handle_command(a);
}

bool algorithm_set_var_list(algorithm *a, char *list)
{
    int len = strlen(list);
    if (list[0] != '"' && list[len - 1] != '"')
    {
        printf("Var list needs to be enclosed in double quotes. Sorry, bra\n");
        return false;
    }
    int new_len = len - 1; // minus both quotes, but plus one for '\0'
    char quote_stripped_list[new_len];
    memset(quote_stripped_list, 0, new_len);
    strncpy(quote_stripped_list, list + 1, len - 2);

    strncpy(a->env.variable_list_string, quote_stripped_list,
            MAX_STATIC_STRING_SZ);
    char const *sep = " ";
    char *tok, *last_tok;
    a->env.variable_list_len = 0;
    for (tok = strtok_r(quote_stripped_list, sep, &last_tok); tok;
         tok = strtok_r(NULL, sep, &last_tok))
    {
        strncpy(a->env.variable_list_vals[a->env.variable_list_len++], tok,
                MAX_VAR_VAL_LEN);
        if (a->env.variable_list_len >= MAX_LIST_ITEMS)
        {
            return false;
        }
    }

    return true;
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
    regcomp(&env_rgx, "^[[:space:]]*([[:alnum:]]*)*[[:space:]]*(\".*\")(.*)",
            REG_EXTENDED | REG_ICASE);
    if (regexec(&env_rgx, line, 4, env_match_group, 0) == 0)
    {
        int var_select_type = VAR_STEP;
        a->has_env = true;
        int var_select_len =
            env_match_group[1].rm_eo - env_match_group[1].rm_so;
        if (var_select_len != 0)
        {
            char var_select_type_string[var_select_len + 1];
            var_select_type_string[var_select_len] = '\0';
            strncpy(var_select_type_string, line + env_match_group[1].rm_so,
                    var_select_len);

            var_select_type = algorithm_get_var_select_type_from_string(
                var_select_type_string);
        }
        algorithm_set_var_select_type(a, var_select_type);

        int var_list_len = env_match_group[2].rm_eo - env_match_group[2].rm_so;
        char var_list[var_list_len + 1];
        var_list[var_list_len] = '\0';
        strncpy(var_list, line + env_match_group[2].rm_so, var_list_len);

        if (!algorithm_set_var_list(a, var_list))
            result = false;

        int cmd_len = env_match_group[3].rm_eo - env_match_group[3].rm_so;
        if (cmd_len <= MAX_CMD_LEN)
        {
            char cmd[cmd_len + 1];
            cmd[cmd_len] = '\0';
            strncpy(cmd, line + env_match_group[3].rm_so, cmd_len);
            algorithm_set_cmd(a, cmd);
        }
        else
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

    //for (int i = 0; i < num_wurds; i++)
    //    printf("WURD:%s\n", wurds[i]);

    if (strncmp(wurds[0], "every", 5) == 0)
    {
        a->process_type = EVERY;
    }
    else if (strncmp(wurds[0], "over", 4) == 0)
    {
        a->process_type = OVER;
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
    else if (a->var_select_type == VAR_OSC)
    {
        if (a->env.variable_list_len > 2)
        {
            double lo = atof(a->env.variable_list_vals[0]);
            double hi = atof(a->env.variable_list_vals[1]);
            if (lo < hi)
            {
                int num_ticks_per_cycle =
                    mixer_get_ticks_per_cycle_unit(mixr, a->event_type) *
                    a->process_step;
                int cur_midi_tick =
                    mixr->timing_info.midi_tick % num_ticks_per_cycle;
                double relative_position = 0;
                int halfway = num_ticks_per_cycle / 2.0;
                if (cur_midi_tick < halfway)
                {
                    relative_position = scaleybum(0, num_ticks_per_cycle, lo,
                                                  hi, cur_midi_tick * 2);
                }
                else
                {
                    relative_position =
                        scaleybum(0, num_ticks_per_cycle, lo, hi,
                                  (halfway - (cur_midi_tick - halfway)) * 2);
                }

                itoa(relative_position, replacement_value);
            }
            else
            {
                printf("Lo and Hi don't look good: %f and %f\n", lo, hi);
                return;
            }
        }
        else
        {
            printf("Need two list values to OSC between\n");
            return;
        }
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

void algorithm_set_var_select_type(algorithm *a, unsigned int var_select_type)
{
    if (var_select_type < MAX_VAR_SELECT_TYPES)
    {
        a->var_select_type = var_select_type;
        if (var_select_type == VAR_OSC)
            a->process_type = OVER;
        else
            a->process_type = EVERY;
    }
}
bool algorithm_set_cmd(algorithm *a, char *cmd)
{
    printf("SETCMD! giotsz:%s\n", cmd);
    if (strlen(cmd) <= MAX_CMD_LEN)
    {
        strncpy(a->command, cmd, MAX_CMD_LEN);
        return true;
    }
    return false;
}
