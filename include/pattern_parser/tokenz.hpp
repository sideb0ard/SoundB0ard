#pragma once

#include <string>

namespace pattern_parser
{
using TokenType = std::string;

const TokenType PATTERN_SQUARE_BRACKET_LEFT = "[";
const TokenType PATTERN_SQUARE_BRACKET_RIGHT = "]";
const TokenType PATTERN_MULTIPLIER = "*";
const TokenType PATTERN_DIVIDER = "/";
const TokenType PATTERN_COMMA = ",";
const TokenType PATTERN_IDENT = "PATTERN_IDENT";
const TokenType PATTERN_EOF = "EOF";
const TokenType PATTERN_GROUP = "GROUP";

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

} // namespace pattern_parser
