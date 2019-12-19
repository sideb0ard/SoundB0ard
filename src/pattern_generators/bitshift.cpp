#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <range/v3/all.hpp>
#include <sstream>

#include "bitshift.h"
#include "defjams.h"
#include "mixer.h"
#include "pattern_generator.h"
#include "pattern_utils.h"
#include "utils.h"

extern mixer *mixr;

namespace
{

void print_symbols(std::vector<Symbol> syms)
{
    for (auto s : syms)
        std::cout << s << " ";
    std::cout << std::endl;
}

const std::vector<std::string> ops = {"<<", ">>", "^", "|", "~", "&",
                                      "+",  "-",  "*", "/", "%", "t"};
// bool is_number(const std::string &s)
//{
//    return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) {
//                             return !std::isdigit(c);
//                         }) == s.end();
//}
//
// bool is_operator(const std::string &s)
//{
//    if (std::find(ops.begin(), ops.end(), s) != ops.end())
//        return true;
//    return false;
//}
//
// bool is_parenthesis(const std::string &s, SymbolAssociativity sym)
//{
//    if ("(" == s && sym == SymbolAssociativity::LEFT)
//        return true;
//    else if (")" == s && sym == SymbolAssociativity::RIGHT)
//        return true;
//
//    return false;
//}

bool is_valid_char(char ch)
{
    if (isdigit(ch))
        return true;

    static char acceptable[] = {'<', '>', '^', '|', '~', '&', '+', '-',
                                '*', '/', '%', '(', ')', 't', 0};

    for (auto &c : acceptable)
    {
        if (ch == c)
            return true;
    }
    return false;
}

OperatorType which_op(char c)
{
    switch (c)
    {
    case ('<'):
        return OperatorType::LEFT_SHIFT;
        break;
    case ('>'):
        return OperatorType::RIGHT_SHIFT;
        break;
    case ('^'):
        return OperatorType::XOR;
        break;
    case ('|'):
        return OperatorType::OR;
        break;
    case ('~'):
        return OperatorType::NOT;
        break;
    case ('&'):
        return OperatorType::AND;
        break;
    case ('+'):
        return OperatorType::PLUS;
        break;
    case ('-'):
        return OperatorType::MINUS;
        break;
    case ('*'):
        return OperatorType::MULTIPLY;
        break;
    case ('/'):
        return OperatorType::DIVIDE;
        break;
    case ('%'):
        return OperatorType::MODULO;
        break;
    default:
        return OperatorType::UNUSED;
    }
}

} // namespace

Symbol::Symbol(SymbolType type, int value) : sym_type{type}, value{value} {}

Symbol::Symbol(SymbolType type, std::string identifier, OperatorType op_type)
    : sym_type{type}, op_type{op_type}, identifier{identifier}
{
    if (identifier == "<<")
    {
        precedence = 5;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == ">>")
    {
        precedence = 5;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == "^")
    {
        precedence = 9;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == "|")
    {
        precedence = 10;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == "~")
    {
        precedence = 2;
        associativity = SymbolAssociativity::RIGHT;
        ary = Aryness::UNARY;
    }
    else if (identifier == "&")
    {
        precedence = 8;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == "+")
    {
        precedence = 4;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == "-")
    {
        precedence = 4;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == "*")
    {
        precedence = 3;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == "/")
    {
        precedence = 3;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
    else if (identifier == "%")
    {
        precedence = 3;
        associativity = SymbolAssociativity::LEFT;
        ary = Aryness::BINARY;
    }
}

pattern_generator *new_bitshift(int num_wurds, char wurds[][SIZE_OF_WURD])
{

    bitshift *bs = (bitshift *)calloc(1, sizeof(bitshift));
    if (!bs)
    {
        printf("WOOF!\n");
        return NULL;
    }

    bitshift_pattern my_pattern = {};
    std::string line{};
    for (int i = 0; i < num_wurds; i++)
    {
        line.append(wurds[i]);
        if (i != num_wurds - 1)
            line.append(" ");
    }

    // std::cout << "LINE! " << line << std::endl;

    extract_symbols_from_line(line, bs->pattern.infix_tokenized_pattern);
    convert_to_infix(bs->pattern.infix_tokenized_pattern,
                     bs->pattern.rpn_tokenized_pattern);

    // print_symbols(my_pattern.infix_tokenized_pattern);
    // print_symbols(my_pattern.rpn_tokenized_pattern);

    bs->sg.status = &bitshift_status;
    bs->sg.generate = &bitshift_generate;
    bs->sg.event_notify = &bitshift_event_notify;
    bs->sg.set_debug = &bitshift_set_debug;
    bs->sg.type = BITSHIFT;
    // bs->time_counter = 40761023; // init value, rather than 0
    bs->time_counter =
        mixr->timing_info.cur_sample; // init value, rather than 0
    return (pattern_generator *)bs;
}
// static void bitshift_change_pattern(bitshift *sg, char *pattern)
//{
//    (void)sg;
//    (void)pattern;
//    // TODO
//}

void bitshift_status(void *self, wchar_t *wstring)
{
    bitshift *bs = (bitshift *)self;

    std::string infix_pattern =
        symbols_to_string(bs->pattern.infix_tokenized_pattern);

    std::string rpn_pattern =
        symbols_to_string(bs->pattern.rpn_tokenized_pattern);

    swprintf(wstring, MAX_STATIC_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "PATTERN GEN ] - " WCOOL_COLOR_PINK
             "infix pattern: %s\nrpn pattern: %s",
             infix_pattern.c_str(), rpn_pattern.c_str());
}

void bitshift_generate(void *self, void *data)
{
    bitshift *bs = (bitshift *)self;
    int t = bs->time_counter++;

    midi_event *midi_pattern = (midi_event *)data;
    std::vector<int> answer_vec = {};

    int len_rpn = bs->pattern.rpn_tokenized_pattern.size();
    for (int i = 0; i < len_rpn; i++)
    {
        Symbol s = bs->pattern.rpn_tokenized_pattern[i];

        if (s.sym_type == SymbolType::NUMBER)
        {
            if (bs->sg.debug)
                std::cout << "GOTSZ A NUMBER:" << s << std::endl;
            answer_vec.push_back(s.value);
        }
        else if (s.sym_type == SymbolType::TEE_TOKEN)
        {
            if (bs->sg.debug)
                std::cout << "GOTSZ A TEE:" << s << std::endl;
            answer_vec.push_back(t);
        }
        else if (s.sym_type == SymbolType::OP)
        {
            if (bs->sg.debug)
                std::cout << "GOTSZ An OP:" << s << std::endl;
            if (s.ary == Aryness::UNARY)
            {
                // only UNARY is bitwise NOT ~
                std::cout << "UNARY?! must be a ~" << std::endl;
                Symbol next_token = bs->pattern.rpn_tokenized_pattern[++i];
                if (next_token.sym_type != SymbolType::NUMBER ||
                    next_token.sym_type != SymbolType::TEE_TOKEN)
                {
                    std::cout << "WOAH, NELLIE, NOT A NUMBER!\n";
                    return;
                }
                int op1 = next_token.sym_type == SymbolType::NUMBER
                              ? next_token.value
                              : mixr->timing_info.cur_sample;

                answer_vec.push_back(~op1);
            }
            else
            {
                int op2 = answer_vec.back();
                answer_vec.pop_back();
                int op1 = answer_vec.back();
                answer_vec.pop_back();

                if (bs->sg.debug)
                    printf("OP1 is %d and OP2 is %d\n", op1, op2);

                int answer = 0;
                switch (s.op_type)
                {
                case (OperatorType::LEFT_SHIFT):
                    answer = op1 << op2;
                    break;
                case (OperatorType::RIGHT_SHIFT):
                    answer = op1 >> op2;
                    break;
                case (OperatorType::XOR):
                    answer = op1 ^ op2;
                    break;
                case (OperatorType::OR):
                    answer = op1 | op2;
                    break;
                case (OperatorType::AND):
                    answer = op1 & op2;
                    break;
                case (OperatorType::PLUS):
                    answer = op1 + op2;
                    break;
                case (OperatorType::MINUS):
                    answer = op1 - op2;
                    break;
                case (OperatorType::MULTIPLY):
                    answer = op1 * op2;
                    break;
                case (OperatorType::DIVIDE):
                    if (op2 != 0)
                        answer = op1 / op2;
                    break;
                case (OperatorType::MODULO):
                    if (op2 != 0)
                        answer = op1 % op2;
                    break;
                default:
                    std::cout << "WOOOOAH, NELLY! DONT KNOW WHAT OP YA GOT!\n";
                }
                if (bs->sg.debug)
                    std::cout << "Answer is " << answer << std::endl;
                answer_vec.push_back(answer);
            }
        }
    }
    apply_short_to_midi_pattern(answer_vec.back(), midi_pattern);
}

void bitshift_event_notify(void *self, broadcast_event event)
{
    (void)self;
    (void)event;
    // no-op
}

void bitshift_set_debug(void *self, bool b)
{
    bitshift *bs = (bitshift *)self;
    bs->sg.debug = b;
}
void bitshift_set_time_counter(bitshift *bs, int time)
{
    if (time >= 0)
        bs->time_counter = time;
}

bool bitshift_save(bitshift *bs, char *name)
{
    (void)bs;
    (void)name;
    return true;
    // TODO finish
}

std::ostream &operator<<(std::ostream &out, const Symbol &s)
{
    if (s.sym_type == SymbolType::NUMBER)
        out << s.value;
    else if (s.sym_type == SymbolType::OP)
        out << s.identifier;
    else if (s.sym_type == SymbolType::LEFT_PARENS)
        out << " ( ";
    else if (s.sym_type == SymbolType::RIGHT_PARENS)
        out << " ) ";
    else if (s.sym_type == SymbolType::TEE_TOKEN)
        out << " t ";
    return out;
}

void convert_to_infix(const std::vector<Symbol> &tokens,
                      std::vector<Symbol> &output_queue)
{
    std::vector<Symbol> operator_stack;

    for (const Symbol t : tokens)
    {
        std::cout << "\nLooking at " << t << std::endl;
        if (t.sym_type == SymbolType::NUMBER)
        {
            output_queue.push_back(t);
            std::cout << "Add token to output" << std::endl;
        }
        if (t.sym_type == SymbolType::TEE_TOKEN)
        {
            output_queue.push_back(t);
            std::cout << "Add token to output" << std::endl;
        }
        else if (t.sym_type == SymbolType::OP)
        {
            while (operator_stack.size() > 0)
            {
                Symbol top_of_stack = operator_stack.back();

                if (top_of_stack.precedence > t.precedence ||
                    (top_of_stack.precedence == t.precedence &&
                     top_of_stack.associativity == SymbolAssociativity::LEFT))
                {
                    if (top_of_stack.sym_type == SymbolType::LEFT_PARENS)
                        break;

                    output_queue.push_back(top_of_stack);
                    operator_stack.pop_back();

                    std::cout << "POP stack to output: " << top_of_stack
                              << std::endl;
                }
                else
                {
                    break;
                }
            }
            std::cout << "PUSH token to stack" << t << std::endl;
            operator_stack.push_back(t);
        }
        else if (t.sym_type == SymbolType::LEFT_PARENS)
        {
            std::cout << "Push token to stack" << std::endl;
            operator_stack.push_back(t);
        }
        else if (t.sym_type == SymbolType::RIGHT_PARENS)
        {
            bool left_parens_found = false;
            while (operator_stack.size() > 0 && !left_parens_found)
            {
                std::cout << "Pop stack" << std::endl;
                Symbol top_of_stack = operator_stack.back();
                operator_stack.pop_back();
                if (top_of_stack.sym_type == SymbolType::LEFT_PARENS)
                    left_parens_found = true;
                else
                    output_queue.push_back(top_of_stack);
            }
            if (!left_parens_found)
            {
                std::cout << "ERRR! doesn't compute!" << std::endl;
            }
        }
    }
    print_symbols(output_queue);
    print_symbols(operator_stack);
    while (!operator_stack.empty())
    {
        output_queue.emplace_back(operator_stack.back());
        operator_stack.pop_back();
    }
    print_symbols(output_queue);
    print_symbols(operator_stack);
}

void extract_symbols_from_line(const std::string &line,
                               std::vector<Symbol> &symbols)
{
    auto tokens = line | ranges::view::split(' ');
    for (const std::string &token : tokens)
    {
        std::cout << "looking at " << token << std::endl;
        int token_len = token.size();
        for (int i = 0; i < token_len; i++)
        {
            if (isdigit(token[i]))
            {
                std::cout << "digit:" << token[i] << std::endl;
                std::ostringstream num;
                while (i < (token_len - 1) && isdigit(token[i + 1]))
                    num << token[i++];
                num << token[i];
                symbols.emplace_back(
                    Symbol(SymbolType::NUMBER, std::stoi(num.str())));
            }
            else if (token[i] == '<' || token[i] == '>')
            {
                char shifty = token[i];
                i++;
                if (token[i] != shifty)
                {
                    std::cout << "Mismatched angle brackets! BARF!"
                              << std::endl;
                }
                std::cout << "SHIFT:" << token[i] << std::endl;
                if (shifty == '<')
                    symbols.emplace_back(
                        Symbol(SymbolType::OP, "<<", OperatorType::LEFT_SHIFT));
                else
                    symbols.emplace_back(Symbol(SymbolType::OP, ">>",
                                                OperatorType::RIGHT_SHIFT));
            }
            else if (token[i] == '(')
            {
                std::cout << "PARENS:" << token[i] << std::endl;
                symbols.emplace_back(
                    Symbol(SymbolType::LEFT_PARENS, "(", OperatorType::UNUSED));
            }
            else if (token[i] == ')')
            {
                std::cout << "PARENS:" << token[i] << std::endl;
                symbols.emplace_back(Symbol(SymbolType::RIGHT_PARENS, "(",
                                            OperatorType::UNUSED));
            }
            else if (is_valid_char(token[i]))
            {
                std::cout << "VALID CHAR:" << token[i] << std::endl;
                if (token[i] == 't')
                {
                    std::cout << "T:" << token[i] << std::endl;
                    symbols.emplace_back(Symbol(SymbolType::TEE_TOKEN, "t",
                                                OperatorType::UNUSED));
                }
                else
                {
                    std::cout << "OP!" << token[i] << std::endl;
                    OperatorType op_type = which_op(token[i]);
                    std::string ident{token[i]};
                    std::cout << "IDENTIFIER: " << ident << std::endl;
                    symbols.emplace_back(
                        Symbol(SymbolType::OP, ident, op_type));
                }
            }
            else
                std::cout << "Um, think your bitshift is biffed!" << std::endl;
        }
    }
}

std::string symbols_to_string(const std::vector<Symbol> symbols)
{
    std::ostringstream my_out;
    for (auto s : symbols)
        my_out << s << " ";
    return my_out.str();
}
