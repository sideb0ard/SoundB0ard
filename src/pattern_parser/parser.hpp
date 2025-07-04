#pragma once

#include <memory>
#include <pattern_parser/ast.hpp>
#include <pattern_parser/tokenizer.hpp>
#include <pattern_parser/tokenz.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace pattern_parser {

class Parser {
 public:
  explicit Parser(std::shared_ptr<pattern_parser::Tokenizer> tokenizer);

  std::shared_ptr<pattern_parser::PatternGroup> ParsePattern();
  std::shared_ptr<pattern_parser::PatternNode> ParsePatternNode();
  std::shared_ptr<pattern_parser::PatternNode> ParsePatternLeaf();
  std::shared_ptr<pattern_parser::PatternNode> ParsePatternMultiStep();
  std::shared_ptr<pattern_parser::PatternNode> ParsePatternGroup();

  bool CheckErrors();
  void ShowTokens();

 private:
  bool ExpectPeek(pattern_parser::TokenType t);
  bool CurTokenIs(pattern_parser::TokenType t) const;
  bool PeekTokenIs(pattern_parser::TokenType t) const;
  void PeekError(pattern_parser::TokenType t);

  void NextToken();

 private:
  std::shared_ptr<pattern_parser::Tokenizer> tokenizer_;
  pattern_parser::Token cur_token_;
  pattern_parser::Token peek_token_;

  std::vector<std::string> errors_;
};

}  // namespace pattern_parser
