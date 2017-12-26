#pragma once

#include "defjams.h"
#include "sequence_generator.h"
#include "stack.h"

enum direction
{
    LEFT,
    RIGHT
};

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
    LEFTBRACKET,
    RIGHTBRACKET,
    TEE, // letter 't' time counter
    NUM_OPS
};

typedef struct bitshift_token
{
    int type;
    int val;
} bitshift_token;

#define MAX_TOKENS_IN_PATTERN 100 // arbitrary
typedef struct bitshift_pattern
{
    bitshift_token tokenized_pattern[MAX_TOKENS_IN_PATTERN];
    int num_tokens;
} bitshift_pattern;

typedef struct bitshift
{
    sequence_generator sg;
    bitshift_pattern pattern;
    //Stack *rpn_stack;
} bitshift;

sequence_generator *new_bitshift(int argc, char argv[][SIZE_OF_WURD]);
//void bitshift_init(bitshift *b);
int bitshift_generate(void *self);
void bitshift_status(void *self, wchar_t *status_string);

//Stack *new_rpn_stack(char *apattern);
//char interpreter(Stack *rpn_stack);

int parse_bitshift_tokens_from_wurds(int argc, char argv[][SIZE_OF_WURD],
                            bitshift_token *tokenized_pattern, int *num_tokens);
void tokenized_pattern_to_string(bitshift_pattern *pattern, char *pattern_as_string, int stringlen);

//int associativity(int op); // left or right
//int bin_or_uni(int op);
//int calc(int operator, int first_operand, int val);
//int eval_token(token *re_token, Stack *ans_stack, token *tmp_tokens,
//               int *tmp_token_num);
//int parse_bitshift_pattern(char *pattern, Stack *stack);
//char parse_rpn(Stack *rpn_stack);
//int precedence(int op);
//bool isnum(char *ch);
//bool isvalid_char(char *ch);
//bool isvalid_pattern(char *pattern);
//void reverse_stack(Stack *rpn_stack);
