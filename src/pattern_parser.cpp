#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/resource.h>
#include <sys/time.h>

#include <cmdloop.h>
#include <defjams.h>
#include <drumsampler.h>
#include <euclidean.h>
#include <midimaaan.h>
#include <mixer.h>
#include <pattern_parser.h>
#include <utils.h>

#define WEE_STACK_SIZE 32
#define _MAX_VAR_NAME 32
#define MAX_TOKENS 512
#define MAX_DIVISIONS 10
#define MAX_GENERATED_PATTERNS 16

extern mixer *mixr;

// algo originally inspired from
// https://www.geeksforgeeks.org/check-for-balanced-parentheses-in-an-expression/

typedef struct wee_stack
{
    unsigned int stack[WEE_STACK_SIZE];
    int idx;
} wee_stack;

typedef struct division_marker
{
    int position;
    int value;
} division_marker;

// static char *token_type_names[] = {(char *)"SQUARE_LEFT",
//                                   (char *)"SQUARE_RIGHT",
//                                   (char *)"CURLY_LEFT",
//                                   (char *)"CURLY_RIGHT",
//                                   (char *)"ANGLE_BRACKET_LEFT",
//                                   (char *)"ANGLE_BRACKET_RIGHT",
//                                   (char *)"ANGLE_EXPRESSION",
//                                   (char *)"BLANK",
//                                   (char *)"VAR_NAME"};

bool generate_pattern_from_tokens(pattern_token tokens[MAX_PATTERN],
                                  int num_tokens, midi_event *pattern,
                                  unsigned int pattern_type)
{

    bool return_val = true;

    pattern_group *pgroups =
        (pattern_group *)calloc(MAX_PATTERN, sizeof(pattern_group));
    int current_pattern_group = 0;
    int num_pattern_groups = 0;

    // var_tokens are the content tokens, where something should happen
    // as opposed to the tokens to be be expanded, like brackets
    pattern_token *var_tokens =
        (pattern_token *)calloc(MAX_TOKENS, sizeof(pattern_token));
    int var_tokens_idx = 0;

    for (int i = 0; i < num_tokens; i++)
    {
        if (tokens[i].type == SQUARE_BRACKET_LEFT)
        {
            pgroups[++num_pattern_groups].parent = current_pattern_group;
            int cur_child = pgroups[current_pattern_group].num_children++;
            pgroups[current_pattern_group].children[cur_child].level_idx =
                num_pattern_groups;
            pgroups[current_pattern_group].children[cur_child].new_level = true;
            current_pattern_group = num_pattern_groups;
        }
        else if (tokens[i].type == SQUARE_BRACKET_RIGHT)
            current_pattern_group = pgroups[current_pattern_group].parent;
        else if (tokens[i].type == BLANK || tokens[i].type == VAR_NAME ||
                 tokens[i].type == ANGLE_EXPRESSION)
        {
            pgroups[current_pattern_group].num_children++;
            var_tokens[var_tokens_idx++] = tokens[i];
        }
    }

    int level = 0;
    int start_idx = 0;
    int ppositions[MAX_PATTERN] = {};
    int numpositions = 0;
    work_out_positions(pgroups, level, start_idx, PPBAR, ppositions,
                       &numpositions);
    free(pgroups);

    // validation ..
    int num_uniq = 0;
    int uniq_positions[MAX_PATTERN] = {};
    for (int i = 0; i < numpositions; i++)
        if (!is_int_member_in_array(ppositions[i], uniq_positions, num_uniq))
            uniq_positions[num_uniq++] = ppositions[i];

    if (num_uniq != var_tokens_idx)
    {
        printf("Vars and timings don't match, ya numpty: num_uniq:%d "
               "var_tokens:%d\n",
               num_uniq, var_tokens_idx);
        return_val = false;
        goto cleanup_and_return;
    }

    // print_pattern_tokens(var_tokens, var_tokens_idx);
    // printf("New algorithm!\n");
    // for (int i = 0; i < num_uniq; i++)
    //    printf("pos:[%d] %s\n", uniq_positions[i], var_tokens[i].value);

    for (int i = 0; i < num_uniq; i++)
    {
        if (var_tokens[i].type == VAR_NAME)
        {
            pattern[uniq_positions[i]].event_type = MIDI_ON;
            pattern[uniq_positions[i]].data2 = DEFAULT_VELOCITY;
            if (pattern_type == MIDI_PATTERN)
                pattern[uniq_positions[i]].data1 = atoi(var_tokens[i].value);
            else if (pattern_type == NOTE_PATTERN)
            {
                int midi_num = get_midi_note_from_string(var_tokens[i].value);
                if (midi_num != -1)
                    pattern[uniq_positions[i]].data1 = midi_num;
            }
        }
    }

cleanup_and_return:
    free(var_tokens);
    return return_val;
}

static bool is_valid_token_char(char c)
{
    if (isalnum(c) || c == '/' || c == '*' || c == '(' || c == ')' ||
        c == ',' || c == '<' || c == '>' || c == '{' || c == '}' || c == '~' ||
        c == '_' || c == '-' || c == '!' || c == '#')
        return true;
    return false;
}

static bool is_valid_pattern_char(char c)
{
    if (is_valid_token_char(c) || c == '[' || c == ']' || c == ',' || c == ' ')
        return true;
    return false;
}
static bool is_start_of_modifier(char *c)
{
    if (*c && (*c == '(' || *c == '*' || *c == '/'))
        return true;
    return false;
}

static bool is_valid_token_name(char *token_name)
{
    // pretty weak, just checks if starts with alnum or bang
    // followed by optional expander
    regex_t valid_token_rx;
    regcomp(&valid_token_rx, "^[[:alnum:]!]+[\\*/(]?",
            REG_EXTENDED | REG_ICASE);
    bool ret = false;
    if (regexec(&valid_token_rx, token_name, 0, NULL, 0) == 0)
        ret = true;
    regfree(&valid_token_rx);

    return ret;
}
static int grep_expander(char *wurd, pattern_token *token)
{
    // looks for multiplier or divisor e.g. bd*2 or [bd bd]/3
    regmatch_t asterisk_group[4];
    regex_t asterisk_rgx;
    regcomp(&asterisk_rgx, "([][:alnum:]#]+)([\\*/])([[:digit:]]+)",
            REG_EXTENDED | REG_ICASE);

    // looks for euclidean/bjorklund expander e.g. bd(3,8)
    regmatch_t euclid_group[4];
    regex_t euclid_rgx;
    regcomp(&euclid_rgx, "([][:alnum:]]+)\\(([[:digit:]]+),([[:digit:]]+)\\)",
            REG_EXTENDED | REG_ICASE);

    int num_to_increment_by = 0; // return value, used to increment string
    if (regexec(&asterisk_rgx, wurd, 4, asterisk_group, 0) == 0)
    {
        int var_name_len = asterisk_group[1].rm_eo - asterisk_group[1].rm_so;
        char var_name[var_name_len + 1];
        var_name[var_name_len] = '\0';
        strncpy(var_name, wurd + asterisk_group[1].rm_so, var_name_len);

        int op_len = asterisk_group[2].rm_eo - asterisk_group[2].rm_so;
        char var_op[op_len + 1];
        var_op[op_len] = '\0';
        strncpy(var_op, wurd + asterisk_group[2].rm_so, op_len);

        int multi_len = asterisk_group[3].rm_eo - asterisk_group[3].rm_so;
        char var_mod[multi_len + 1];
        var_mod[multi_len] = '\0';
        strncpy(var_mod, wurd + asterisk_group[3].rm_so, multi_len);
        int modifier = atoi(var_mod);

        if (var_op[0] == '*')
        {
            token->has_multiplier = true;
            token->multiplier = modifier;
        }
        else if (var_op[0] == '/')
        {
            token->has_divider = true;
            token->divider = modifier;
        }
        strncpy(token->value, var_name, MAX_PATTERN_CHAR_VAL - 1);

        num_to_increment_by = op_len + multi_len;
    }
    else if (regexec(&euclid_rgx, wurd, 4, euclid_group, 0) == 0)
    {
        int var_name_len = euclid_group[1].rm_eo - euclid_group[1].rm_so;
        char var_name[var_name_len + 1];
        var_name[var_name_len] = '\0';
        strncpy(var_name, wurd + euclid_group[1].rm_so, var_name_len);

        int euclid_hit_len = euclid_group[2].rm_eo - euclid_group[2].rm_so;
        char euc_hit[euclid_hit_len + 1];
        euc_hit[euclid_hit_len] = '\0';
        strncpy(euc_hit, wurd + euclid_group[2].rm_so, euclid_hit_len);
        int ehit = atoi(euc_hit);

        int euclid_step_len = euclid_group[3].rm_eo - euclid_group[3].rm_so;
        char euc_step[euclid_step_len + 1];
        euc_step[euclid_step_len] = '\0';
        strncpy(euc_step, wurd + euclid_group[3].rm_so, euclid_step_len);
        int estep = atoi(euc_step);

        token->has_euclid = true;
        token->euclid_hits = ehit;
        token->euclid_steps = estep;
        strncpy(token->value, var_name, MAX_PATTERN_CHAR_VAL - 1);

        num_to_increment_by =
            euclid_hit_len + euclid_step_len + 3; //'(', ',' and ')'
    }

    regfree(&asterisk_rgx);
    regfree(&euclid_rgx);

    return num_to_increment_by;
}

static int extract_tokens_from_line(pattern_token *tokens, int *token_idx,
                                    char *line)
{

    char *c = line;
    while (*c)
    {
        if (*c == ' ')
        {
            c++;
        }
        else if (*c == '[')
        {
            tokens[(*token_idx)++].type = SQUARE_BRACKET_LEFT;
            c++;
        }
        else if (*c == ']')
        {
            tokens[*token_idx].type = SQUARE_BRACKET_RIGHT;
            if (is_start_of_modifier(c + 1))
            {
                int inc = grep_expander(c, &tokens[*token_idx]);
                c += inc;
            }
            (*token_idx)++;
            c++;
        }
        else if (*c == '{')
        {
            tokens[(*token_idx)++].type = CURLY_BRACKET_LEFT;
            c++;
        }
        else if (*c == '}')
        {
            tokens[(*token_idx)++].type = CURLY_BRACKET_RIGHT;
            if (is_start_of_modifier(c + 1))
            {
                int inc = grep_expander(c, &tokens[*token_idx]);
                c += inc;
            }

            c++;
        }
        else if (*c == '<')
        {
            char angle_contents[MAX_PATTERN_CHAR_VAL];
            int ac_idx = 0;

            c++; // skip the '<'

            tokens[*token_idx].type = ANGLE_EXPRESSION;
            while ((*c) != '>')
            {
                angle_contents[ac_idx++] = *c;
                c++;
            }
            c++; // skip the '>'

            printf("Captured an ANGLE_EXPRESSION! %s\n", angle_contents);
            strncpy(tokens[*token_idx].value, angle_contents,
                    MAX_PATTERN_CHAR_VAL - 1);
            char *wurd, *last_wurd;
            char const *sep = " ";
            int var_count = 0;
            for (wurd = strtok_r(angle_contents, sep, &last_wurd); wurd;
                 wurd = strtok_r(NULL, sep, &last_wurd))
            {
                printf("STROKin %s\n", wurd);
                strncpy((char *)tokens[*token_idx].steps[var_count], wurd, 4);
                var_count++;
                if (var_count > 4)
                {
                    printf("ma wee stack overfloweth!\n");
                    return 1;
                }
            }
            tokens[*token_idx].num_steps = var_count;
            printf("NUM STEPS Set to %d\n", tokens[*token_idx].num_steps);
            (*token_idx)++;
        }
        else if ((*c == '_') || (*c == '-') || (*c == '~'))
        {
            tokens[(*token_idx)++].type = BLANK;
            c++;
        }
        else
        {
            char var_name[_MAX_VAR_NAME] = {};
            int var_name_idx = 0;
            while (is_valid_token_char(*c))
                var_name[var_name_idx++] = *c++;
            // printf("VAR_NAME:%s\n", var_name);
            if (is_valid_token_name(var_name))
            {
                tokens[(*token_idx)].type = VAR_NAME;
                strncpy(tokens[*token_idx].value, var_name,
                        MAX_PATTERN_CHAR_VAL);
                // printf("IN HERE! val is %s\n", tokens[*token_idx].value);
                grep_expander(var_name, &tokens[*token_idx]);
                (*token_idx)++;
            }
            else
            {
                printf("As far as i can tell, %s ain't no valid name, cuz\n",
                       var_name);
                return 1;
            }
        }
    }

    return 0;
}

void work_out_positions(pattern_group pgroups[MAX_PATTERN], int level,
                        int start_idx, int pattern_len,
                        int ppositions[MAX_PATTERN], int *numpositions)
{

    int num_children = pgroups[level].num_children;
    // printf("Looking at Level:%d start_idx:%d pattern_len:%d
    // num_children:%d\n",
    //       level, start_idx, pattern_len, num_children);
    if (num_children != 0)
    {
        double pattern_ratio = 16 / (pattern_len / PPSIXTEENTH);
        uint16_t euclid_pattern = create_euclidean_rhythm(num_children, 16);

        int posz[num_children];
        int posz_idx = 0;
        for (int i = 0; i < 16; i++)
        {
            int rel_position = 1 << (15 - i);
            if (rel_position & euclid_pattern)
            {
                posz[posz_idx++] =
                    ((i / pattern_ratio) * PPSIXTEENTH) + start_idx;
                // printf("i:%d pattern_ratio:%f posz[i]:%d\n", i,
                // pattern_ratio,
                //       posz[posz_idx]);
            }
        }
        int child_pattern_len = pattern_len;
        for (int i = 0; i < num_children; i++)
        {
            int chidx = posz[i];
            int child = pgroups[level].children[i].level_idx;
            ppositions[(*numpositions)++] = chidx;
            // printf("CHILD:%d chidx:%d numpositions:%d\n", child, chidx,
            //       *numpositions);
            if (pgroups[level].children[i].new_level)
            {
                // printf("New level!\n");
                if ((i + 1) < num_children)
                    child_pattern_len = posz[i + 1] - posz[i];
                else
                    child_pattern_len = (pattern_len + start_idx) - posz[i];
                work_out_positions(pgroups, child, chidx, child_pattern_len,
                                   ppositions, numpositions);
                // printf("POPPED BACK from New level!\n");
            }
        }
    }
}

static bool matchy_matchy(unsigned int left, char *right)
{
    if (left == SQUARE_BRACKET_LEFT && *right == ']')
        return true;
    else if (left == CURLY_BRACKET_LEFT && *right == '}')
        return true;
    else if (left == ANGLE_BRACKET_LEFT && *right == '>')
        return true;
    return false;
}

static bool wee_stack_push(wee_stack *s, char *c)
{
    if (s->idx < WEE_STACK_SIZE)
    {
        unsigned int type = -1;
        switch (*c)
        {
        case ('['):
            type = SQUARE_BRACKET_LEFT;
            break;
        case ('{'):
            type = CURLY_BRACKET_LEFT;
            break;
        case ('<'):
            type = ANGLE_BRACKET_LEFT;
            break;
        }
        s->stack[s->idx] = type;
        ++(s->idx);
        return true;
    }
    else
        printf("stack is full!\n");
    return false;
}

static bool wee_stack_pop(wee_stack *s, unsigned int *ret)
{
    if (s->idx > 0)
    {
        --(s->idx);
        *ret = s->stack[s->idx];
        return true;
    }
    else
        printf("stack is empty!\n");
    return false;
}

bool is_valid_pattern(char *line)
{
    // checks that chars are in valid chars,
    // and that parens are balanced

    wee_stack parens_stack = {};

    char *c = line;
    while (*c)
    {
        if (!is_valid_pattern_char(*c))
        {
            printf("BARF! '%c' is not a valid pattern char\n", *c);
            return false;
        }
        else if (*c == '[' || *c == '{' || *c == '<')
        {
            if (!wee_stack_push(&parens_stack, c))
                return false;
        }
        else if (*c == ']' || *c == '}' || *c == '>')
        {
            unsigned int matchy = 0;
            if (!wee_stack_pop(&parens_stack, &matchy))
                return false;

            if (!matchy_matchy(matchy, c))
                return false;
        }
        c++;
    }

    if (parens_stack.idx == 0)
        return true;

    return false;
}

static void copy_pattern_token(pattern_token *to, pattern_token *from)
{
    to->type = from->type;
    strncpy(to->value, from->value, MAX_PATTERN_CHAR_VAL - 1);
}

static void expand_the_expanders(pattern_token tokens[MAX_PATTERN], int len,
                                 pattern_token expanded_tokens[MAX_PATTERN],
                                 int *expanded_tokens_idx)
{
    for (int i = 0; i < len; i++)
    {
        if (tokens[i].type == VAR_NAME)
        {
            if (tokens[i].has_multiplier)
            {
                expanded_tokens[(*expanded_tokens_idx)++].type =
                    SQUARE_BRACKET_LEFT;
                for (int j = 0; j < tokens[i].multiplier; j++)
                    copy_pattern_token(
                        &expanded_tokens[(*expanded_tokens_idx)++], &tokens[i]);
                expanded_tokens[(*expanded_tokens_idx)++].type =
                    SQUARE_BRACKET_RIGHT;
            }
            else if (tokens[i].has_euclid)
            {
                if (tokens[i].euclid_steps > 64)
                {
                    printf("Meh, ain't tested this beyond 64 steps - ignoring "
                           "yer request\n");
                    continue;
                }

                expanded_tokens[(*expanded_tokens_idx)++].type =
                    SQUARE_BRACKET_LEFT;

                short euclid = create_euclidean_rhythm(tokens[i].euclid_hits,
                                                       tokens[i].euclid_steps);
                for (int j = 0; j < tokens[i].euclid_steps; j++)
                {
                    if (euclid & (1 << (15 - j)))
                        copy_pattern_token(
                            &expanded_tokens[(*expanded_tokens_idx)++],
                            &tokens[i]);
                    else
                        expanded_tokens[(*expanded_tokens_idx)++].type = BLANK;
                }
                expanded_tokens[(*expanded_tokens_idx)++].type =
                    SQUARE_BRACKET_RIGHT;
            }
            else if (tokens[i].has_divider)
            {
                copy_pattern_token(&expanded_tokens[*expanded_tokens_idx],
                                   &tokens[i]);
                expanded_tokens[*expanded_tokens_idx].has_divider = true;
                expanded_tokens[*expanded_tokens_idx].divider =
                    tokens[i].divider;
                (*expanded_tokens_idx)++;
            }
            else
                copy_pattern_token(&expanded_tokens[(*expanded_tokens_idx)++],
                                   &tokens[i]);
        }
        else if (tokens[i].type == SQUARE_BRACKET_RIGHT)
        {
            if (tokens[i].has_multiplier)
            {
                int multi = tokens[i].multiplier;

                copy_pattern_token(&expanded_tokens[(*expanded_tokens_idx)++],
                                   &tokens[i]);

                int num_right_brackets_seen = 0;
                bool found = false;
                // (*expanded_tokens_idx) points to one after SQ RIGHT so..
                int idx_one_position_before_cur_square_right =
                    (*expanded_tokens_idx) - 2;
                for (int rev_idx = idx_one_position_before_cur_square_right;
                     rev_idx >= 0 && !found; rev_idx--)
                {
                    if (expanded_tokens[rev_idx].type == SQUARE_BRACKET_LEFT)
                    {
                        if (num_right_brackets_seen > 0)
                            num_right_brackets_seen--;
                        else
                        {
                            found = true;
                            int num_items =
                                idx_one_position_before_cur_square_right -
                                rev_idx;
                            pattern_token temp_tokens[num_items];

                            for (int j = rev_idx + 1, k = 0;
                                 j <= idx_one_position_before_cur_square_right;
                                 j++, k++)
                            {
                                copy_pattern_token(&temp_tokens[k],
                                                   &expanded_tokens[j]);
                            }
                            (*expanded_tokens_idx) = rev_idx + 1;
                            for (int j = 0; j < multi; j++)
                            {
                                for (int k = 0; k < num_items; k++)
                                {
                                    copy_pattern_token(
                                        &expanded_tokens[(
                                            *expanded_tokens_idx)++],
                                        &temp_tokens[k]);
                                }
                            }
                            expanded_tokens[(*expanded_tokens_idx)++].type =
                                SQUARE_BRACKET_RIGHT;
                        }
                    }
                    else if (expanded_tokens[rev_idx].type ==
                             SQUARE_BRACKET_RIGHT)
                    {
                        num_right_brackets_seen++;
                    }
                }
            }
            else if (tokens[i].has_divider)
            {
                copy_pattern_token(&expanded_tokens[*expanded_tokens_idx],
                                   &tokens[i]);
                expanded_tokens[*expanded_tokens_idx].has_divider = true;
                expanded_tokens[*expanded_tokens_idx].divider =
                    tokens[i].divider;

                (*expanded_tokens_idx)++;
            }
            else
            {
                copy_pattern_token(&expanded_tokens[(*expanded_tokens_idx)++],
                                   &tokens[i]);
            }
        }
        else if (tokens[i].type == ANGLE_EXPRESSION)
        {
            copy_pattern_token(&expanded_tokens[*expanded_tokens_idx],
                               &tokens[i]);
            expanded_tokens[*expanded_tokens_idx].num_steps =
                tokens[i].num_steps;
            for (int j = 0; j < tokens[i].num_steps; j++)
                strncpy(expanded_tokens[*expanded_tokens_idx].steps[j],
                        tokens[i].steps[j], 4);
            (*expanded_tokens_idx)++;
        }
        else
        {
            copy_pattern_token(&expanded_tokens[(*expanded_tokens_idx)++],
                               &tokens[i]);
        }
    }
}

bool parse_pattern(char *line, midi_event *target_pattern,
                   unsigned int pattern_type)
{
    if (!is_valid_pattern(line))
    {
        printf("Belched on yer pattern, mate. it was stinky\n");
        return false;
    }

    bool return_val = true;

    int token_idx = 0;
    pattern_token *tokens =
        (pattern_token *)calloc(MAX_PATTERN, sizeof(pattern_token));

    int expanded_tokens_idx = 0;
    pattern_token *expanded_tokens =
        (pattern_token *)calloc(MAX_PATTERN, sizeof(pattern_token));

    int err = 0;
    if ((err = extract_tokens_from_line(tokens, &token_idx, line)))
    {
        printf("Error extracting tokens!\n");
        return_val = false;
        goto tidy_up_and_return;
    }
    expand_the_expanders(tokens, token_idx, expanded_tokens,
                         &expanded_tokens_idx);

    if (!generate_pattern_from_tokens(expanded_tokens, expanded_tokens_idx,
                                      target_pattern, pattern_type))
        return_val = false;

tidy_up_and_return:
    free(tokens);
    free(expanded_tokens);

    return true;
}

void check_and_set_pattern(SoundGenerator *sg, int target_pattern_num,
                           unsigned int pattern_type,
                           char wurds[][SIZE_OF_WURD], int num_wurds)
{
    int line_len = 0;
    for (int i = 0; i < num_wurds; i++)
        line_len += strlen(wurds[i]);
    line_len += num_wurds + 1;
    char line[line_len];
    memset(line, 0, line_len * sizeof(char));
    for (int i = 0; i < num_wurds; i++)
    {
        strcat(line, wurds[i]);
        if (i != num_wurds - 1)
            strcat(line, " ");
    }

    midi_event pattern[PPBAR] = {};
    if (parse_pattern(line, pattern, pattern_type))
    {
        pattern_change_info change_info = {.clear_previous = true,
                                           .temporary = false};
        sequence_engine_set_pattern(&sg->engine, target_pattern_num,
                                    change_info, pattern);
    }
}
