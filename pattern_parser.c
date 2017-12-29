#include <stdio.h>

#include "defjams.h"
#include "euclidean.h"
#include "mixer.h"
#include "pattern_parser.h"
#include "sample_sequencer.h"
#include "sequencer_utils.h"

extern mixer *mixr;

enum pattern_token_type
{
    SQUARE_BRACKET_LEFT,
    SQUARE_BRACKET_RIGHT,
    CURLY_BRACKET_LEFT,
    CURLY_BRACKET_RIGHT,
    PAREN_BRACKET_LEFT,
    PAREN_BRACKET_RIGHT,
    VAR_NAME
} pattern_token_type;

#define MAX_PATTERN_CHAR_VAL 100
typedef struct pattern_token
{
    unsigned int type;
    char value[MAX_PATTERN_CHAR_VAL];
} pattern_token;

void extract_tokens_from_pattern_wurds(pattern_token *tokens, int *token_idx,
                                       char *wurd)
{
    printf("TOKEN IDX:%d\n", *token_idx);
    printf("Looking at %s\n", wurd);
    char *c = wurd;
    while (*c)
    {
        printf("%c\n", *c);
        if (*c == '[')
            printf("SQ_LEFTBRACKET!\n");
        else if (*c == ']')
            printf("SQ_RIGHTBRACKET!\n");
        else
            printf("VAR!\n");
        c++;
    }
}

#define MAX_PATTERN 64
void parse_pattern(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    pattern_token tokens[MAX_PATTERN];
    int token_idx = 0;
    printf("Got %d wurds\n", num_wurds);
    for (int i = 0; i < num_wurds; i++)
    {
        if (mixer_is_valid_env_var(mixr, wurds[i]))
        {
            printf("Valid ENV VAR! %s\n", wurds[i]);
            int sg_num;
            if (get_environment_val(wurds[i], &sg_num))
            {
                printf("CLEARING %s(%d)\n", wurds[i], sg_num);
                sample_sequencer *seq =
                    (sample_sequencer *)mixr->sound_generators[sg_num];
                seq_clear_pattern(&seq->m_seq, 0);
            }
        }
        else
            printf("NAE Valid ENV VAR! %s\n", wurds[i]);
        // extract_tokens_from_pattern_wurds(tokens, &token_idx, wurds[i]);
    }
    int rhythm = create_euclidean_rhythm(num_wurds, 16);
    char rhythmbit[17];
    char_binary_version_of_int(rhythm, rhythmbit);
    printf("Pattern: %s\n", rhythmbit);

    int wurd_idx = 0;
    for (int i = 15; i >= 0; i--)
    {
        if (rhythm & 1 << i)
        {
            int step = 15 - i;
            int sg_num;
            printf("YAR, HIT!\n");
            if (get_environment_val(wurds[wurd_idx], &sg_num))
            {
                printf("Playing %s(%d) at step:%d\n", wurds[wurd_idx], sg_num,
                       step);
                sample_sequencer *seq =
                    (sample_sequencer *)mixr->sound_generators[sg_num];
                seq_add_hit(&seq->m_seq, 0, step);
            }
            wurd_idx++;
        }
    }
    // for (int i = 0; i < token_idx; i++)
    //{
    //    printf("Tokens: %s\n", tokens[i].value);
    //}
    // 1. parse all wurds into tokens
    // 2. parse tokens into ordered groups
    // 3. parse groups into var separated patterns
    // 4. apply patterns to var/instruments
}
