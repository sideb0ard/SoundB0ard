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

PatternGroup::PatternGroup()
{
    std::cout << "Pattern Group CREATING A Vector!\n";
    std::vector<std::shared_ptr<pattern_parser::PatternNode>> first_vec;
    event_groups_.push_back(first_vec);
}

std::string PatternGroup::String() const
{
    std::stringstream ss;
    std::cout << "  PatternGroup String! num event groups"
              << event_groups_.size() << std::endl;
    for (auto &eg : event_groups_)
    {
        for (auto &e : eg)
        {
            ss << e->String() << " ";
        }
        ss << "\n";
    }

    if (divisor_value_)
        ss << "/" << divisor_value_;

    return ss.str();
}

} // namespace pattern_parser
