#pragma once

#define MAX_PATTERN 64
#define MAX_PATTERN_CHAR_VAL 100
#define MAX_CHILDREN 20

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

typedef struct pattern_token
{
    unsigned int type;
    char value[MAX_PATTERN_CHAR_VAL];
    int idx;
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

void parse_pattern(int num_wurds, char wurds[][SIZE_OF_WURD]);
void work_out_positions(pattern_group pgroups[MAX_PATTERN], int level,
                        int start_idx, int pattern_len,
                        int ppositions[MAX_PATTERN], int *numpositions);
int extract_tokens_from_pattern_wurds(pattern_token *tokens, int *token_idx,
                                      char *wurd);
void parse_tokens_into_groups(pattern_token tokens[MAX_PATTERN],
                              int num_tokens);
