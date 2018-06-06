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
    a->counter = 0;
    a->debug = false;

    return a;
}

static void handle_command(algorithm *a)
{
    if (a->counter % a->step == 0)
    {
        algorithm_replace_vars_in_cmd(a);
        // printf("UPDated cmd: %s\n", a->runnable_command);
        interpret(a->runnable_command);
    }

    a->counter++;
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
        strncpy(a->env.variable_list_vals[a->env.list_len++], tok,
                MAX_VAR_VAL_LEN);
        if (a->env.list_len >= MAX_LIST_ITEMS)
        {
            result = false;
            break;
        }
    }
    return result;
}

static bool extract_env_details(algorithm *a, char *env)
{
    // can either be a list denoted by quotes e.g. "0.2 0.6 0.7"
    // or else steps, similar to for loop, denoted by two ";"
    bool result = true;

    regmatch_t var_list_match[3];
    regex_t list_rgx;
    regcomp(&list_rgx,
            "^[[:space:]]*([[:alnum:]]+)[[:space:]]*=[[:space:]]*\"(.*)\"",
            REG_EXTENDED | REG_ICASE);

    regmatch_t var_step_match[4];
    regex_t step_rgx;
    regcomp(&step_rgx,
            "^[[:space:]]*([[:alnum:]]+)[[:space:]]*=[[:space:]]*([["
            ":alnum:]]+)[[:space:]]*;(.*)\"",
            REG_EXTENDED | REG_ICASE);

    if (regexec(&list_rgx, env, 3, var_list_match, 0) == 0)
    {
        printf("OOh! got a list!\n");
        a->env.type = LIST_TYPE;
        int var_name_len = var_list_match[1].rm_eo - var_list_match[1].rm_so;
        if (var_name_len <= MAX_VAR_KEY_LEN)
        {
            strncpy(a->env.variable_key, env + var_list_match[1].rm_so,
                    var_name_len);
        }
        else
            result = false;
        printf("KEY! %s Type:%d\n", a->env.variable_key, a->env.type);

        int list_len = var_list_match[2].rm_eo - var_list_match[2].rm_so;
        char list[list_len + 1];
        list[list_len] = '\0';
        strncpy(list, env + var_list_match[2].rm_so, list_len);
        printf("LIST: %s\n", list);
        if (!extract_list_elements(a, list))
            result = false;
    }
    else if (regexec(&step_rgx, env, 4, var_step_match, 0) == 0)
    {
        printf("AH! steps!\n");
    }
    return result;
}

static bool extract_and_validate_environment(algorithm *a, char *line)
{
    bool result = true;

    strncpy(a->input_line, line, MAX_CMD_LEN - 1);

    printf("LINE! %s\n", line);
    regmatch_t env_match_group[3];
    regex_t env_rgx;
    regcomp(&env_rgx, "^\\((.*)\\)(.*)", REG_EXTENDED | REG_ICASE);
    if (regexec(&env_rgx, line, 3, env_match_group, 0) == 0)
    {
        a->has_env = true;
        printf("FOUND an ENV!\n");
        int env_len = env_match_group[1].rm_eo - env_match_group[1].rm_so;
        char env[env_len + 1];
        env[env_len] = '\0';
        strncpy(env, line + env_match_group[1].rm_so, env_len);
        printf("ENV: %s\n", env);

        int cmd_len = env_match_group[2].rm_eo - env_match_group[2].rm_so;
        if (cmd_len <= MAX_CMD_LEN)
            strncpy(a->command, line + env_match_group[2].rm_so, cmd_len);
        else
            result = false;
        printf("CMD: %s\n", a->command);

        if (strncmp(env, "scramble", 8) == 0 || strncmp(env, "stutter", 7) == 0)
        {
            printf("SCRA/TUUTMBLER!\n");
            char localwurd[5] = {};
            int looper_num = -1;
            sscanf(a->command, "%s %d", localwurd, &looper_num);
            if (strncmp(localwurd, "loop", 4) == 0 && looper_num != 1)
            {
                a->env.target_soundgen = looper_num;
                if (strncmp(env, "sc", 2) == 0)
                    a->env.type = SCRAMBLER_TYPE;
                else
                    a->env.type = STUTTER_TYPE;
            }
        }
        else if (!extract_env_details(a, env))
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

bool extract_cmds_from_line(algorithm *a, int num_wurds,
                            char wurds[][SIZE_OF_WURD])
{

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
    a->step = step;

    if (strncmp(wurds[2], "loop", 4) == 0 || strncmp(wurds[2], "bar", 3) == 0)
        a->event_type = TIME_START_OF_LOOP_TICK;
    else if (strncmp(wurds[2], "midi", 4) == 0)
        a->event_type = TIME_MIDI_TICK;
    else if (strncmp(wurds[2], "4th", 3) == 0 ||
             strncmp(wurds[2], "quart", 5) == 0)
        a->event_type = TIME_QUARTER_TICK;
    else if (strncmp(wurds[2], "8th", 4) == 0)
        a->event_type = TIME_EIGHTH_TICK;
    else if (strncmp(wurds[2], "16th", 4) == 0)
        a->event_type = TIME_SIXTEENTH_TICK;
    else if (strncmp(wurds[2], "32nd", 4) == 0)
        a->event_type = TIME_THIRTYSECOND_TICK;
    else
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
    if (a->env.type == LIST_TYPE)
    {
        char cur_var_value[MAX_VAR_VAL_LEN] = {};
        strncpy(cur_var_value, a->env.variable_list_vals[a->env.list_idx++],
                MAX_VAR_VAL_LEN - 1);
        if (a->env.list_idx >= a->env.list_len)
            a->env.list_idx = 0;
        // printf("CUR VAL! %s\n", cur_var_value);

        memset(a->runnable_command, 0, MAX_CMD_LEN);

        char orig_cmd[MAX_CMD_LEN] = {};
        strcpy(orig_cmd, a->command);

        char *toke, *last_toke;
        char const *sep = " ";
        for (toke = strtok_r(orig_cmd, sep, &last_toke); toke;
             toke = strtok_r(NULL, sep, &last_toke))
        {

            if (strncmp(a->env.variable_key, toke, MAX_VAR_KEY_LEN) == 0)
                strcat(a->runnable_command, cur_var_value);
            else
                strcat(a->runnable_command, toke);

            strcat(a->runnable_command, " ");
        }
    }
}
void algorithm_status(void *self, wchar_t *status_string)
{
    algorithm *a = (algorithm *)self;
    char *ALGO_COLOR = ANSI_COLOR_RESET;
    if (a->active)
        ALGO_COLOR = COOL_COLOR_PINK;

    swprintf(status_string, MAX_PS_STRING_SZ,
             WANSI_COLOR_WHITE
             "%sprocess:" WANSI_COLOR_WHITE "%s"
             "%s step:" WANSI_COLOR_WHITE "%d"
             "%s event:" WANSI_COLOR_WHITE "%s"
             "%s env:(type:%s var:%s val:%s)\n"
             "        Cmd: %s ListLen:%d (input_line:%s)",
             ALGO_COLOR, s_process_type[a->process_type],
             ALGO_COLOR, a->step,
             ALGO_COLOR, s_event_type[a->event_type],
             ALGO_COLOR, s_env_type[a->env.type],
             a->env.variable_key,
             a->env.type == LIST_TYPE
                 ? a->env.variable_list_vals[a->env.list_idx]
                 : a->env.variable_scalar_value,
             a->command, a->env.list_len, a->input_line);
    wcscat(status_string, WANSI_COLOR_RESET);
}

void algorithm_start(algorithm *a) { a->active = true; }
void algorithm_stop(algorithm *a) { a->active = false; }
