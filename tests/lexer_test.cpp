#include "interpreter/lexer.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "interpreter/token.hpp"

namespace {

struct LexerTest : public ::testing::Test {
  std::unique_ptr<lexer::Lexer> lex;

  std::string input = R"(let five = 5;
let ten = 10;

let add = fn(x, y) {
    x + y;
};

let result = add(five, ten);
!-/*5;
5 < 10 > 5;

if (5 < 10) {
    return true;
} else {
    return false;
}

10 == 10;
10 != 9;
"foobar"
"foo bar"
[1, 2];
{"foo": "bar"}
++3;
--3;
for (i = 0; i < 10; ++i) {};
ls;
ls kicks;
ps;
)";

  std::vector<std::pair<token::TokenType, std::string>> testTokens = {
      {token::SLANG_LET, "let"},        {token::SLANG_IDENT, "five"},
      {token::SLANG_ASSIGN, "="},       {token::SLANG_NUMBER, "5"},
      {token::SLANG_SEMICOLON, ";"},    {token::SLANG_LET, "let"},
      {token::SLANG_IDENT, "ten"},      {token::SLANG_ASSIGN, "="},
      {token::SLANG_NUMBER, "10"},      {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_LET, "let"},        {token::SLANG_IDENT, "add"},
      {token::SLANG_ASSIGN, "="},       {token::SLANG_FUNCTION, "fn"},
      {token::SLANG_LPAREN, "("},       {token::SLANG_IDENT, "x"},
      {token::SLANG_COMMA, ","},        {token::SLANG_IDENT, "y"},
      {token::SLANG_RPAREN, ")"},       {token::SLANG_LBRACE, "{"},
      {token::SLANG_IDENT, "x"},        {token::SLANG_PLUS, "+"},
      {token::SLANG_IDENT, "y"},        {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_RBRACE, "}"},       {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_LET, "let"},        {token::SLANG_IDENT, "result"},
      {token::SLANG_ASSIGN, "="},       {token::SLANG_IDENT, "add"},
      {token::SLANG_LPAREN, "("},       {token::SLANG_IDENT, "five"},
      {token::SLANG_COMMA, ","},        {token::SLANG_IDENT, "ten"},
      {token::SLANG_RPAREN, ")"},       {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_BANG, "!"},         {token::SLANG_MINUS, "-"},
      {token::SLANG_SLASH, "/"},        {token::SLANG_ASTERISK, "*"},
      {token::SLANG_NUMBER, "5"},       {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_NUMBER, "5"},       {token::SLANG_LT, "<"},
      {token::SLANG_NUMBER, "10"},      {token::SLANG_GT, ">"},
      {token::SLANG_NUMBER, "5"},       {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_IF, "if"},          {token::SLANG_LPAREN, "("},
      {token::SLANG_NUMBER, "5"},       {token::SLANG_LT, "<"},
      {token::SLANG_NUMBER, "10"},      {token::SLANG_RPAREN, ")"},
      {token::SLANG_LBRACE, "{"},       {token::SLANG_RETURN, "return"},
      {token::SLANG_TRUE, "true"},      {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_RBRACE, "}"},       {token::SLANG_ELSE, "else"},
      {token::SLANG_LBRACE, "{"},       {token::SLANG_RETURN, "return"},
      {token::SLANG_FALSE, "false"},    {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_RBRACE, "}"},       {token::SLANG_NUMBER, "10"},
      {token::SLANG_EQ, "=="},          {token::SLANG_NUMBER, "10"},
      {token::SLANG_SEMICOLON, ";"},    {token::SLANG_NUMBER, "10"},
      {token::SLANG_NOT_EQ, "!="},      {token::SLANG_NUMBER, "9"},
      {token::SLANG_SEMICOLON, ";"},    {token::SLANG_STRING, "foobar"},
      {token::SLANG_STRING, "foo bar"}, {token::SLANG_LBRACKET, "["},
      {token::SLANG_NUMBER, "1"},       {token::SLANG_COMMA, ","},
      {token::SLANG_NUMBER, "2"},       {token::SLANG_RBRACKET, "]"},
      {token::SLANG_SEMICOLON, ";"},    {token::SLANG_LBRACE, "{"},
      {token::SLANG_STRING, "foo"},     {token::SLANG_COLON, ":"},
      {token::SLANG_STRING, "bar"},     {token::SLANG_RBRACE, "}"},
      {token::SLANG_INCREMENT, "++"},   {token::SLANG_NUMBER, "3"},
      {token::SLANG_SEMICOLON, ";"},    {token::SLANG_DECREMENT, "--"},
      {token::SLANG_NUMBER, "3"},       {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_FOR, "for"},        {token::SLANG_LPAREN, "("},
      {token::SLANG_IDENT, "i"},        {token::SLANG_ASSIGN, "="},
      {token::SLANG_NUMBER, "0"},       {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_IDENT, "i"},        {token::SLANG_LT, "<"},
      {token::SLANG_NUMBER, "10"},      {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_INCREMENT, "++"},   {token::SLANG_IDENT, "i"},
      {token::SLANG_RPAREN, ")"},       {token::SLANG_LBRACE, "{"},
      {token::SLANG_RBRACE, "}"},       {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_LS, "ls"},          {token::SLANG_SEMICOLON, ";"},
      {token::SLANG_LS, "ls"},          {token::SLANG_IDENT, "kicks"},
      {token::SLANG_SEMICOLON, ";"},    {token::SLANG_PS, "ps"},
      {token::SLANG_SEMICOLON, ";"},    {token::SLANG_EOFF, ""},
  };

  LexerTest() {
    std::cout << "Parsey setup!\n";
    lex = std::make_unique<lexer::Lexer>(input);
  }
};  // namespace

TEST_F(LexerTest, TestNextToken) {
  for (auto tt : testTokens) {
    token::Token tok = lex->NextToken();
    EXPECT_EQ(tok.type_, tt.first);
    EXPECT_EQ(tok.literal_, tt.second);
  }
}

}  // namespace
