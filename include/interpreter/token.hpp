#pragma once

#include <string>

namespace token
{
using TokenType = std::string;

const TokenType SLANG_ILLEGAL = "ILLEGAL";
const TokenType SLANG_EOFF = "EOF";

const TokenType SLANG_IDENT = "IDENT";
const TokenType SLANG_INT = "INT";
const TokenType SLANG_STRING = "STRING";

const TokenType SLANG_ASSIGN = "=";
const TokenType SLANG_PLUS = "+";
const TokenType SLANG_MINUS = "-";
const TokenType SLANG_BANG = "!";
const TokenType SLANG_ASTERISK = "*";
const TokenType SLANG_SLASH = "/";
const TokenType SLANG_PIPE = "|";

const TokenType SLANG_LT = "<";
const TokenType SLANG_GT = ">";

const TokenType SLANG_COMMA = ",";
const TokenType SLANG_COLON = ":";
const TokenType SLANG_SEMICOLON = ";";

const TokenType SLANG_LPAREN = "(";
const TokenType SLANG_RPAREN = ")";
const TokenType SLANG_LBRACE = "{";
const TokenType SLANG_RBRACE = "}";
const TokenType SLANG_LBRACKET = "[";
const TokenType SLANG_RBRACKET = "]";

const TokenType SLANG_FOR = "FOR";
const TokenType SLANG_FUNCTION = "FUNCTION";
const TokenType SLANG_LET = "LET";
const TokenType SLANG_TRUE = "TRUE";
const TokenType SLANG_FALSE = "FALSE";
const TokenType SLANG_IF = "IF";
const TokenType SLANG_ELSE = "ELSE";
const TokenType SLANG_RETURN = "RETURN";

const TokenType SLANG_EQ = "==";
const TokenType SLANG_NOT_EQ = "!=";

const TokenType SLANG_INCREMENT = "++";
const TokenType SLANG_DECREMENT = "--";

const TokenType SLANG_FM_SYNTH = "FM_SYNTH";
const TokenType SLANG_EVERY = "EVERY";
const TokenType SLANG_PS = "PS";
const TokenType SLANG_LS = "LS";
const TokenType SLANG_SAMPLE = "SAMPLE";
const TokenType SLANG_PROC = "PROC";
const TokenType SLANG_PROC_ID = "PROC_ID";
const TokenType SLANG_DOLLAR = "$";

const TokenType SLANG_TIMING_MIDI_TICK = "MIDI TICK";
const TokenType SLANG_TIMING_THIRTYSECOND = "THIRTYSECOND";
const TokenType SLANG_TIMING_TWENTYFOURTH = "TWENTYFOURTH";
const TokenType SLANG_TIMING_SIXTEENTH = "SIXTEENTH";
const TokenType SLANG_TIMING_TWELTH = "TWELTH";
const TokenType SLANG_TIMING_EIGHTH = "EIGHTH";
const TokenType SLANG_TIMING_SIXTH = "SIXTH";
const TokenType SLANG_TIMING_QUARTER = "QUARTER";
const TokenType SLANG_TIMING_THIRD = "THIRD";
const TokenType SLANG_TIMING_BAR = "BAR";

// pattern functions
const TokenType SLANG_REV = "REVERSE";
const TokenType SLANG_ROTATE_LEFT = "ROTATE LEFT";
const TokenType SLANG_ROTATE_RIGHT = "ROTATE RIGHT";

class Token
{
  public:
    Token() {}
    Token(TokenType type, std::string literal)
        : type_{type}, literal_{literal} {};

    friend std::ostream &operator<<(std::ostream &, const Token &);

  public:
    TokenType type_;
    std::string literal_;
};

TokenType LookupIdent(std::string ident);

} // namespace token
