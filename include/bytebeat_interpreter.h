#pragma once
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
    TEE // letter 't' time counter
};

typedef struct token
{
    int type;
    int val;
} token;

Stack *new_rpn_stack(char *apattern);
char interpreter(Stack *rpn_stack);

int associativity(int op); // left or right
int bin_or_uni(int op);
int calc(int operator, int first_operand, int val);
int eval_token(token *re_token, Stack *ans_stack, token *tmp_tokens, int *tmp_token_num);
int parse_bytebeat(char *pattern, Stack *stack);
char parse_rpn(Stack *rpn_stack);
int precedence(int op);
bool isnum(char *ch);
bool isvalid_char(char *ch);
bool isvalid_pattern(char *pattern);
void reverse_stack(Stack *rpn_stack);
