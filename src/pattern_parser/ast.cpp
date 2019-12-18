#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <pattern_parser/ast.hpp>

namespace pattern_parser
{

std::string Identifier::String() const { return value_; }
std::string IntegerLiteral::String() const { return token_.literal_; }

std::string EventGroup::String() const
{
    std::stringstream ss;
    for (auto &s : events_)
        ss << s->String();

    return ss.str();
}

} // namespace pattern_parser
