#include <iostream>
#include <string>
#include <unordered_map>

#include <pattern_parser/tokenz.hpp>

namespace pattern_parser
{

const std::unordered_map<std::string, pattern_parser::TokenType> keywords{
    {"*", PATTERN_MULTIPLIER},
    {"/", PATTERN_DIVIDER},
    {"[", PATTERN_SQUARE_BRACKET_LEFT},
    {"]", PATTERN_SQUARE_BRACKET_RIGHT},
};

TokenType LookupIdent(std::string ident)
{
    std::unordered_map<std::string, TokenType>::const_iterator got =
        keywords.find(ident);
    if (got != keywords.end())
    {
        return got->second;
    }

    return PATTERN_IDENT;
}

std::ostream &operator<<(std::ostream &out, const pattern_parser::Token &tok)
{
    out << "{type:" << tok.type_ << " literal:" << tok.literal_ << "}";
    return out;
}

} // namespace pattern_parser
