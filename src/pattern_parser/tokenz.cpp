#include <iostream>
#include <string>
#include <unordered_map>

#include <pattern_parser/tokenz.hpp>

namespace pattern_parser
{

std::ostream &operator<<(std::ostream &out, const pattern_parser::Token &tok)
{
    out << "{type:" << tok.type_ << " literal:" << tok.literal_ << "}";
    return out;
}

} // namespace pattern_parser
