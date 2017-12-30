#include <stdio.h>
#include <string.h>

#include "defjams.h"
#include "euclidean.h"
#include "mixer.h"
#include "pattern_parser.h"
#include "sample_sequencer.h"
#include "sequencer_utils.h"

extern mixer *mixr;

static bool is_in_array(int num_to_look_for, int *nums, int nums_len)
{
    for (int i = 0; i < nums_len; i++)
        if (nums[i] == num_to_look_for)
            return true;
    return false;
}

void parse_pattern(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    pattern_token tokens[MAX_PATTERN];
    int token_idx = 0;
    printf("Got %d wurds\n", num_wurds);

    pattern_group pgroups[MAX_PATTERN] = {0};
    int current_pattern_group = 0;
    int num_pattern_groups = 0;

    pattern_token token_vars[100] = {0};
    int token_vars_idx = 0;

    for (int i = 0; i < num_wurds; i++)
    {
        if (strncmp(wurds[i], "[", 1) == 0)
        {
            pgroups[++num_pattern_groups].parent = current_pattern_group;
            int cur_child = pgroups[current_pattern_group].num_children++;
            pgroups[current_pattern_group].children[cur_child].level_idx = num_pattern_groups;
            pgroups[current_pattern_group].children[cur_child].new_level = true;
            current_pattern_group = num_pattern_groups;
        }
        else if (strncmp(wurds[i], "]", 1) == 0)
            current_pattern_group = pgroups[current_pattern_group].parent;
        else
        {
            pgroups[current_pattern_group].num_children++;

            token_vars[token_vars_idx].type = VAR_NAME;
            strcpy(token_vars[token_vars_idx].value, wurds[i]);
            token_vars_idx++;
        }
    }

    printf("Num Groups:%d\n", num_pattern_groups);
    for (int i = 0; i <= num_pattern_groups; i++)
        printf("Group %d - parent is %d contains %d members\n", i, pgroups[i].parent, pgroups[i].num_children);

    int level = 0;
    int start_idx = 0;
    int pattern_len = PPBAR;
    int ppositions[MAX_PATTERN] = {0};
    int numpositions;
    work_out_positions(pgroups, level, start_idx, pattern_len, ppositions, &numpositions);

    int num_uniq = 0;
    int uniq_positions[MAX_PATTERN] = {0};
    for (int i = 0; i < numpositions; i++)
        if (!is_in_array(ppositions[i], uniq_positions, num_uniq))
            uniq_positions[num_uniq++] = ppositions[i];

    if (num_uniq != token_vars_idx)
        printf("Vars and timings don't match, ya numpty\n");
    else {
        for (int i = 0; i < num_uniq; i++)
            printf("Play at %s %d\n", token_vars[i].value, uniq_positions[i]);
    }

    //int sq_bracket_balance = 0;
    //for (int i = 0; i < num_wurds; i++)
    //{
    //    //if (mixer_is_valid_env_var(mixr, wurds[i]))
    //    //{
    //    //    printf("Valid ENV VAR! %s\n", wurds[i]);
    //    //    int sg_num;
    //    //    if (get_environment_val(wurds[i], &sg_num))
    //    //    {
    //    //        printf("CLEARING %s(%d)\n", wurds[i], sg_num);
    //    //        sample_sequencer *seq =
    //    //            (sample_sequencer *)mixr->sound_generators[sg_num];
    //    //        seq_clear_pattern(&seq->m_seq, 0);
    //    //    }
    //    //}
    //    //else
    //    //    printf("NAE Valid ENV VAR! %s\n", wurds[i]);
    //    sq_bracket_balance += extract_tokens_from_pattern_wurds(tokens, &token_idx, wurds[i]);
    //}

    //printf("\n");
    //if (sq_bracket_balance != 0)
    //    printf("NAH, MATE! brackets aren't balanced.\n");
    //else
    //    printf("Kosher, mate\n");

    //int rhythm = create_euclidean_rhythm(num_wurds, 16);
    //char rhythmbit[17];
    //char_binary_version_of_int(rhythm, rhythmbit);
    //printf("Pattern: %s\n", rhythmbit);

    //int wurd_idx = 0;
    //for (int i = 15; i >= 0; i--)
    //{
    //    if (rhythm & 1 << i)
    //    {
    //        int step = 15 - i;
    //        int sg_num;
    //        printf("YAR, HIT!\n");
    //        if (get_environment_val(wurds[wurd_idx], &sg_num))
    //        {
    //            printf("Playing %s(%d) at step:%d\n", wurds[wurd_idx], sg_num,
    //                   step);
    //            sample_sequencer *seq =
    //                (sample_sequencer *)mixr->sound_generators[sg_num];
    //            seq_add_hit(&seq->m_seq, 0, step);
    //        }
    //        wurd_idx++;
    //    }
    //}

    // for (int i = 0; i < token_idx; i++)
    //{
    //    printf("Tokens: %s\n", tokens[i].value);
    //}
    // 1. parse all wurds into tokens
    // 2. parse tokens into ordered groups
    // 3. parse groups into var separated patterns
    // 4. apply patterns to var/instruments
}
int extract_tokens_from_pattern_wurds(pattern_token *tokens, int *token_idx,
                                       char *wurd)
{
    //printf("TOKEN IDX:%d\n", *token_idx);
    //printf("Looking at %s\n", wurd);

    int sq_bracket_balance = 0;

    char *c = wurd;
    while (*c)
    {
        printf("%c\n", *c);
        if (*c == '[')
        {
            sq_bracket_balance++;
            printf("SQ_LEFTBRACKET!\n");
        }
        else if (*c == ']')
        {
            sq_bracket_balance--;
            printf("SQ_RIGHTBRACKET!\n");
        }
        else
            printf("VAR!\n");
        c++;
    }

    return sq_bracket_balance;
}

void work_out_positions(pattern_group pgroups[MAX_PATTERN],
                        int level,
                        int start_idx,
                        int pattern_len,
                        int ppositions[MAX_PATTERN],
                        int *numpositions)
{

    //printf("Looking at Level:%d start_idx:%d pattern_len: %d\n", level, start_idx, pattern_len);
    int num_children = pgroups[level].num_children;
    int incr = pattern_len / num_children;
    pattern_len /= num_children;
    for (int i = 0; i < num_children; i++)
    {
        int child = pgroups[level].children[i].level_idx;
        int chidx = (i*incr) + start_idx;
        //printf("CHILD:%d plays at pos%d\n", child, chidx);
        ppositions[(*numpositions)++] = chidx;
        if (pgroups[level].children[i].new_level)
        {
            //printf("NEW LEVEL!\n");
            work_out_positions(pgroups, child, chidx, incr, ppositions, numpositions);
        }
    }
}


