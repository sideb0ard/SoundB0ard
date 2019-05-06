#pragma once

#include "defjams.h"
#include "pattern_generator.h"
#include "stack.h"

#include <functional>
#include <stack>
#include <vector>

enum class SymbolType
{
    NUMBER,
    OP,
    LEFT_PARENS,
    RIGHT_PARENS,
    TEE_TOKEN
};

enum class SymbolAssociativity
{
    LEFT,
    RIGHT
};

enum class Aryness
{
    UNARY,
    BINARY
};

enum class OperatorType
{
    LEFT_SHIFT,
    RIGHT_SHIFT,
    XOR,
    OR,
    NOT,
    AND,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    UNUSED, // for TEE and PARENS
    NUM_OPS
};

class Symbol
{
  public:
    Symbol(SymbolType type, int value); // NUMBER
    Symbol(SymbolType type, std::string identifier,
           OperatorType op_type); // OP
    SymbolType sym_type;

    // used if type is NUMBER
    int value;

    // used if type is OP
    OperatorType op_type;
    std::string identifier; // used for print
    int precedence;
    SymbolAssociativity associativity;
    Aryness ary;
};

// enum unary_or_binary
//{
//    UNARY,
//    BINARY
//};
//
// enum bitshift_token_type
//{
//    NUMBER,
//    OPERATOR,
//    TEE_TOKEN,
//    BRACKET,
//};
//
// enum bitshift_ops
//{
//    LEFTSHIFT,
//    RIGHTSHIFT,
//    XOR,
//    OR,
//    NOT,
//    AND,
//    PLUS,
//    MINUS,
//    MULTIPLY,
//    DIVIDE,
//    MODULO,
//    TEE, // letter 't' time counter
//    NUM_OPS
//};
//
// typedef struct bitshift_token
//{
//    int type;
//    int val;
//} bitshift_token;

static const char BITSHIFT_PRESET_FILENAME[] = "settings/bitshiftz.dat";

//#define MAX_TOKENS_IN_PATTERN 100 // arbitrary
typedef struct bitshift_pattern
{
    std::vector<Symbol> infix_tokenized_pattern;
    std::vector<Symbol> rpn_tokenized_pattern;
    // bitshift_token rpn_tokenized_pattern[MAX_TOKENS_IN_PATTERN];
    // int num_infix_tokens;
    // int num_rpn_tokens;
} bitshift_pattern;

typedef struct bitshift
{
    pattern_generator sg;
    bitshift_pattern pattern;
    int time_counter;
} bitshift;

pattern_generator *new_bitshift(int argc, char argv[][SIZE_OF_WURD]);
void bitshift_generate(void *self, void *data);
void bitshift_status(void *self, wchar_t *status_string);
void bitshift_event_notify(void *self, broadcast_event event);

// int parse_bitshift_tokens_from_wurds(int argc, char argv[][SIZE_OF_WURD],
//                                     bitshift_token *tokenized_pattern,
//                                     int *num_tokens);
// void tokenized_pattern_to_string(bitshift_token *pattern, int token_len,
//                                 char *pattern_as_string, int stringlen);
// void shunting_yard_algorithm(bitshift_pattern *pattern);

// int associativity(int op); // left or right
// int bin_or_uni(int op);
// int precedence(int op);
std::ostream &operator<<(std::ostream &out, const Symbol &s);
void convert_to_infix(const std::vector<Symbol> &sym_list,
                      std::vector<Symbol> &output);
void extract_symbols_from_line(const std::string &line,
                               std::vector<Symbol> &output);
std::string symbols_to_string(const std::vector<Symbol> pattern);

void bitshift_set_time_counter(bitshift *bs, int time);
void bitshift_set_debug(void *self, bool b);
bool bitshift_save(bitshift *bs, char *preset_name);
bool bitshift_load(bitshift *bs, char *preset_name);
