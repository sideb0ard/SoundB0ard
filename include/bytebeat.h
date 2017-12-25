#pragma once

#include "defjams.h"
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

enum token_type
{
    NUMBER,
    OPERATOR,
    TEE_TOKEN,
};

enum ops
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

typedef struct token
{
    int type;
    int val;
} token;

#define MAX_TOKENS_IN_PATTERN 100 // arbitrary
typedef struct bytebeat
{
    char pattern[1024];
    token tokenized_pattern[MAX_TOKENS_IN_PATTERN];
    Stack *rpn_stack;
} bytebeat;

void bytebeat_init(bytebeat *b);
double bytebeat_gennext(bytebeat *b);
void bytebeat_status(bytebeat *b, wchar_t *status_string);

Stack *new_rpn_stack(char *apattern);
char interpreter(Stack *rpn_stack);

int parse_tokens_from_wurds(int argc, char argv[][SIZE_OF_WURD],
                            token *tokenized_pattern);

int associativity(int op); // left or right
int bin_or_uni(int op);
int calc(int operator, int first_operand, int val);
int eval_token(token *re_token, Stack *ans_stack, token *tmp_tokens,
               int *tmp_token_num);
int parse_bytebeat(char *pattern, Stack *stack);
char parse_rpn(Stack *rpn_stack);
int precedence(int op);
bool isnum(char *ch);
bool isvalid_char(char *ch);
bool isvalid_pattern(char *pattern);
void reverse_stack(Stack *rpn_stack);
