#include <stdlib.h>
#include <string.h>

#include "bitshift.h"
#include "defjams.h"
#include "mixer.h"
#include "sequence_generator.h"
#include "utils.h"

extern mixer *mixr;
static char *s_token_types[] = {"NUMBER", "OPERATOR", "TEE_TOKEN", "BRACKET"};
static char *s_brackets[] = {"(", ")"};
static char *s_ops[] = {"<<", ">>", "^", "|", "~", "&",
                        "+",  "-",  "*", "/", "%", "t"};

void token_val_to_string(bitshift_token *t, char *char_val)
{
    if (t->type == NUMBER)
        itoa(t->val, char_val);
    else if (t->type == TEE_TOKEN)
        itoa(mixr->timing_info.cur_sample, char_val);
    else if (t->type == BRACKET)
        strcpy(char_val, s_brackets[t->val]);
    else
        strcpy(char_val, s_ops[t->val]);
}
void tokenized_pattern_to_string(bitshift_token *pattern, int token_len,
                                 char *pattern_as_string, int stringlen)
{
    memset(pattern_as_string, 0, stringlen);
    int string_idx = 0;
    for (int i = 0; i < token_len; i++)
    {
        bitshift_token t = pattern[i];

        char char_val[100] = {0};
        token_val_to_string(&t, char_val);
        if (i != token_len - 1)
            strcat(char_val, " ");

        int lenny = strlen(char_val);
        if ((string_idx + lenny) < stringlen)
            strncat(pattern_as_string, char_val, lenny);
        else
        {
            printf("Pattern too long for string!\n");
            return;
        }
        string_idx += lenny;
    }
}

#define LARGEST_POSSIBLE 2048
sequence_generator *new_bitshift(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    bitshift_pattern my_pattern;
    int err = parse_bitshift_tokens_from_wurds(
        num_wurds, wurds, my_pattern.infix_tokenized_pattern,
        &my_pattern.num_infix_tokens);
    if (err)
    {
        printf("ERR!! %d\n", err);
        return NULL;
    }
    shunting_yard_algorithm(&my_pattern);

    bitshift *bs = calloc(1, sizeof(bitshift));
    if (!bs)
    {
        printf("WOOF!\n");
        return NULL;
    }
    memcpy(&bs->pattern, &my_pattern, sizeof(my_pattern));
    bs->sg.status = &bitshift_status;
    bs->sg.generate = &bitshift_generate;

    return (sequence_generator *)bs;
}
void bitshift_change_pattern(bitshift *sg, char *pattern)
{
    // TODO
}

void bitshift_status(void *self, wchar_t *wstring)
{
    bitshift *bs = (bitshift *)self;
    int patsy_size = MAX_PS_STRING_SZ / 2;
    char infix_pattern[patsy_size];
    char rpn_pattern[patsy_size];
    tokenized_pattern_to_string(bs->pattern.infix_tokenized_pattern,
                                bs->pattern.num_infix_tokens, infix_pattern,
                                patsy_size);
    tokenized_pattern_to_string(bs->pattern.rpn_tokenized_pattern,
                                bs->pattern.num_rpn_tokens, rpn_pattern,
                                patsy_size);
    swprintf(wstring, MAX_PS_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "SEQUENCE GEN ] - " WCOOL_COLOR_PINK
             "infix pattern: %s // rpn pattern: %s",
             infix_pattern, rpn_pattern);
}

int bitshift_generate(void *self)
{
    // bitshift *sg = (bitshift*) self;
    // char ans = interpreter(sg->rpn_stack);
    // print_bin_num(ans);
    // return ans;
    return 0;
}

static unsigned int which_op(char c)
{
    switch (c)
    {
    case ('<'):
        return LEFTSHIFT;
        break;
    case ('>'):
        return RIGHTSHIFT;
        break;
    case ('^'):
        return XOR;
        break;
    case ('|'):
        return OR;
        break;
    case ('~'):
        return NOT;
        break;
    case ('&'):
        return AND;
        break;
    case ('+'):
        return PLUS;
        break;
    case ('-'):
        return MINUS;
        break;
    case ('*'):
        return MULTIPLY;
        break;
    case ('/'):
        return DIVIDE;
        break;
    case ('%'):
        return MODULO;
        break;
    case ('('):
        return LEFT;
        break;
    case (')'):
        return RIGHT;
        break;
    case ('t'):
        return TEE;
        break;
    default:
        return NUM_OPS;
    }
}

static bool is_shift_char(char ch)
{
    if (ch == '<' || ch == '>')
        return true;
    return false;
}

static bool is_bracket_char(char ch)
{
    if (ch == '(' || ch == ')')
        return true;
    return false;
}

static bool isvalid_char(char *ch)
{
    if (isdigit(*ch))
        return true;

    static char acceptable[] = {'<', '>', '^', '|', '~', '&', '+',
                                '-', '*', '/', '%', '(', ')', 't'};

    int acceptable_len = strlen(acceptable);
    for (int i = 0; i < acceptable_len; i++)
    {
        if (*ch == acceptable[i])
            return true;
    }
    return false;
}

#define MAXLEN_CHAR_NUMBER 50 // arbitrary
int parse_bitshift_tokens_from_wurds(int argc, char argv[][SIZE_OF_WURD],
                                     bitshift_token *tokenized_pattern,
                                     int *num_tokens)
{
    int tokenized_pattern_idx = 0;
    for (int i = 0; i < argc; i++)
    {
        char *c = argv[i];
        while (*c)
        {
            if (*c == 32)
            {
            } // Space, final frontier, is a NO-OP
            else if (isdigit(*c))
            {
                char multichar_num[MAXLEN_CHAR_NUMBER] = {0};
                int idx = 0;
                while (isdigit(*(c + 1)))
                    multichar_num[idx++] = *c++;
                multichar_num[idx++] = *c;

                bitshift_token t = {NUMBER, atoi(multichar_num)};
                if (tokenized_pattern_idx < MAX_TOKENS_IN_PATTERN)
                    tokenized_pattern[tokenized_pattern_idx++] = t;
                else
                    return 1;
            }
            else if (is_shift_char(*c))
            {
                char shifty = *c;
                c++;
                if (!is_shift_char(*c) || *c != shifty)
                {
                    printf("TOOOOO SHIFTY barfed on mismatching %c and %c!\n",
                           shifty, *c);
                    return 1;
                }

                bitshift_token t;
                t.type = OPERATOR;
                if (shifty == '<')
                    t.val = LEFTSHIFT;
                else
                    t.val = RIGHTSHIFT;
                tokenized_pattern[tokenized_pattern_idx++] = t;
            }
            else if (is_bracket_char(*c))
            {
                bitshift_token t;
                t.type = BRACKET;
                if (*c == '(')
                    t.val = LEFT;
                else
                    t.val = RIGHT;
                tokenized_pattern[tokenized_pattern_idx++] = t;
            }
            else if (isvalid_char(c))
            {
                unsigned int op = which_op(*c);
                bitshift_token t;
                if (op == TEE)
                {
                    t.type = TEE_TOKEN;
                    t.val = 0;
                }
                else
                {
                    t.type = OPERATOR;
                    t.val = op;
                }
                tokenized_pattern[tokenized_pattern_idx++] = t;
            }
            else
            {
                printf("NAE VALID - barfed on %c!\n", *c);
                return 1;
            }
            c++;
        }
    }
    *num_tokens = tokenized_pattern_idx;
    return 0;
}

// http://en.cppreference.com/w/c/language/operator_precedence
int precedence(int op)
{
    switch (op)
    {
    case 0: // left and right shift
    case 1:
        return 4;
    case 2: // XOR
        return 2;
    case 3: // bitwize OR
        return 1;
    case 4: // bitwize NOT ~
        return 7;
    case 5: // butwize AND
        return 3;
    case 6: // PLUS
    case 7: // MINUS
        return 5;
    case 8:  // MULTIPLY
    case 9:  // DIVIDE
    case 10: // MODULO
        return 6;
    case 11: // left bracket
    case 12: // right bracket
        return 0;
    default:
        return 99;
    }
}

int associativity(int op)
{
    switch (op)
    {
    case 4:
        return RIGHT;
    default:
        return LEFT;
    }
}

int bin_or_uni(int op)
{
    switch (op)
    {
    case 4: // bitwise NOT ~
        return UNARY;
    default:
        return BINARY;
    }
}

// https://en.wikipedia.org/wiki/Shunting-yard_algorithm#The_algorithm_in_detail
// converts an infix notation pattern to reverse polish notation
void shunting_yard_algorithm(bitshift_pattern *pattern)
{
    int num_infix = pattern->num_infix_tokens;
    bitshift_token *infix_tokens = pattern->infix_tokenized_pattern;

    int output_stack_idx = 0;
    bitshift_token output_stack[MAX_TOKENS_IN_PATTERN] = {0};

    int operator_stack_idx = 0;
    bitshift_token operator_stack[MAX_TOKENS_IN_PATTERN] = {0};

    for (int i = 0; i < num_infix; i++)
    {
        bitshift_token cur = infix_tokens[i];

        char char_val[100] = {0};
        token_val_to_string(&cur, char_val);
        printf("Looking at %s\n", char_val);

        if (cur.type == NUMBER)
        {
            printf("NUMBER, pushing to output stack\n");
            output_stack[output_stack_idx++] = cur;
        }
        else if (cur.type == TEE_TOKEN)
        {
            printf("TEE, pushing to output stack\n");
            output_stack[output_stack_idx++] = cur;
        }
        else if (cur.type == OPERATOR)
        {
            printf("We're an OPERATOR!\n");
            bitshift_token top_of_op_stack =
                operator_stack[operator_stack_idx - 1];
            while (operator_stack_idx > 0 &&
                   (((precedence(top_of_op_stack.val) > precedence(cur.val)) ||
                     (precedence(top_of_op_stack.val) == precedence(cur.val) &&
                      associativity(top_of_op_stack.val) == LEFT)) &&
                    (!(top_of_op_stack.type == BRACKET &&
                       top_of_op_stack.val == LEFT))))
            {
                printf("POPPING top o' OP stack: %s %d\n",
                       s_token_types[top_of_op_stack.type],
                       top_of_op_stack.val);
                output_stack[output_stack_idx++] =
                    operator_stack[--operator_stack_idx];
                top_of_op_stack = operator_stack[operator_stack_idx - 1];
            }

            operator_stack[operator_stack_idx++] = cur;
        }
        else if (cur.type == BRACKET)
        {
            if (cur.val == LEFT)
            {
                printf("LEFTY - pushing to operator stack!\n");
                operator_stack[operator_stack_idx++] = cur;
            }
            else if (cur.val == RIGHT)
            {
                bool found_left = false;
                bitshift_token top_of_op_stack =
                    operator_stack[operator_stack_idx - 1];
                while (operator_stack_idx > 0 &&
                       (!(top_of_op_stack.type == BRACKET &&
                          top_of_op_stack.val == LEFT)))
                {
                    output_stack[output_stack_idx++] =
                        operator_stack[--operator_stack_idx];
                    top_of_op_stack = operator_stack[operator_stack_idx - 1];
                }
                if (!(top_of_op_stack.type == BRACKET &&
                      top_of_op_stack.val == LEFT))
                {
                    printf("Mis-matched - didn't find LEFT BRACKET\n");
                }
                else
                {
                    printf("FOUND LEFT BRACKET, all good!\n");
                    --operator_stack_idx;
                }
            }
        }
    }
    while (operator_stack_idx > 0)
        output_stack[output_stack_idx++] = operator_stack[--operator_stack_idx];

    printf("\n\nOUTPUT STACK SIZE: %d\n", output_stack_idx);
    for (int i = 0; i < output_stack_idx; i++)
    {
        bitshift_token cur = output_stack[i];
        printf("CUR %d:%d\n", cur.type, cur.val);
        char char_val[100] = {0};
        token_val_to_string(&cur, char_val);
        printf("outputstack %d %s\n", i, char_val);
    }
    char a_pattern[MAX_PS_STRING_SZ];
    // tokenized_pattern_to_string(bs->pattern.infix_tokenized_pattern,
    // bs->pattern.num_infix_tokens, a_pattern, MAX_PS_STRING_SZ);
    tokenized_pattern_to_string(output_stack, output_stack_idx, a_pattern,
                                MAX_PS_STRING_SZ);
    printf("TOKEN2STRING: %s\n", a_pattern);
    // memset(pattern->rpn_tokenized_pattern, 0, MAX_TOKENS_IN_PATTERN *
    // sizeof(bitshift_token));
    memcpy(pattern->rpn_tokenized_pattern, output_stack,
           MAX_TOKENS_IN_PATTERN * sizeof(bitshift_token));
    pattern->num_rpn_tokens = output_stack_idx;
}
