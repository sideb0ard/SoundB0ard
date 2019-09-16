#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"

namespace parser
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

const std::unordered_map<token::TokenType, Precedence> precedences{
    {token::EQ, Precedence::EQUALS},
    {token::NOT_EQ, Precedence::EQUALS},
    {token::LT, Precedence::LESSGREATER},
    {token::GT, Precedence::LESSGREATER},
    {token::PLUS, Precedence::SUM},
    {token::MINUS, Precedence::SUM},
    {token::SLASH, Precedence::PRODUCT},
    {token::ASTERISK, Precedence::PRODUCT},
    {token::LPAREN, Precedence::CALL},
    {token::LBRACKET, Precedence::INDEX}};

class Parser
{
  public:
    explicit Parser(std::shared_ptr<lexer::Lexer> lexer);

    std::shared_ptr<ast::Program> ParseProgram();
    bool CheckErrors();

  private:
    std::shared_ptr<ast::Statement> ParseStatement();
    std::shared_ptr<ast::LetStatement> ParseLetStatement();
    std::shared_ptr<ast::ReturnStatement> ParseReturnStatement();
    std::shared_ptr<ast::ForStatement> ParseForStatement();

    std::shared_ptr<ast::ExpressionStatement> ParseExpressionStatement();

    std::shared_ptr<ast::Expression> ParseExpression(Precedence p);
    std::shared_ptr<ast::Expression> ParseIdentifier();
    std::shared_ptr<ast::Expression> ParseIntegerLiteral();
    std::shared_ptr<ast::Expression> ParseBoolean();
    std::shared_ptr<ast::Expression> ParseForPrefixExpression();
    std::shared_ptr<ast::Expression> ParsePrefixExpression();
    std::shared_ptr<ast::Expression>
    ParseInfixExpression(std::shared_ptr<ast::Expression> left);
    std::shared_ptr<ast::Expression> ParseGroupedExpression();
    std::shared_ptr<ast::Expression> ParseIfExpression();

    std::shared_ptr<ast::Expression>
    ParseIndexExpression(std::shared_ptr<ast::Expression> left);

    std::shared_ptr<ast::Expression> ParseFunctionLiteral();
    std::shared_ptr<ast::Expression> ParseStringLiteral();
    std::shared_ptr<ast::Expression> ParseArrayLiteral();
    std::shared_ptr<ast::Expression> ParseHashLiteral();
    std::vector<std::shared_ptr<ast::Identifier>> ParseFunctionParameters();

    std::shared_ptr<ast::Expression>
    ParseCallExpression(std::shared_ptr<ast::Expression> funct);
    std::vector<std::shared_ptr<ast::Expression>>
    ParseExpressionList(token::TokenType end);

    std::shared_ptr<ast::BlockStatement> ParseBlockStatement();

    bool ExpectPeek(token::TokenType t);
    bool CurTokenIs(token::TokenType t) const;
    bool PeekTokenIs(token::TokenType t) const;

    void PeekError(token::TokenType t);

    Precedence PeekPrecedence() const;
    Precedence CurPrecedence() const;
    void NextToken();

  private:
    std::shared_ptr<lexer::Lexer> lexer_;
    token::Token cur_token_;
    token::Token peek_token_;

    std::vector<std::string> errors_;
};

} // namespace parser
