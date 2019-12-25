#include <pattern_parser/tokenizer.hpp>

#include <iostream>
#include <string>

namespace
{
bool IsValidIdentifier(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' ||
           c == '-' || c == '.';
}
bool IsDigit(char c) { return '0' <= c && c <= '9'; }

bool IsBalanced(std::string &input)
{
    // dumb algorithm counting matching number of curly braces.
    int num_braces = 0;
    const int len = input.length();
    for (int i = 0; i < len; i++)
    {
        if (input[i] == '{')
            num_braces++;
        else if (input[i] == '}')
            num_braces--;
    }

    return num_braces == 0;
}

} // namespace

namespace pattern_parser
{

Tokenizer::Tokenizer(std::string input) : input_{input} { ReadChar(); }

bool Tokenizer::ReadInput(std::string input)
{
    input_ += input;

    if (IsBalanced(input_))
    {
        ReadChar();
        return true;
    }
    return false;
}

void Tokenizer::Reset()
{
    input_.clear();
    current_char_ = 0;
    current_position_ = 0;
    next_position_ = 0;
}

std::string Tokenizer::ReadString()
{
    auto pos = current_position_ + 1;
    while (true)
    {
        ReadChar();
        if (current_char_ == '"' || current_char_ == 0)
            break;
    }
    return input_.substr(pos, current_position_ - pos);
}

void Tokenizer::ReadChar()
{
    const int len = input_.length();
    if (next_position_ >= len)
        current_char_ = 0;
    else
        current_char_ = input_[next_position_];

    current_position_ = next_position_;
    next_position_ += 1;
}

char Tokenizer::PeekChar()
{
    if (next_position_ >= static_cast<int>(input_.length()))
        return 0;
    else
        return input_[next_position_];
}

pattern_parser::Token Tokenizer::NextToken()
{

    pattern_parser::Token tok;

    SkipWhiteSpace();

    switch (current_char_)
    {
    case ('['):
        tok.type_ = pattern_parser::PATTERN_SQUARE_BRACKET_LEFT;
        tok.literal_ = current_char_;
        break;
    case (']'):
        tok.type_ = pattern_parser::PATTERN_SQUARE_BRACKET_RIGHT;
        tok.literal_ = current_char_;
        break;
    case ('*'):
        tok.type_ = pattern_parser::PATTERN_MULTIPLIER;
        tok.literal_ = current_char_;
        break;
    case ('/'):
        tok.type_ = pattern_parser::PATTERN_DIVISOR;
        tok.literal_ = current_char_;
        break;
    case (','):
        tok.type_ = pattern_parser::PATTERN_COMMA;
        tok.literal_ = current_char_;
        break;
    case ('('):
        tok.type_ = pattern_parser::PATTERN_OPEN_PAREN;
        tok.literal_ = current_char_;
        break;
    case (')'):
        tok.type_ = pattern_parser::PATTERN_CLOSE_PAREN;
        tok.literal_ = current_char_;
        break;
    case ('<'):
        tok.type_ = pattern_parser::PATTERN_OPEN_ANGLE_BRACKET;
        tok.literal_ = current_char_;
        break;
    case ('>'):
        tok.type_ = pattern_parser::PATTERN_CLOSE_ANGLE_BRACKET;
        tok.literal_ = current_char_;
        break;
    case (0):
        tok.type_ = pattern_parser::PATTERN_EOF;
        break;
    default:
        if (IsValidIdentifier(current_char_))
        {
            tok.literal_ = ReadIdentifier();
            tok.type_ = PATTERN_IDENT;
            return tok;
        }
        else if (IsDigit(current_char_))
        {
            tok.type_ = pattern_parser::PATTERN_INT;
            tok.literal_ = ReadNumber();
            return tok;
        }
        else
        {
            tok.type_ = pattern_parser::PATTERN_ILLEGAL;
            tok.literal_ = current_char_;
        }
    }

    ReadChar();
    return tok;
}

std::string Tokenizer::ReadIdentifier()
{
    int position = current_position_;
    while (IsValidIdentifier(current_char_))
        ReadChar();
    return input_.substr(position, current_position_ - position);
}

std::string Tokenizer::ReadNumber()
{
    int position = current_position_;
    while (IsDigit(current_char_))
        ReadChar();
    return input_.substr(position, current_position_ - position);
}

void Tokenizer::SkipWhiteSpace()
{
    while (current_char_ == ' ' || current_char_ == '\t' ||
           current_char_ == '\n' || current_char_ == '\r')
        ReadChar();
}

std::string Tokenizer::GetInput() { return input_; }

} // namespace pattern_parser
