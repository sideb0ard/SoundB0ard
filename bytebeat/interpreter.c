#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bytebeat.h"
#include "mixer.h"
#include "stack.h"

extern mixer *mixr;

// order must match the ops enum in bytebeat_interpreter.h
char *ops[] = {"<<", ">>", "^", "|", "~", "&", "+",
               "-",  "*",  "/", "%", "(", ")", "t"};

//////////////////////////////////////////////////////////////////////////////

bool isnum(char *ch)
{
    if (*ch >= 48 && *ch <= 57)
        return true;

    return false;
}

bool isvalid_char(char *ch)
{
    if (isnum(ch))
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

bool isvalid_pattern(char *pattern)
{
    int pattern_len = strlen(pattern);

    for (int i = 0; i < pattern_len; i++)
    {
        // printf("%c (%d)\n", pattern[i], pattern[i]);
        if (pattern[i] == 0 || pattern[i] == 32) // EOF or space
            continue;
        if (!isvalid_char(&pattern[i]))
            return false;
    }

    return true;
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

int parse_bytebeat(char *pattern, Stack *rpn_stack)
{

    if (!isvalid_pattern(pattern))
    {
        printf("Beat it ya val jerk - acceptables chars are 0-9,"
               "<, >, |, ^, ~, +, -, /, *, &, %%\n");
        return EXIT_FAILURE;
    }

    Stack *operatorz = calloc(1, sizeof(List));
    stack_init(operatorz, NULL);

    // int pattern_len = strlen(pattern);

    char *sep = " ";
    char *wurd, *wurdleft;
    for (wurd = strtok_r(pattern, sep, &wurdleft); wurd;
         wurd = strtok_r(NULL, sep, &wurdleft))
    {
        token *toke = calloc(1, sizeof(token));
        // printf("\nToken is %s\n", wurd);
        int val = atoi(wurd);
        if (val != 0)
        {
            printf("NUM! %d\n", val);
            toke->type = NUMBER;
            toke->val = val;
            stack_push(rpn_stack, (void *)(toke));
        }
        else
        {
            printf("OP! %s\n", wurd);

            toke->type = OPERATOR;
            if (strcmp(wurd, "<<") == 0)
                toke->val = LEFTSHIFT;
            else if (strcmp(wurd, ">>") == 0)
                toke->val = RIGHTSHIFT;
            else if (strcmp(wurd, "^") == 0)
                toke->val = XOR;
            else if (strcmp(wurd, "|") == 0)
                toke->val = OR;
            else if (strcmp(wurd, "~") == 0)
                toke->val = NOT;
            else if (strcmp(wurd, "&") == 0)
                toke->val = AND;
            else if (strcmp(wurd, "+") == 0)
                toke->val = PLUS;
            else if (strcmp(wurd, "-") == 0)
                toke->val = MINUS;
            else if (strcmp(wurd, "*") == 0)
                toke->val = MULTIPLY;
            else if (strcmp(wurd, "/") == 0)
                toke->val = DIVIDE;
            else if (strcmp(wurd, "%") == 0)
                toke->val = MODULO;
            else if (strcmp(wurd, "(") == 0)
                toke->val = LEFTBRACKET;
            else if (strcmp(wurd, ")") == 0)
                toke->val = RIGHTBRACKET;
            else if (strcmp(wurd, "t") == 0)
                toke->val = TEE;
            else
            {
                printf("OOFT!\n");
                return 1;
            }
            if (toke->val == TEE)
            {
                toke->type = TEE_TOKEN;
                toke->val = TEE;
                stack_push(rpn_stack, (void *)(toke));
                continue;
            }
            if (toke->val == LEFTBRACKET)
            {
                stack_push(operatorz, (void *)(toke));
                continue;
            }
            if (toke->val == RIGHTBRACKET)
            {
                // printf("RIGHT BRACKET!\n");
                while (stack_size(operatorz) > 0)
                {
                    // printf("STACKSIZE %d\n", stack_size(operatorz));

                    token *top_o_the_stack = stack_peek(operatorz);

                    // printf("Looking at %s\n", ops[top_o_the_stack->val]);

                    if (top_o_the_stack->val == LEFTBRACKET)
                    {
                        // printf("WOOP - found bracjet!\n");
                        stack_pop(operatorz, (void **)&top_o_the_stack);
                    }
                    else
                    {
                        stack_pop(operatorz, (void **)&top_o_the_stack);
                        stack_push(rpn_stack, (void *)(top_o_the_stack));
                    }
                }
                continue;
            }

            if (stack_size(operatorz) > 0)
            {
                token *top_o_the_stack = stack_peek(operatorz);
                if (top_o_the_stack->val == LEFTBRACKET)
                {
                    stack_push(operatorz, (void *)(toke));
                    continue;
                }
                // printf("STACK SIZE GRETR THAN 0: %d\n",
                // stack_size(operatorz));
                bool wurk_it = true;
                while (wurk_it && stack_size(operatorz) > 0)
                {
                    token *top_o_the_stack = stack_peek(operatorz);
                    // printf(
                    //     "PRECEDENCE OF ME %d, precedence of top of stack
                    //     %d\n",
                    //     precedence(toke->val),
                    //     precedence(top_o_the_stack->val));
                    // printf("ASSOCIATIVITY OF ME %s, ASSOCIATIVITY of top of "
                    //        "stack %s\n",
                    //        associativity(toke->val) ? "RIGHT" : "LEFT",
                    //        associativity(top_o_the_stack->val) ? "RIGHT"
                    //                                            : "LEFT");
                    if (((associativity(toke->val) == LEFT) &&
                         (precedence(toke->val) <=
                          precedence(top_o_the_stack->val))) ||
                        ((associativity(toke->val) == RIGHT) &&
                         (precedence(toke->val) <
                          precedence(top_o_the_stack->val))))
                    {
                        // printf("POPing to PUSH\n");
                        if (stack_pop(operatorz, (void **)&top_o_the_stack) ==
                            0)
                        {
                            stack_push(rpn_stack, (void *)top_o_the_stack);
                        }
                        else
                        {
                            printf("Couldnae pop, mate\n");
                        }
                    }
                    else
                    {
                        wurk_it = false;
                        // printf("WURKIT False\n");
                    }
                }
            }
            stack_push(operatorz, (void *)(toke));
        }
        // printf("STACK_SIZE(operatorz): %d\n", stack_size(operatorz));
        // printf("STACK_SIZE(rpn_stack): %d\n", stack_size(rpn_stack));
    }

    token *re_token;

    while (stack_pop(operatorz, (void **)&re_token) == 0)
    {
        stack_push(rpn_stack, (void *)re_token);
    }

    stack_destroy(operatorz);

    return EXIT_SUCCESS;
}

int calc(int operator, int first_operand, int second_operand)
{
    switch (operator)
    {
    case 0: // left shift
        return first_operand << second_operand;
    case 1: // right shift
        return first_operand >> second_operand;
    case 2: // XOR
        return first_operand ^ second_operand;
    case 3: // bitwize OR
        return first_operand | second_operand;
    case 4: // bitwize NOT ~ // should never hit this as its Unary
        return 99;
    case 5: // bitwize AND
        return first_operand & second_operand;
    case 6: // PLUS
        return first_operand + second_operand;
    case 7: // MINUS
        return first_operand - second_operand;
    case 8: // MULTIPLY
        return first_operand * second_operand;
    case 9: // DIVIDE
        return first_operand / second_operand;
    case 10: // MODULO
        return first_operand % second_operand;
    default:
        return 99;
    }
}

int eval_token(token *re_token, Stack *ans_stack, token *tmp_tokens,
               int *tmp_token_num)
{
    if (re_token->type == NUMBER)
    {
        tmp_tokens[*tmp_token_num].type = NUMBER;
        tmp_tokens[*tmp_token_num].val = re_token->val;
        stack_push(ans_stack, (void *)&tmp_tokens[(*tmp_token_num)++]);
    }
    else if (re_token->type == TEE_TOKEN)
    {
        tmp_tokens[*tmp_token_num].type = NUMBER;
        tmp_tokens[*tmp_token_num].val = mixr->timing_info.midi_tick;
        stack_push(ans_stack, (void *)&tmp_tokens[(*tmp_token_num)++]);
    }
    else if (re_token->type == OPERATOR)
    {
        if (bin_or_uni(re_token->val) == UNARY)
        {
            token *first_operand;
            stack_pop(ans_stack, (void **)&first_operand);
            int ans = ~first_operand->val;
            first_operand->val = ans;
            stack_push(ans_stack, (void *)first_operand);
        }
        else
        { // BINARY operator
            if (stack_size(ans_stack) < 2)
            {
                printf("Barf, not enough operands on stack\n");
                return 1;
            }
            token *second_operand;
            stack_pop(ans_stack, (void **)&second_operand);

            token *first_operand;
            stack_pop(ans_stack, (void **)&first_operand);

            char ans =
                calc(re_token->val, first_operand->val, second_operand->val);

            tmp_tokens[*tmp_token_num].type = NUMBER;
            tmp_tokens[*tmp_token_num].val = ans;
            stack_push(ans_stack, (void *)&tmp_tokens[(*tmp_token_num)++]);
        }
    }
    else
    {
        printf("Wowo, nelly!\n");
        return -1;
    }
    return 0;
}

char parse_rpn(Stack *rpn_stack)
{
    token tmp_tokens[30];
    memset(tmp_tokens, 0, sizeof(tmp_tokens));
    int tmp_token_num = 0;

    Stack ans_stack;
    stack_init(&ans_stack, NULL);

    ListElmt *listee = rpn_stack->head;
    while (listee->next != NULL)
    {
        token *re_token = listee->data;
        eval_token(re_token, &ans_stack, tmp_tokens, &tmp_token_num);
        listee = listee->next;
    }
    token *re_token = listee->data;
    eval_token(re_token, &ans_stack, tmp_tokens, &tmp_token_num);

    char ret_val = -1;

    if (stack_size(&ans_stack) == 1)
    {
        token *atoke;
        stack_pop(&ans_stack, (void **)&atoke);
        ret_val = atoke->val;
    }
    else
    {
        printf("Nah man, too many items on stack, sumthing is fucked\n");
    }

    return ret_val;
}

void reverse_stack(Stack *stack)
{
    // printf("\n=============\n");
    Stack *rev_stack = calloc(1, sizeof(List));
    stack_init(rev_stack, NULL);

    token *tokey;
    while (stack_size(stack) > 0)
    {
        stack_pop(stack, (void **)&tokey);
        stack_push(rev_stack, (void *)tokey);
    }

    Stack tmp_stack = *stack;
    *stack = *rev_stack;
    stack_destroy(&tmp_stack);
}

Stack *new_rpn_stack(char *apattern)
{
    char pattern[128];
    strncpy(pattern, apattern, 127);

    Stack *rpn_stack = calloc(1, sizeof(List));
    stack_init(rpn_stack, NULL);

    parse_bytebeat(pattern, rpn_stack);
    reverse_stack(rpn_stack);

    return rpn_stack;
}

char interpreter(Stack *rpn_stack)
{
    char ans = parse_rpn(rpn_stack);
    return ans;
}
