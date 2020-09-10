#pragma once

#include <string>

namespace token
{
using TokenType = std::string;

// General Programming Language constructs///////
// ///////

const TokenType SLANG_ILLEGAL = "ILLEGAL";
const TokenType SLANG_EOFF = "EOF";

const TokenType SLANG_IDENT = "IDENT";
const TokenType SLANG_NUMBER = "NUMBER";
const TokenType SLANG_STRING = "STRING";

const TokenType SLANG_ASSIGN = "=";
const TokenType SLANG_PLUS = "+";
const TokenType SLANG_MINUS = "-";
const TokenType SLANG_BANG = "!";
const TokenType SLANG_ASTERISK = "*";
const TokenType SLANG_SLASH = "/";
const TokenType SLANG_MODULO = "%";

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
const TokenType SLANG_GENERATOR = "GENERATOR";
const TokenType SLANG_GENERATOR_SETUP = "GENERATOR_SETUP";
const TokenType SLANG_GENERATOR_RUN = "GENERATOR_RUN";
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

// Soundb0ard specific //////////////////
// /////////////////////////

const TokenType SLANG_BPM = "BPM";
const TokenType SLANG_EVERY = "EVERY";
const TokenType SLANG_WHILE = "WHILE";
const TokenType SLANG_OVER = "OVER";
const TokenType SLANG_RAMP = "RAMP";
const TokenType SLANG_OSC = "OSCILLATE";
const TokenType SLANG_PS = "PS";
const TokenType SLANG_LS = "LS";
const TokenType SLANG_HELP = "HELP";
const TokenType SLANG_SET = "SET";
const TokenType SLANG_PAN = "PAN";
const TokenType SLANG_PLAY = "PLAY";
const TokenType SLANG_PITCH = "PITCH";
const TokenType SLANG_VOLUME = "VOLUME";
const TokenType SLANG_INFO = "INFO";

// instruments
const TokenType SLANG_SAMPLE = "SAMPLE";
const TokenType SLANG_GRANULAR = "GRANULAR";
const TokenType SLANG_GRAIN = "GRAIN";
const TokenType SLANG_LOOP = "LOOP";
const TokenType SLANG_FM_SYNTH = "FM_SYNTH";
const TokenType SLANG_MOOG_SYNTH = "MOOG_SYNTH";
const TokenType SLANG_DRUM_SYNTH = "DRUM_SYNTH";
const TokenType SLANG_DIGI_SYNTH = "DIGI_SYNTH";

const TokenType SLANG_PROC = "PROC";
const TokenType SLANG_PROC_ID = "PROC_ID";
// Process pattern types
const TokenType SLANG_DOLLAR = "$"; // environment vars in pattern
const TokenType SLANG_HASH = "#";   // values in pattern, targets follow

const TokenType SLANG_PIPE = "|"; // separtes pattern functions from pattern

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
