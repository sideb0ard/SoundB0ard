#include <iostream>
#include <string>
#include <unordered_map>

#include <interpreter/token.hpp>

namespace token
{
const std::unordered_map<std::string, TokenType> keywords{
    {"else", SLANG_ELSE},
    {"false", SLANG_FALSE},
    {"for", SLANG_FOR},
    {"fn", SLANG_FUNCTION},
    {"fm", SLANG_FM_SYNTH},
    {"moog", SLANG_MOOG_SYNTH},
    {"sample", SLANG_SAMPLE},
    {"granular", SLANG_GRANULAR},
    {"if", SLANG_IF},
    {"let", SLANG_LET},
    {"true", SLANG_TRUE},
    {"return", SLANG_RETURN},
    {"every", SLANG_EVERY},
    {"over", SLANG_OVER},
    {"set", SLANG_SET},
    {"play", SLANG_PLAY},
    {"ps", SLANG_PS},
    {"ls", SLANG_LS},
    {"proc", SLANG_PROC},
    {"rev", SLANG_REV},
    {"rotl", SLANG_ROTATE_LEFT},
    {"rotr", SLANG_ROTATE_RIGHT},
};

TokenType LookupIdent(std::string ident)
{
    std::unordered_map<std::string, TokenType>::const_iterator got =
        keywords.find(ident);
    if (got != keywords.end())
    {
        return got->second;
    }

    return SLANG_IDENT;
}

std::ostream &operator<<(std::ostream &out, const Token &tok)
{
    out << "{type:" << tok.type_ << " literal:" << tok.literal_ << "}";
    return out;
}

} // namespace token
