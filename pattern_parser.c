#include <stdio.h>
#include <string.h>

#include "defjams.h"
#include "euclidean.h"
#include "mixer.h"
#include "pattern_parser.h"
#include "sample_sequencer.h"
#include "sequencer_utils.h"

extern mixer *mixr;

static char *token_type_names[] = {"SQUARE_LEFT", "SQUARE_RIGHT", "CURLY_LEFT",
                                   "CURLY_RIGHT", "PAREN_LEFT",   "PAREN_RIGHT",
                                   "BLANK",       "VAR_NAME"};

static bool is_in_array(int num_to_look_for, int *nums, int nums_len)
{
    for (int i = 0; i < nums_len; i++)
        if (nums[i] == num_to_look_for)
            return true;
    return false;
}

void parse_pattern(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    printf("Got %d wurds\n", num_wurds);
    pattern_token tokens[MAX_PATTERN] = {0};
    int token_idx = 0;

    // 1. parse all wurds into tokens
    int sq_bracket_balance = 0;
    for (int i = 0; i < num_wurds; i++)
        sq_bracket_balance +=
            extract_tokens_from_pattern_wurds(tokens, &token_idx, wurds[i]);
    printf("TOKEN_IDX is %d\n", token_idx);

    if (sq_bracket_balance != 0)
    {
        printf("NAH, MATE! brackets aren't balanced.\n");
        return;
    }

    for (int i = 0; i < token_idx; i++)
    {
        printf("Tokens: %s // %s\n", token_type_names[tokens[i].type],
               tokens[i].value);
    }

    // 2. parse tokens into ordered groups
    parse_tokens_into_groups(tokens, token_idx);
}

void parse_tokens_into_groups(pattern_token tokens[MAX_PATTERN], int num_tokens)
{
    pattern_group pgroups[MAX_PATTERN] = {0};
    int current_pattern_group = 0;
    int num_pattern_groups = 0;

    pattern_token var_tokens[100] = {0};
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
        else if (tokens[i].type == BLANK || tokens[i].type == VAR_NAME)
        {
            pgroups[current_pattern_group].num_children++;
            var_tokens[var_tokens_idx++] = tokens[i];
        }
    }

    printf("Num Groups:%d\n", num_pattern_groups);
    for (int i = 0; i <= num_pattern_groups; i++)
        printf("Group %d - parent is %d contains %d members\n", i,
               pgroups[i].parent, pgroups[i].num_children);

    int level = 0;
    int start_idx = 0;
    int pattern_len = PPBAR;
    int ppositions[MAX_PATTERN] = {0};
    int numpositions = 0;
    work_out_positions(pgroups, level, start_idx, pattern_len, ppositions,
                       &numpositions);

    int num_uniq = 0;
    int uniq_positions[MAX_PATTERN] = {0};
    for (int i = 0; i < numpositions; i++)
        if (!is_in_array(ppositions[i], uniq_positions, num_uniq))
            uniq_positions[num_uniq++] = ppositions[i];

    if (num_uniq != var_tokens_idx)
    {
        printf("Vars and timings don't match, ya numpty: num_uniq:%d "
               "var_tokens:%d\n",
               num_uniq, var_tokens_idx);
        return;
    }

    // 3. verify env vars
    for (int i = 0; i < var_tokens_idx; i++)
    {
        char *var_key = var_tokens[i].value;
        if (mixer_is_valid_env_var(mixr, var_key))
        {
            printf("Valid ENV VAR! %s\n", var_key);
            int sg_num;
            if (get_environment_val(var_key, &sg_num))
            {
                printf("CLEARING %s(%d)\n", var_key, sg_num);
                sample_sequencer *seq =
                    (sample_sequencer *)mixr->sound_generators[sg_num];
                seq_clear_pattern(&seq->m_seq, 0);
            }
        }
        else if (var_tokens[i].type == BLANK)
        {
        } // no-op
        else
        {
            printf("NAE Valid ENV VAR! %s\n", var_key);
            return;
        }
    }

    for (int i = 0; i < num_uniq; i++)
    {
        if (var_tokens[i].type != BLANK)
        {
            int sg_num;
            get_environment_val(var_tokens[i].value, &sg_num);
            printf("Play at %s %d\n", var_tokens[i].value, uniq_positions[i]);
            sample_sequencer *seq =
                (sample_sequencer *)mixr->sound_generators[sg_num];
            seq_add_micro_hit(&seq->m_seq, 0, uniq_positions[i]);
        }
    }
}

int extract_tokens_from_pattern_wurds(pattern_token *tokens, int *token_idx,
                                      char *wurd)
{
    int sq_bracket_balance = 0;

    printf("WURD %s\n", wurd);
    char *c = wurd;
    while (*c)
    {
        char var_name[100] = {0};
        int var_name_idx = 0;

        if (*c == '[')
        {
            sq_bracket_balance++;
            printf("SQ_LEFTBRACKET!\n");
            tokens[(*token_idx)++].type = SQUARE_BRACKET_LEFT;
            c++;
        }
        else if (*c == ']')
        {
            sq_bracket_balance--;
            printf("SQ_RIGHTBRACKET!\n");
            tokens[(*token_idx)++].type = SQUARE_BRACKET_RIGHT;
            c++;
        }
        else if ((*c == '_') || (*c == '-') || (*c == '~'))
        {
            printf("BLANK!\n");
            tokens[(*token_idx)++].type = BLANK;
            c++;
        }
        else
        {
            while (isalnum(*c))
                var_name[var_name_idx++] = *c++;
            printf("VAR! %s\n", var_name);
            tokens[(*token_idx)].type = VAR_NAME;
            strncpy(tokens[*token_idx].value, var_name, MAX_PATTERN_CHAR_VAL);
            (*token_idx)++;
        }
    }

    return sq_bracket_balance;
}

void work_out_positions(pattern_group pgroups[MAX_PATTERN], int level,
                        int start_idx, int pattern_len,
                        int ppositions[MAX_PATTERN], int *numpositions)
{

    // printf("Looking at Level:%d start_idx:%d pattern_len: %d\n", level,
    // start_idx, pattern_len);
    int num_children = pgroups[level].num_children;
    if (num_children != 0)
    {
        int incr = pattern_len / num_children;
        for (int i = 0; i < num_children; i++)
        {
            int child = pgroups[level].children[i].level_idx;
            int chidx = (i * incr) + start_idx;
            // printf("CHILD:%d plays at pos%d\n", child, chidx);
            ppositions[(*numpositions)++] = chidx;
            if (pgroups[level].children[i].new_level)
            {
                // printf("NEW LEVEL!\n");
                work_out_positions(pgroups, child, chidx, incr, ppositions,
                                   numpositions);
            }
        }
    }
}
