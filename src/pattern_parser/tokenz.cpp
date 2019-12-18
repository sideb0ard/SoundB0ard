#include <iostream>
#include <string>
#include <unordered_map>

#include <pattern_parser/tokenz.hpp>

namespace pattern_parser
{

const std::unordered_map<std::string, pattern_parser::TokenType> keywords{
    //    {"else", SLANG_ELSE},     {"false", SLANG_FALSE}, {"for", SLANG_FOR},
    //    {"fn", SLANG_FUNCTION},   {"fm", SLANG_FM_SYNTH}, {"if", SLANG_IF},
    //    {"let", SLANG_LET},       {"true", SLANG_TRUE},   {"return",
    //    SLANG_RETURN},
    //    {"every", SLANG_EVERY},   {"ps", SLANG_PS},       {"ls", SLANG_LS},
    //    {"sample", SLANG_SAMPLE}, {"proc", SLANG_PROC},
};

TokenType LookupIdent(std::string ident)
{
    std::unordered_map<std::string, TokenType>::const_iterator got =
        keywords.find(ident);
    if (got != keywords.end())
    {
        return got->second;
    }

    return ENV_IDENT;
}

std::ostream &operator<<(std::ostream &out, const pattern_parser::Token &tok)
{
    out << "{type:" << tok.type_ << " literal:" << tok.literal_ << "}";
    return out;
}

} // namespace pattern_parser
