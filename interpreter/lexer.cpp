#include "lexer.hpp"
#include <string>

namespace
{
bool IsValidIdentifier(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
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

namespace lexer
{

Lexer::Lexer(std::string input) : input_{input} { ReadChar(); }

bool Lexer::ReadInput(std::string input)
{
    input_ += input;

    if (IsBalanced(input_))
    {
        ReadChar();
        return true;
    }
    return false;
}

void Lexer::Reset()
{
    input_.clear();
    current_char_ = 0;
    current_position_ = 0;
    next_position_ = 0;
}

std::string Lexer::ReadString()
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

void Lexer::ReadChar()
{
    const int len = input_.length();
    if (next_position_ >= len)
        current_char_ = 0;
    else
        current_char_ = input_[next_position_];

    current_position_ = next_position_;
    next_position_ += 1;
}

char Lexer::PeekChar()
{
    if (next_position_ >= static_cast<int>(input_.length()))
        return 0;
    else
        return input_[next_position_];
}

token::Token Lexer::NextToken()
{

    token::Token tok;

    SkipWhiteSpace();

    switch (current_char_)
    {
    case ('='):
        if (PeekChar() == '=')
        {
            ReadChar();
            tok.type_ = token::EQ;
            tok.literal_ = "==";
        }
        else
        {
            tok.type_ = token::ASSIGN;
            tok.literal_ = current_char_;
        }
        break;
    case ('+'):
        if (PeekChar() == '+')
        {
            ReadChar();
            tok.type_ = token::INCREMENT;
            tok.literal_ = "++";
        }
        else
        {
            tok.type_ = token::PLUS;
            tok.literal_ = current_char_;
        }
        break;
    case ('-'):
        if (PeekChar() == '-')
        {
            ReadChar();
            tok.type_ = token::DECREMENT;
            tok.literal_ = "--";
        }
        else
        {
            tok.type_ = token::MINUS;
            tok.literal_ = current_char_;
        }
        break;
    case ('!'):
        if (PeekChar() == '=')
        {
            ReadChar();
            tok.type_ = token::NOT_EQ;
            tok.literal_ = "!=";
        }
        else
        {
            tok.type_ = token::BANG;
            tok.literal_ = current_char_;
        }
        break;
    case ('*'):
        tok.type_ = token::ASTERISK;
        tok.literal_ = current_char_;
        break;
    case ('/'):
        tok.type_ = token::SLASH;
        tok.literal_ = current_char_;
        break;
    case ('<'):
        tok.type_ = token::LT;
        tok.literal_ = current_char_;
        break;
    case ('>'):
        tok.type_ = token::GT;
        tok.literal_ = current_char_;
        break;
    case (','):
        tok.type_ = token::COMMA;
        tok.literal_ = current_char_;
        break;
    case (';'):
        tok.type_ = token::SEMICOLON;
        tok.literal_ = current_char_;
        break;
    case ('('):
        tok.type_ = token::LPAREN;
        tok.literal_ = current_char_;
        break;
    case (')'):
        tok.type_ = token::RPAREN;
        tok.literal_ = current_char_;
        break;
    case ('{'):
        tok.type_ = token::LBRACE;
        tok.literal_ = current_char_;
        break;
    case ('}'):
        tok.type_ = token::RBRACE;
        tok.literal_ = current_char_;
        break;
    case ('['):
        tok.type_ = token::LBRACKET;
        tok.literal_ = current_char_;
        break;
    case (']'):
        tok.type_ = token::RBRACKET;
        tok.literal_ = current_char_;
        break;
    case (':'):
        tok.type_ = token::COLON;
        tok.literal_ = current_char_;
        break;
    case ('"'):
        tok.type_ = token::STRING;
        tok.literal_ = ReadString();
        break;
    case (0):
        tok.type_ = token::EOFF;
        break;
    default:
        if (IsValidIdentifier(current_char_))
        {
            tok.literal_ = ReadIdentifier();
            tok.type_ = token::LookupIdent(tok.literal_);
            return tok;
        }
        else if (IsDigit(current_char_))
        {
            tok.type_ = token::INT;
            tok.literal_ = ReadNumber();
            return tok;
        }
        else
        {
            tok.type_ = token::ILLEGAL;
            tok.literal_ = current_char_;
        }
    }

    ReadChar();
    return tok;
}

std::string Lexer::ReadIdentifier()
{
    int position = current_position_;
    while (IsValidIdentifier(current_char_))
        ReadChar();
    return input_.substr(position, current_position_ - position);
}

std::string Lexer::ReadNumber()
{
    int position = current_position_;
    while (IsDigit(current_char_))
        ReadChar();
    return input_.substr(position, current_position_ - position);
}

void Lexer::SkipWhiteSpace()
{
    while (current_char_ == ' ' || current_char_ == '\t' ||
           current_char_ == '\n' || current_char_ == '\r')
        ReadChar();
}

std::string Lexer::GetInput() { return input_; }

} // namespace lexer
