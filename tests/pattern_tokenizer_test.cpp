#include <iostream>
#include <memory>
#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenizer.hpp>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

namespace {

struct PatternTokenizerTest : public ::testing::Test {
  void SetUp(void) { std::cout << "Setup!\n"; }
};

TEST_F(PatternTokenizerTest, TestTokenz) {
  std::string pattern{"[bd, sd]/2*()<>"};
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  pattern_parser::Token tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_SQUARE_BRACKET_LEFT);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_IDENT);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_COMMA);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_IDENT);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_SQUARE_BRACKET_RIGHT);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_DIVISOR);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_NUMBER);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_MULTIPLIER);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_OPEN_PAREN);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_CLOSE_PAREN);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_OPEN_ANGLE_BRACKET);

  tok = tokenizer->NextToken();
  EXPECT_EQ(tok.type_, pattern_parser::PATTERN_CLOSE_ANGLE_BRACKET);
}

}  // namespace
