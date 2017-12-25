#include <stdlib.h>
#include <string.h>

#include "bytebeat.h"
#include "defjams.h"
#include "sequence_generator.h"
#include "utils.h"

static char *s_token_types[] = {"NUMBER", "OPERATOR", "TEE_TOKEN"};
static char *s_ops[] = {"<<", ">>", "^", "|", "~", "&", "+",
                        "-",  "*",  "/", "%", "(", ")", "t"};

void print_token(token t)
{
    char num_val[10] = {0};
    if (t.type == NUMBER)
        itoa(t.val, num_val);
    printf("TOKEN:: TYPE:%s VAL:%s\n", s_token_types[t.type],
           t.type == NUMBER ? num_val : s_ops[t.val]);
}

#define LARGEST_POSSIBLE 2048
sequence_generator *new_sequence_generator(int num_wurds,
                                           char wurds[][SIZE_OF_WURD])
{
    printf("NUM WURDS %d\n", num_wurds);
    char *pattern_static[LARGEST_POSSIBLE] = {0};
    char *pattern = (char *)pattern_static;
    for (int i = 0; i < num_wurds; i++)
    {
        printf("WURDDDDY! %s\n", wurds[i]);
        strncat(pattern, wurds[i], SIZE_OF_WURD);
        strcat(pattern, " ");
    }
    printf("PATTERN! is %s\n", pattern);
    token tokenized_pattern[MAX_TOKENS_IN_PATTERN];
    int err = parse_tokens_from_wurds(num_wurds, wurds, tokenized_pattern);
    if (!err)
        for (int i = 0; i < MAX_TOKENS_IN_PATTERN; i++)
            print_token(tokenized_pattern[i]);

    // if (!isvalid_pattern(
    // char *pattern = "t * ( ( t >> 9 | t >> 13 ) & 25 & t >> 6)";
    if (strlen(pattern) > 1023)
    {
        printf("Max pattern size of 1024\n");
        return NULL;
    }
    sequence_generator *sg = calloc(1, sizeof(sequence_generator));
    if (!sg)
    {
        printf("WOOF!\n");
        return NULL;
    }
    strncpy(sg->pattern, pattern, 1023);
    sg->status = &sequence_generator_status;
    sg->generate = &sequence_generator_generate;

    sg->rpn_stack = new_rpn_stack(pattern);

    return sg;
}
void sequence_generator_change_pattern(sequence_generator *sg, char *pattern)
{
    // TODO
}

void sequence_generator_status(void *self, wchar_t *wstring)
{
    sequence_generator *sg = (sequence_generator *)self;
    swprintf(wstring, MAX_PS_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "SEQUENCE GEN ] - " WCOOL_COLOR_PINK
             "pattern: %s",
             sg->pattern);
}

int sequence_generator_generate(void *self)
{
    // sequence_generator *sg = (sequence_generator*) self;
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
        return LEFTBRACKET;
        break;
    case (')'):
        return RIGHTBRACKET;
        break;
    case ('t'):
        return TEE;
        break;
    default:
        return NUM_OPS;
    }
}

static bool isshift_char(char ch)
{
    if (ch == '<' || ch == '>')
        return true;
    return false;
}

int parse_tokens_from_wurds(int argc, char argv[][SIZE_OF_WURD],
                            token *tokenized_pattern)
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
                char multichar_num[10] = {0};
                int idx = 0;
                while (isdigit(*(c + 1)))
                    multichar_num[idx++] = *c++;
                multichar_num[idx++] = *c;

                token t = {NUMBER, atoi(multichar_num)};
                if (tokenized_pattern_idx < MAX_TOKENS_IN_PATTERN)
                    tokenized_pattern[tokenized_pattern_idx++] = t;
                else
                    return 1;
            }
            else if (isshift_char(*c))
            {
                char shifty = *c;
                c++;
                if (!isshift_char(*c) || *c != shifty)
                {
                    printf("TOOOOO SHIFTY barfed on mismatching %c and %c!\n",
                           shifty, *c);
                    return 1;
                }

                token t;
                t.type = OPERATOR;
                if (shifty == '<')
                    t.val = LEFTSHIFT;
                else
                    t.val = RIGHTSHIFT;
                tokenized_pattern[tokenized_pattern_idx++] = t;
            }
            else if (isvalid_char(c))
            {
                unsigned int op = which_op(*c);
                token t;
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
    return 0;
}
