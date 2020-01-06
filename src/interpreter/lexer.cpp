#include <interpreter/lexer.hpp>

#include <iostream>
#include <string>

namespace
{

bool IsDigit(char c) { return '0' <= c && c <= '9'; }

bool IsValidIdentifier(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' ||
           c == '/' || c == '-' || c == '.' || IsDigit(c);
}

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
            tok.type_ = token::SLANG_EQ;
            tok.literal_ = "==";
        }
        else
        {
            tok.type_ = token::SLANG_ASSIGN;
            tok.literal_ = current_char_;
        }
        break;
    case ('+'):
        if (PeekChar() == '+')
        {
            ReadChar();
            tok.type_ = token::SLANG_INCREMENT;
            tok.literal_ = "++";
        }
        else
        {
            tok.type_ = token::SLANG_PLUS;
            tok.literal_ = current_char_;
        }
        break;
    case ('-'):
        if (PeekChar() == '-')
        {
            ReadChar();
            tok.type_ = token::SLANG_DECREMENT;
            tok.literal_ = "--";
        }
        else
        {
            tok.type_ = token::SLANG_MINUS;
            tok.literal_ = current_char_;
        }
        break;
    case ('!'):
        if (PeekChar() == '=')
        {
            ReadChar();
            tok.type_ = token::SLANG_NOT_EQ;
            tok.literal_ = "!=";
        }
        else
        {
            tok.type_ = token::SLANG_BANG;
            tok.literal_ = current_char_;
        }
        break;
    case ('$'):
        tok.type_ = token::SLANG_DOLLAR;
        tok.literal_ = current_char_;
        break;
    case ('#'):
        tok.type_ = token::SLANG_HASH;
        tok.literal_ = current_char_;
        break;
    case ('*'):
        tok.type_ = token::SLANG_ASTERISK;
        tok.literal_ = current_char_;
        break;
    case ('/'):
        tok.type_ = token::SLANG_SLASH;
        tok.literal_ = current_char_;
        break;
    case ('<'):
        tok.type_ = token::SLANG_LT;
        tok.literal_ = current_char_;
        break;
    case ('>'):
        tok.type_ = token::SLANG_GT;
        tok.literal_ = current_char_;
        break;
    case (','):
        tok.type_ = token::SLANG_COMMA;
        tok.literal_ = current_char_;
        break;
    case (';'):
        tok.type_ = token::SLANG_SEMICOLON;
        tok.literal_ = current_char_;
        break;
    case ('('):
        tok.type_ = token::SLANG_LPAREN;
        tok.literal_ = current_char_;
        break;
    case (')'):
        tok.type_ = token::SLANG_RPAREN;
        tok.literal_ = current_char_;
        break;
    case ('{'):
        tok.type_ = token::SLANG_LBRACE;
        tok.literal_ = current_char_;
        break;
    case ('}'):
        tok.type_ = token::SLANG_RBRACE;
        tok.literal_ = current_char_;
        break;
    case ('['):
        tok.type_ = token::SLANG_LBRACKET;
        tok.literal_ = current_char_;
        break;
    case (']'):
        tok.type_ = token::SLANG_RBRACKET;
        tok.literal_ = current_char_;
        break;
    case (':'):
        tok.type_ = token::SLANG_COLON;
        tok.literal_ = current_char_;
        break;
    case ('|'):
        tok.type_ = token::SLANG_PIPE;
        tok.literal_ = current_char_;
        break;
    case ('"'):
        tok.type_ = token::SLANG_STRING;
        tok.literal_ = ReadString();
        break;
    case (0):
        tok.type_ = token::SLANG_EOFF;
        break;
    default:
        if (current_char_ == 'p' && IsDigit(PeekChar()))
        {
            tok.literal_ = ReadProcId();
            std::cout << "PROC IDzz!! " << tok.literal_ << "\n";
            tok.type_ = token::SLANG_PROC_ID;
            return tok;
        }
        if (IsValidIdentifier(current_char_) && !IsDigit(current_char_))
        {
            tok.literal_ = ReadIdentifier();
            tok.type_ = token::LookupIdent(tok.literal_);
            return tok;
        }
        else if (IsDigit(current_char_))
        {
            tok.type_ = token::SLANG_NUMBER;
            tok.literal_ = ReadNumber();
            return tok;
        }
        else
        {
            tok.type_ = token::SLANG_ILLEGAL;
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
    bool has_decimal_point{false};
    while (IsDigit(current_char_) || current_char_ == '.')
    {
        if (current_char_ == '.')
        {
            if (!has_decimal_point)
                has_decimal_point = true;
            else
                break; // eek, summit wrong, can only be a single place.
        }
        ReadChar();
    }
    return input_.substr(position, current_position_ - position);
}

std::string Lexer::ReadProcId()
{
    // discard current 'p'
    ReadChar();

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
