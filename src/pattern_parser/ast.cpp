#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <pattern_parser/ast.hpp>

namespace pattern_parser
{

std::string Identifier::String() const
{

    std::stringstream ss;
    ss << value_;
    ss << MOD_NAMES[modifier_] << modifier_value_;
    return ss.str();
}

std::string IntegerLiteral::String() const
{

    std::stringstream ss;
    ss << token_.literal_;
    ss << MOD_NAMES[modifier_] << modifier_value_;
    return ss.str();
}

std::string EventGroup::String() const
{
    std::stringstream ss;
    for (auto &s : events_)
        ss << s->String();

    ss << MOD_NAMES[modifier_] << modifier_value_;

    return ss.str();
}

} // namespace pattern_parser
