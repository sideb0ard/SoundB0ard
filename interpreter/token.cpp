#include <iostream>
#include <string>
#include <unordered_map>

#include "token.hpp"

namespace token
{
const std::unordered_map<std::string, TokenType> keywords{
    {"else", ELSE}, {"false", FALSE}, {"for", FOR},   {"fn", FUNCTION},
    {"if", IF},     {"let", LET},     {"true", TRUE}, {"return", RETURN}};

TokenType LookupIdent(std::string ident)
{
    std::unordered_map<std::string, TokenType>::const_iterator got =
        keywords.find(ident);
    if (got != keywords.end())
        return got->second;
    return IDENT;
}

std::ostream &operator<<(std::ostream &out, const Token &tok)
{
    out << "{type:" << tok.type_ << " literal:" << tok.literal_ << "}";
    return out;
}

} // namespace token
