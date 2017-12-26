#include <stdlib.h>
#include <string.h>

#include "bitshift.h"
#include "defjams.h"
#include "sequence_generator.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
static char *s_token_types[] = {"NUMBER", "OPERATOR", "TEE_TOKEN"};
static char *s_ops[] = {"<<", ">>", "^", "|", "~", "&", "+",
                        "-",  "*",  "/", "%", "(", ")", "t"};

void tokenized_pattern_to_string(bitshift_pattern *pattern, char *pattern_as_string, int stringlen)
{
    memset(pattern_as_string, 0, stringlen);
    int string_idx = 0;
    for (int i = 0; i < pattern->num_tokens; i++)
    {
        bitshift_token t = pattern->tokenized_pattern[i];

        char char_val[100] = {0};
        if (t.type == NUMBER)
            itoa(t.val, char_val);
        else if (t.type == TEE_TOKEN)
            itoa(mixr->timing_info.cur_sample, char_val);
        else
            strcpy(char_val, s_ops[t.val]);
        if (i != pattern->num_tokens - 1)
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
sequence_generator *new_bitshift(int num_wurds,
                                 char wurds[][SIZE_OF_WURD])
{
    bitshift_pattern my_pattern;
    int err = parse_bitshift_tokens_from_wurds(num_wurds, wurds, my_pattern.tokenized_pattern, &my_pattern.num_tokens);
    if (err)
    {
        printf("ERR!! %d\n", err);
        return NULL;
    }

    bitshift *bs = calloc(1, sizeof(bitshift));
    if (!bs)
    {
        printf("WOOF!\n");
        return NULL;
    }
    bs->pattern = my_pattern;
    bs->sg.status = &bitshift_status;
    bs->sg.generate = &bitshift_generate;

    //sg->rpn_stack = new_rpn_stack(pattern);

    return (sequence_generator*) bs;
}
void bitshift_change_pattern(bitshift *sg, char *pattern)
{
    // TODO
}

void bitshift_status(void *self, wchar_t *wstring)
{
    bitshift *bs = (bitshift *)self;
    char a_pattern[MAX_PS_STRING_SZ];
    tokenized_pattern_to_string(&bs->pattern, a_pattern, MAX_PS_STRING_SZ);
    swprintf(wstring, MAX_PS_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "SEQUENCE GEN ] - " WCOOL_COLOR_PINK
             "pattern: %s",
             a_pattern);
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
                            bitshift_token *tokenized_pattern, int *num_tokens)
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

                bitshift_token t;
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
