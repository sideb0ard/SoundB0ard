#include <iostream>
#include <numeric>
#include <pattern_parser/ast.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace pattern_parser {

std::string PatternLeaf::String() const {
  std::stringstream ss;
  ss << value_;
  if (divisor_value_) ss << "/" << divisor_value_;
  if (euclidean_hits_ && euclidean_steps_)
    ss << "(" << euclidean_hits_ << "," << euclidean_steps_ << ")";
  return ss.str();
}

std::string PatternMultiStep::String() const {
  std::stringstream ss;
  for (auto &v : values_) ss << v->String();

  if (divisor_value_) ss << "/" << divisor_value_;

  return ss.str();
}

PatternGroup::PatternGroup() {
  std::vector<std::shared_ptr<pattern_parser::PatternNode>> first_vec;
  event_groups_.push_back(first_vec);
}

std::string PatternGroup::String() const {
  std::stringstream ss;
  int num_event_groups{0};
  for (auto &eg : event_groups_) {
    ss << "Event Group:" << ++num_event_groups << " -- ";
    for (auto &e : eg) {
      ss << e->String() << " ";
    }
    ss << "\n";
  }

  if (divisor_value_) ss << "/" << divisor_value_;

  return ss.str();
}

}  // namespace pattern_parser
