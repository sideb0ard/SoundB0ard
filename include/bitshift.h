#pragma once

#include "defjams.h"
#include "sequence_generator.h"
#include "stack.h"

enum unary_or_binary
{
    UNARY,
    BINARY
};

enum bitshift_token_type
{
    NUMBER,
    OPERATOR,
    TEE_TOKEN,
    BRACKET,
};

enum bitshift_ops
{
    LEFTSHIFT,
    RIGHTSHIFT,
    XOR,
    OR,
    NOT,
    AND,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    TEE, // letter 't' time counter
    NUM_OPS
};

typedef struct bitshift_token
{
    int type;
    int val;
} bitshift_token;

static const char BITSHIFT_PRESET_FILENAME[] = "settings/bitshiftz.dat";

#define MAX_TOKENS_IN_PATTERN 100 // arbitrary
typedef struct bitshift_pattern
{
    bitshift_token infix_tokenized_pattern[MAX_TOKENS_IN_PATTERN];
    bitshift_token rpn_tokenized_pattern[MAX_TOKENS_IN_PATTERN];
    int num_infix_tokens;
    int num_rpn_tokens;
} bitshift_pattern;

typedef struct bitshift
{
    sequence_generator sg;
    bitshift_pattern pattern;
    int time_counter;
} bitshift;

sequence_generator *new_bitshift(int argc, char argv[][SIZE_OF_WURD]);
uint16_t bitshift_generate(void *self, void *data);
void bitshift_status(void *self, wchar_t *status_string);
void bitshift_event_notify(void *self, unsigned int event_type);

int parse_bitshift_tokens_from_wurds(int argc, char argv[][SIZE_OF_WURD],
                                     bitshift_token *tokenized_pattern,
                                     int *num_tokens);
void tokenized_pattern_to_string(bitshift_token *pattern, int token_len,
                                 char *pattern_as_string, int stringlen);
void shunting_yard_algorithm(bitshift_pattern *pattern);

int associativity(int op); // left or right
int bin_or_uni(int op);
int precedence(int op);

void bitshift_set_time_counter(bitshift *bs, int time);
void bitshift_set_debug(void *self, bool b);
bool bitshift_save(bitshift *bs, char *preset_name);
bool bitshift_load(bitshift *bs, char *preset_name);
