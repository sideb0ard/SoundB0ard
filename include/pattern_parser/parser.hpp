#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <pattern_parser/ast.hpp>
#include <pattern_parser/tokenizer.hpp>
#include <pattern_parser/tokenz.hpp>

namespace pattern_parser
{

enum class Precedence
{
    _,
    LOWEST,
    EQUALS,
    LESSGREATER,
    SUM,
    PRODUCT,
    PREFIX,
    CALL,
    INDEX
};

// const std::unordered_map<pattern_parser::TokenType, Precedence> precedences{
//    {pattern_parser::SLANG_EQ, Precedence::EQUALS},
//    {pattern_parser::SLANG_NOT_EQ, Precedence::EQUALS},
//    {pattern_parser::SLANG_LT, Precedence::LESSGREATER},
//    {pattern_parser::SLANG_GT, Precedence::LESSGREATER},
//    {pattern_parser::SLANG_PLUS, Precedence::SUM},
//    {pattern_parser::SLANG_MINUS, Precedence::SUM},
//    {pattern_parser::SLANG_SLASH, Precedence::PRODUCT},
//    {pattern_parser::SLANG_ASTERISK, Precedence::PRODUCT},
//    {pattern_parser::SLANG_LPAREN, Precedence::CALL},
//    {pattern_parser::PATTERN_SQUARE_BRACKET_LEFT, Precedence::INDEX}};

class Parser
{
  public:
    explicit Parser(std::shared_ptr<pattern_parser::Tokenizer> tokenizer);

    std::shared_ptr<pattern_parser::EventGroup> ParsePattern();
    std::shared_ptr<pattern_parser::PatternNode> ParsePatternNode();
    std::shared_ptr<pattern_parser::PatternNode> ParsePatternIdent();
    std::shared_ptr<pattern_parser::PatternNode> ParsePatternGroup();

    bool CheckErrors();
    void ShowTokens();

  private:
    // std::shared_ptr<ast::ExpressionStatement> ParseExpressionStatement();

    // std::shared_ptr<ast::Expression> ParseExpression(Precedence p);
    // std::shared_ptr<ast::Expression> ParseIdentifier();
    // std::shared_ptr<ast::Expression> ParseIntegerLiteral();

    bool ExpectPeek(pattern_parser::TokenType t);
    bool CurTokenIs(pattern_parser::TokenType t) const;
    bool PeekTokenIs(pattern_parser::TokenType t) const;

    void PeekError(pattern_parser::TokenType t);

    // Precedence PeekPrecedence() const;
    // Precedence CurPrecedence() const;
    void NextToken();

  private:
    std::shared_ptr<pattern_parser::Tokenizer> tokenizer_;
    pattern_parser::Token cur_token_;
    pattern_parser::Token peek_token_;

    std::vector<std::string> errors_;
};

} // namespace pattern_parser
