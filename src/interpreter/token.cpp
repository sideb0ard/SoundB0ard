#include <iostream>
#include <string>
#include <unordered_map>

#include <interpreter/token.hpp>

namespace token
{
const std::unordered_map<std::string, TokenType> keywords{
    {"bpm", SLANG_BPM},         {"else", SLANG_ELSE},
    {"every", SLANG_EVERY},     {"false", SLANG_FALSE},
    {"fm", SLANG_FM_SYNTH},     {"fn", SLANG_FUNCTION},
    {"for", SLANG_FOR},         {"granular", SLANG_GRANULAR},
    {"help", SLANG_HELP},       {"grain", SLANG_GRAIN},
    {"loop", SLANG_LOOP},       {"if", SLANG_IF},
    {"let", SLANG_LET},         {"ls", SLANG_LS},
    {"moog", SLANG_MOOG_SYNTH}, {"osc", SLANG_OSC},
    {"over", SLANG_OVER},       {"pan", SLANG_PAN},
    {"play", SLANG_PLAY},       {"proc", SLANG_PROC},
    {"ps", SLANG_PS},           {"ramp", SLANG_RAMP},
    {"return", SLANG_RETURN},   {"sample", SLANG_SAMPLE},
    {"set", SLANG_SET},         {"true", SLANG_TRUE},
    {"vol", SLANG_VOLUME},      {"while", SLANG_WHILE},
    {"info", SLANG_INFO},
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
