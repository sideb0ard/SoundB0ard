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

int Identifier::NumEvents() const { return 1; }

std::string EventGroup::String() const
{
    std::stringstream ss;
    std::cout << "  EventGroup String! num events" << events_.size()
              << std::endl;
    for (auto &s : events_)
        ss << s->String() << " ";

    return ss.str();
}

int EventGroup::NumEvents() const { return events_.size(); }

} // namespace pattern_parser
