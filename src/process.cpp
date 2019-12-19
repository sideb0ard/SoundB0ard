#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "cmdloop.h"
#include "mixer.h"
#include "process.hpp"
#include "utils.h"
#include <looper.h>
#include <tsqueue.hpp>

#include <pattern_parser/tokenizer.hpp>

extern mixer *mixr;

using Wrapper =
    std::pair<std::shared_ptr<ast::Node>, std::shared_ptr<object::Environment>>;
extern Tsqueue<Wrapper> g_queue;

// void Process::Eval()
//{
//    g_queue.push(std::make_pair(body_, env_));
//    step_counter_++;
//}
//

void Process::ParsePattern()
{
    std::cout << "\nPARSE PATTERN STARTS!\n\n";
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern_);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);

    std::shared_ptr<pattern_parser::EventGroup> pattern_event_group =
        pattern_parzer->ParsePattern();

    for (auto e : pattern_event_group->events_)
        std::cout << e->String() << std::endl;
}

Process::Process(std::string target, std::string pattern)
    : target_{target}, pattern_{pattern}, active_{true}
{
    ParsePattern();
}

void Process::EventNotify(mixer_timing_info tinfo)
{
    if (!active_)
        return;

    // if (event_type_ == TIME_MIDI_TICK)
    //    Eval();
    // else if (event_type_ == TIME_START_OF_LOOP_TICK &&
    // tinfo.is_start_of_loop)
    //{
    //    Eval();
    //}
    // else if (event_type_ == TIME_QUARTER_TICK && tinfo.is_quarter)
    //    Eval();
    // else if (event_type_ == TIME_EIGHTH_TICK && tinfo.is_eighth)
    //    Eval();
    // else if (event_type_ == TIME_SIXTEENTH_TICK && tinfo.is_sixteenth)
    //    Eval();
    // else if (event_type_ == TIME_THIRTYSECOND_TICK && tinfo.is_thirtysecond)
    //    Eval();
    //// if (event_type == a->event_type)
    ////{
    ////    if (event_type == SEQUENCER_NOTE &&
    ////        event.sequencer_src != a->sequencer_src)
    ////        return;
    ////    handle_command(a);
    ////}

    // if (timer_type_ == OVER)
    //    Eval();
    // else if (tinfo.is_sixteenth && timer_type_ == FOR)
    //    Eval();
}

// bool algorithm_set_var_list(algorithm *a, char *list)
//{
//    int len = strlen(list);
//    if (list[0] != '"' && list[len - 1] != '"')
//    {
//        printf("Var list needs to be enclosed in double quotes. Sorry,
//        bra\n"); return false;
//    }
//    int new_len = len - 1; // minus both quotes, but plus one for '\0'
//    char quote_stripped_list[new_len];
//    memset(quote_stripped_list, 0, new_len);
//    strncpy(quote_stripped_list, list + 1, len - 2);
//    strncpy(a->env.variable_list_string, quote_stripped_list,
//            MAX_STATIC_STRING_SZ);
//
//    if (strstr(quote_stripped_list, ":"))
//    {
//        float start, end, step = 1;
//        sscanf(quote_stripped_list, "%f:%f:%f", &start, &end, &step);
//
//        a->env.has_sequence = true;
//        a->env.low_seq = start;
//        a->env.cur_seq = start;
//        a->env.hi_seq = end;
//        a->env.seq_step = step;
//    }
//    else
//    {
//        char const *sep = " ";
//        char *tok, *last_tok;
//        a->env.variable_list_len = 0;
//        for (tok = strtok_r(quote_stripped_list, sep, &last_tok); tok;
//             tok = strtok_r(NULL, sep, &last_tok))
//        {
//            strncpy(a->env.variable_list_vals[a->env.variable_list_len++],
//            tok,
//                    MAX_VAR_VAL_LEN);
//            if (a->env.variable_list_len >= MAX_LIST_ITEMS)
//            {
//                return false;
//            }
//        }
//    }
//
//    return true;
//}
//
// int algorithm_get_var_select_type_from_string(char *wurd)
//{
//    int var_select_type = -1;
//    if (strncmp(wurd, "rand", 4) == 0)
//        var_select_type = VAR_RAND;
//    else if (strncmp(wurd, "osc", 3) == 0)
//        var_select_type = VAR_OSC;
//    else
//        var_select_type = VAR_STEP; // default
//
//    return var_select_type;
//}
//
// static bool extract_and_validate_environment(algorithm *a, char *line)
//{
//    bool result = true;
//
//    strncpy(a->input_line, line, MAX_CMD_LEN - 1);
//
//    regmatch_t env_match_group[4];
//    regex_t env_rgx;
//    regcomp(&env_rgx, "^[[:space:]]*([[:alnum:]]*)*[[:space:]]*(\".*\")(.*)",
//            REG_EXTENDED | REG_ICASE);
//    if (regexec(&env_rgx, line, 4, env_match_group, 0) == 0)
//    {
//        a->has_env = true;
//
//        if (a->Process_type == FOR)
//            a->var_select_type = VAR_FOR;
//        else
//        {
//            int var_select_type = VAR_STEP;
//            int var_select_len =
//                env_match_group[1].rm_eo - env_match_group[1].rm_so;
//            if (var_select_len != 0)
//            {
//                char var_select_type_string[var_select_len + 1];
//                var_select_type_string[var_select_len] = '\0';
//                strncpy(var_select_type_string, line +
//                env_match_group[1].rm_so,
//                        var_select_len);
//
//                var_select_type = algorithm_get_var_select_type_from_string(
//                    var_select_type_string);
//            }
//            algorithm_set_var_select_type(a, var_select_type);
//        }
//
//        int var_list_len = env_match_group[2].rm_eo -
//        env_match_group[2].rm_so; char var_list[var_list_len + 1];
//        var_list[var_list_len] = '\0';
//        strncpy(var_list, line + env_match_group[2].rm_so, var_list_len);
//
//        if (!algorithm_set_var_list(a, var_list))
//            result = false;
//
//        if (a->Process_type == FOR)
//        {
//            if (a->env.variable_list_len < 2)
//                return false;
//            a->counter = atoi(a->env.variable_list_vals[0]);
//        }
//
//        int cmd_len = env_match_group[3].rm_eo - env_match_group[3].rm_so;
//        if (cmd_len <= MAX_CMD_LEN)
//        {
//            char cmd[cmd_len + 1];
//            cmd[cmd_len] = '\0';
//            strncpy(cmd, line + env_match_group[3].rm_so, cmd_len);
//            algorithm_set_cmd(a, cmd);
//        }
//        else
//            result = false;
//    }
//    else
//    {
//        a->has_env = false;
//        printf("Nae environment!\n");
//        printf("CMD: %s\n", line);
//        if (strlen(line) <= MAX_CMD_LEN)
//            strcpy(a->command, line);
//        else
//            result = false;
//    }
//
//    regfree(&env_rgx);
//    return result;
//}
//
// void print_algo_help()
//{
//    printf("Usage: <every|over> n <bar|4th|8th|16th|32nd> [(<step=\"x y "
//           "z..\"|rand=\"x y z..\"|osc=\"lo hi\">)] Process \%%s\n");
//}
//
// int algorithm_set_event_type_from_string(algorithm *a, char *wurd)
//{
//    int event_type = -1;
//    int sequencer_src = -1;
//
//    if (strncmp(wurd, "loop", 4) == 0 || strncmp(wurd, "bar", 3) == 0)
//        event_type = TIME_START_OF_LOOP_TICK;
//    else if (strncmp(wurd, "midi", 4) == 0)
//        event_type = TIME_MIDI_TICK;
//    else if (strncmp(wurd, "4th", 3) == 0 || strncmp(wurd, "quart", 5) == 0)
//        event_type = TIME_QUARTER_TICK;
//    else if (strncmp(wurd, "8th", 4) == 0)
//        event_type = TIME_EIGHTH_TICK;
//    else if (strncmp(wurd, "16th", 4) == 0)
//        event_type = TIME_SIXTEENTH_TICK;
//    else if (strncmp(wurd, "32nd", 4) == 0)
//        event_type = TIME_THIRTYSECOND_TICK;
//    else if (strncmp(wurd, "note", 4) == 0)
//    {
//        printf("NOTE! event_type is %d\n", event_type);
//        int sg_num = -1;
//        char tmpp[28] = {};
//        sscanf(wurd, "%[^:]:%d", tmpp, &sg_num);
//        printf("SCANFd %d from %s\n", sg_num, wurd);
//
//        if (mixer_is_valid_soundgen_num(mixr, sg_num))
//        {
//            printf("Woof! following SoundGen %d!\n", sg_num);
//            sequencer_src = sg_num;
//            event_type = SEQUENCER_NOTE;
//        }
//        else
//            printf("Nah man!\n");
//    }
//
//    if (event_type != -1)
//    {
//        a->event_type = event_type;
//        a->sequencer_src = sequencer_src;
//        return 0;
//    }
//    return -1;
//}
//
// bool extract_cmds_from_line(algorithm *a, int num_wurds,
//                            char wurds[][SIZE_OF_WURD])
//{
//
//    if (strncmp(wurds[0], "every", 5) == 0) // discrete event
//        a->Process_type = EVERY;
//    else if (strncmp(wurds[0], "over", 4) == 0) // continous event
//        a->Process_type = OVER;
//    else if (strncmp(wurds[0], "for", 3) == 0) // one-time event
//        a->Process_type = FOR;
//    else
//    {
//        printf("Need a Process type - 'every', 'over' or 'for'\n");
//        return false;
//    }
//
//    if (a->Process_type == FOR)
//    {
//        a->Process_step = 1;
//        a->event_type = TIME_QUARTER_TICK;
//    }
//    else
//    {
//        int step = atoi(wurds[1]);
//        if (step == 0)
//        {
//            printf("don't be daft, cannae dae 0 times.\n");
//            return false;
//        }
//        a->Process_step = step;
//
//        int err = algorithm_set_event_type_from_string(a, wurds[2]);
//        printf("ERR is %d\n", err);
//        if (err == -1)
//        {
//            printf("Need a time period\n");
//            return false;
//        }
//    }
//
//    int rest_of_string_starting_position = 3; // for a OVER or EVERY cmd
//    if (a->Process_type == FOR)
//        rest_of_string_starting_position = 1;
//
//    char line[MAX_CMD_LEN] = {};
//
//    int len_of_cmd_string = 1; // keep space for null terminator
//    for (int i = rest_of_string_starting_position; i < num_wurds; i++)
//        len_of_cmd_string += strlen(wurds[i]) + 1;
//    if (len_of_cmd_string < MAX_CMD_LEN)
//    {
//        for (int i = rest_of_string_starting_position; i < num_wurds; i++)
//        {
//            strcat(line, wurds[i]);
//            if (i != (num_wurds - 1))
//                strcat(line, " ");
//        }
//    }
//    if (!extract_and_validate_environment(a, line))
//        return false;
//
//    return true;
//}
//
// static void _get_command_replacement_value(algorithm *a,
//                                           char *replacement_value)
//{
//    if (a->var_select_type == VAR_STEP)
//    {
//        if (a->env.has_sequence)
//        {
//            sprintf(replacement_value, "%f", a->env.cur_seq);
//            a->env.cur_seq += a->env.seq_step;
//            if (a->env.cur_seq > a->env.hi_seq)
//                a->env.cur_seq = fmod(a->env.cur_seq, a->env.hi_seq) +
//                                 a->env.low_seq -
//                                 1; // minus 1 balances the above 'greater
//                                 than'
//                                    // which is inclusive of hi_seq
//        }
//        else
//        {
//            strncpy(replacement_value,
//                    a->env.variable_list_vals[a->env.variable_list_idx++],
//                    MAX_VAR_VAL_LEN - 1);
//            if (a->env.variable_list_idx >= a->env.variable_list_len)
//                a->env.variable_list_idx = 0;
//        }
//    }
//    else if (a->var_select_type == VAR_RAND)
//    {
//        strncpy(replacement_value,
//                a->env.variable_list_vals[rand() % a->env.variable_list_len],
//                MAX_VAR_VAL_LEN - 1);
//    }
//    else if (a->var_select_type == VAR_FOR)
//    {
//        char tmpval[MAX_VAR_VAL_LEN] = {};
//        itoa(a->counter, tmpval);
//        strncpy(replacement_value, tmpval, MAX_VAR_VAL_LEN - 1);
//        int end = atoi(a->env.variable_list_vals[1]);
//        (a->counter)++;
//        if (a->counter > end)
//            a->active = false;
//    }
//    else if (a->var_select_type == VAR_OSC)
//    {
//        if (a->env.variable_list_len >= 2)
//        {
//            double lo = atof(a->env.variable_list_vals[0]);
//            double hi = atof(a->env.variable_list_vals[1]);
//            if (lo < hi)
//            {
//                int num_ticks_per_cycle =
//                    mixer_get_ticks_per_cycle_unit(mixr, a->event_type) *
//                    a->Process_step;
//                int cur_midi_tick =
//                    mixr->timing_info.midi_tick % num_ticks_per_cycle;
//                double relative_position = 0;
//                int halfway = num_ticks_per_cycle / 2.0;
//                if (cur_midi_tick < halfway)
//                {
//                    relative_position = scaleybum(0, num_ticks_per_cycle, lo,
//                                                  hi, cur_midi_tick * 2);
//                }
//                else
//                {
//                    relative_position =
//                        scaleybum(0, num_ticks_per_cycle, lo, hi,
//                                  (halfway - (cur_midi_tick - halfway)) * 2);
//                }
//                sprintf(replacement_value, "%f", relative_position);
//            }
//            else
//            {
//                printf("Lo and Hi don't look good: %f and %f\n", lo, hi);
//                return;
//            }
//        }
//        else
//        {
//            printf("Need two list values to OSC between\n");
//            return;
//        }
//    }
//    if (a->debug)
//        printf("REPLAcement val:%s\n", replacement_value);
//}
//
// void algorithm_replace_vars_in_cmd(algorithm *a)
//{
//    char replacement_value[MAX_VAR_VAL_LEN] = {};
//    _get_command_replacement_value(a, replacement_value);
//
//    memset(a->runnable_command, 0, MAX_CMD_LEN);
//
//    char orig_cmd[MAX_CMD_LEN] = {};
//    strcpy(orig_cmd, a->command);
//
//    char *toke, *last_toke;
//    char const *sep = " ";
//    for (toke = strtok_r(orig_cmd, sep, &last_toke); toke;
//         toke = strtok_r(NULL, sep, &last_toke))
//    {
//
//        if (strncmp("%s", toke, MAX_VAR_KEY_LEN) == 0)
//            strcat(a->runnable_command, replacement_value);
//        else
//            strcat(a->runnable_command, toke);
//
//        strcat(a->runnable_command, " ");
//    }
//}

void Process::Status(wchar_t *status_string)
{
    const char *PROC_COLOR = ANSI_COLOR_RESET;
    if (active_)
        PROC_COLOR = COOL_COLOR_PINK;

    swprintf(status_string, MAX_STATIC_STRING_SZ,
             WANSI_COLOR_WHITE "%sProcess: Target:%s Pattern:%s Active:%s",
             PROC_COLOR, target_.c_str(), pattern_.c_str(),
             active_ ? "true" : "false");
    wcscat(status_string, WANSI_COLOR_RESET);
}

void Process::Start() { active_ = true; }
void Process::Stop() { active_ = false; }

// void algorithm_set_var_select_type(algorithm *a, unsigned int
// var_select_type)
//{
//    if (var_select_type < MAX_VAR_SELECT_TYPES)
//    {
//        a->var_select_type = var_select_type;
//        if (var_select_type == VAR_OSC)
//            a->Process_type = OVER;
//        else
//            a->Process_type = EVERY;
//    }
//}
// bool algorithm_set_cmd(algorithm *a, char *cmd)
//{
//    printf("SETCMD! giotsz:%s\n", cmd);
//    if (strlen(cmd) <= MAX_CMD_LEN)
//    {
//        strncpy(a->command, cmd, MAX_CMD_LEN);
//        return true;
//    }
//    return false;
//}
//
// void algorithm_append_target(algorithm *a, char *target)
//{
//    printf("APPEND! giotsz:%s\n", target);
//    printf("CURRENT CMD %s\n", a->command);
//    strcat(a->command, " ");
//    strcat(a->command, target);
//    printf("NEW CMD %s\n", a->command);
//}

void Process::SetDebug(bool b) { debug_ = b; }
