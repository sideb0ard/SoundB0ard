#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <pattern_parser/ast.hpp>

namespace pattern_parser
{

std::string PatternLeaf::String() const
{

    std::stringstream ss;
    ss << value_;
    if (divisor_value_)
        ss << "/" << divisor_value_;
    return ss.str();
}

int PatternLeaf::NumEvents() const { return 1; }

std::string PatternGroup::String() const
{
    std::stringstream ss;
    std::cout << "  PatternGroup String! num events" << events_.size()
              << std::endl;
    for (auto &s : events_)
        ss << s->String() << " ";

    return ss.str();
}

int PatternGroup::NumEvents() const { return events_.size(); }

} // namespace pattern_parser
