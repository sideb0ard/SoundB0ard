#pragma once

#include <stdbool.h>

#include <defjams.h>
#include <sound_generator.h>

#define MAX_PATTERN 254
#define MAX_PATTERN_CHAR_VAL 100
#define MAX_CHILDREN 20

enum pattern_token_type
{
    SQUARE_BRACKET_LEFT,
    SQUARE_BRACKET_RIGHT,
    CURLY_BRACKET_LEFT,
    CURLY_BRACKET_RIGHT,
    ANGLE_BRACKET_LEFT,
    ANGLE_BRACKET_RIGHT,
    ANGLE_EXPRESSION,
    BLANK,
    VAR_NAME
} pattern_token_type;

typedef struct pattern_token
{
    unsigned int type;
    char value[MAX_PATTERN_CHAR_VAL];

    bool has_multiplier;
    int multiplier;

    bool has_divider;
    int divider;

    bool has_euclid;
    int euclid_hits;
    int euclid_steps;

    char steps[5][5];
    int num_steps;
} pattern_token;

typedef struct pg_child
{
    bool new_level;
    int level_idx;
} pg_child;

typedef struct pattern_group
{
    int num_children;
    pg_child children[MAX_CHILDREN];
    int parent;
} pattern_group;

bool parse_pattern(char *line, midi_event *target_pattern,
                   unsigned int pattern_type);
bool is_valid_pattern(char *line);
void work_out_positions(pattern_group pgroups[MAX_PATTERN], int level,
                        int start_idx, int pattern_len,
                        int ppositions[MAX_PATTERN], int *numpositions);
int extract_tokens_from_pattern_wurds(pattern_token *tokens, int *token_idx,
                                      char *wurd);
bool generate_pattern_from_tokens(pattern_token tokens[MAX_PATTERN],
                                  int num_tokens, midi_event *pattern,
                                  unsigned int pattern_type);
void clear_pattern(midi_event *pattern);
void check_and_set_pattern(soundgenerator *sg, int target_pattern_num,
                           unsigned int pattern_type,
                           char wurds[][SIZE_OF_WURD], int num_wurds);
void pattern_add_event(midi_event *pattern, int midi_tick, midi_event ev);
