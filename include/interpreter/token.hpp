#pragma once

#include <string>

namespace token
{
using TokenType = std::string;

const TokenType ILLEGAL = "ILLEGAL";
const TokenType EOFF = "EOF";

const TokenType IDENT = "IDENT";
const TokenType INT = "INT";
const TokenType STRING = "STRING";

const TokenType ASSIGN = "=";
const TokenType PLUS = "+";
const TokenType MINUS = "-";
const TokenType BANG = "!";
const TokenType ASTERISK = "*";
const TokenType SLASH = "/";

const TokenType LT = "<";
const TokenType GT = ">";

const TokenType COMMA = ",";
const TokenType COLON = ":";
const TokenType SEMICOLON = ";";

const TokenType LPAREN = "(";
const TokenType RPAREN = ")";
const TokenType LBRACE = "{";
const TokenType RBRACE = "}";
const TokenType LBRACKET = "[";
const TokenType RBRACKET = "]";

const TokenType FOR = "FOR";
const TokenType FUNCTION = "FUNCTION";
const TokenType LET = "LET";
const TokenType TRUE = "TRUE";
const TokenType FALSE = "FALSE";
const TokenType IF = "IF";
const TokenType ELSE = "ELSE";
const TokenType RETURN = "RETURN";

const TokenType EQ = "==";
const TokenType NOT_EQ = "!=";

const TokenType INCREMENT = "++";
const TokenType DECREMENT = "--";

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
