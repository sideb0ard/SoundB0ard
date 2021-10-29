#include <iostream>
#include <pattern_parser/tokenz.hpp>
#include <string>
#include <unordered_map>

namespace pattern_parser {

std::ostream &operator<<(std::ostream &out, const pattern_parser::Token &tok) {
  out << "{type:" << tok.type_ << " literal:" << tok.literal_ << "}";
  return out;
}

}  // namespace pattern_parser
