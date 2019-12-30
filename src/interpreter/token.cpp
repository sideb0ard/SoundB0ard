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
    {"if", SLANG_IF},
    {"let", SLANG_LET},
    {"true", SLANG_TRUE},
    {"return", SLANG_RETURN},
    {"every", SLANG_EVERY},
    {"ps", SLANG_PS},
    {"ls", SLANG_LS},
    {"sample", SLANG_SAMPLE},
    {"proc", SLANG_PROC},
    {"rev", SLANG_REV},
    {"rotl", SLANG_ROTATE_LEFT},
    {"rotr", SLANG_ROTATE_RIGHT},
};

// const std::unordered_map<std::string, TokenType> eventtypes{
//    {"midi", SLANG_TIMING_MIDI_TICK},
//    {"thirtysecond", SLANG_TIMING_THIRTYSECOND},
//    {"twentyfourth", SLANG_TIMING_TWENTYFOURTH},
//    {"sixteenth", SLANG_TIMING_SIXTEENTH},
//    {"twelth", SLANG_TIMING_TWELTH},
//    {"eighth", SLANG_TIMING_EIGHTH},
//    {"sixth", SLANG_TIMING_SIXTH},
//    {"quarter", SLANG_TIMING_QUARTER},
//    {"third", SLANG_TIMING_THIRD},
//    {"bar", SLANG_TIMING_BAR},
//};

TokenType LookupIdent(std::string ident)
{
    std::unordered_map<std::string, TokenType>::const_iterator got =
        keywords.find(ident);
    if (got != keywords.end())
    {
        return got->second;
    }

    // got = eventtypes.find(ident);
    // if (got != eventtypes.end())
    //    return got->second;

    return SLANG_IDENT;
}

std::ostream &operator<<(std::ostream &out, const Token &tok)
{
    out << "{type:" << tok.type_ << " literal:" << tok.literal_ << "}";
    return out;
}

} // namespace token
