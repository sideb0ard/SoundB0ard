#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <interpreter/ast.hpp>
#include <interpreter/lexer.hpp>
#include <interpreter/token.hpp>
#include <process.hpp>

namespace parser
{

enum class Precedence
{
    _,
    LOWEST,
    LOGICALANDOR,
    EQUALS,
    LESSGREATER,
    SUM,
    PRODUCT,
    PREFIX,
    CALL,
    INDEX
};

const std::unordered_map<token::TokenType, Precedence> precedences{
    {token::SLANG_AND, Precedence::LOGICALANDOR},
    {token::SLANG_OR, Precedence::LOGICALANDOR},
    {token::SLANG_EQ, Precedence::EQUALS},
    {token::SLANG_NOT_EQ, Precedence::EQUALS},
    {token::SLANG_LT, Precedence::LESSGREATER},
    {token::SLANG_GT, Precedence::LESSGREATER},
    {token::SLANG_PLUS, Precedence::SUM},
    {token::SLANG_MINUS, Precedence::SUM},
    {token::SLANG_SLASH, Precedence::PRODUCT},
    {token::SLANG_MODULO, Precedence::PRODUCT},
    {token::SLANG_ASTERISK, Precedence::PRODUCT},
    {token::SLANG_LPAREN, Precedence::CALL},
    {token::SLANG_LBRACKET, Precedence::INDEX}};

class Parser
{
  public:
    explicit Parser(std::shared_ptr<lexer::Lexer> lexer);

    std::shared_ptr<ast::Program> ParseProgram();
    bool CheckErrors();
    void ShowTokens();

  private:
    std::shared_ptr<ast::Statement> ParseStatement();
    std::shared_ptr<ast::LetStatement> ParseLetStatement();
    std::shared_ptr<ast::ReturnStatement> ParseReturnStatement();
    std::shared_ptr<ast::ForStatement> ParseForStatement();
    std::shared_ptr<ast::HelpStatement> ParseHelpStatement();
    std::shared_ptr<ast::LsStatement> ParseLsStatement();
    std::shared_ptr<ast::InfoStatement> ParseInfoStatement();
    std::shared_ptr<ast::BpmStatement> ParseBpmStatement();
    std::shared_ptr<ast::PsStatement> ParsePsStatement();
    std::shared_ptr<ast::SetStatement> ParseSetStatement();
    // bool ParseSetStatementValue(std::string &value_result);
    std::shared_ptr<ast::PanStatement> ParsePanStatement();
    std::shared_ptr<ast::PlayStatement> ParsePlayStatement();
    std::shared_ptr<ast::PitchStatement> ParsePitchStatement();
    std::shared_ptr<ast::ProcessStatement> ParseProcessStatement();
    void ConsumePatternFunctions(std::shared_ptr<ast::ProcessStatement> proc);
    std::shared_ptr<ast::VolumeStatement> ParseVolumeStatement();

    std::shared_ptr<ast::Statement> ParseExpressionStatement();

    std::shared_ptr<ast::Expression> ParseExpression(Precedence p);
    std::shared_ptr<ast::Expression> ParseIdentifier();
    std::shared_ptr<ast::Expression> ParseNumberLiteral();
    std::shared_ptr<ast::Expression> ParseBoolean();
    std::shared_ptr<ast::Expression> ParseForPrefixExpression();
    std::shared_ptr<ast::Expression> ParsePrefixExpression();
    std::shared_ptr<ast::Expression>
    ParseInfixExpression(std::shared_ptr<ast::Expression> left);
    std::shared_ptr<ast::Expression> ParseGroupedExpression();
    std::shared_ptr<ast::Expression> ParseIfExpression();
    std::shared_ptr<ast::Expression> ParseEveryExpression();
    std::shared_ptr<ast::Expression> ParseDurationExpression();
    std::shared_ptr<ast::Expression> ParseVelocityExpression();

    std::shared_ptr<ast::Expression>
    ParseIndexExpression(std::shared_ptr<ast::Expression> left);

    std::shared_ptr<ast::Expression> ParseFunctionLiteral();
    std::shared_ptr<ast::Expression> ParseGeneratorLiteral();
    std::shared_ptr<ast::Expression> ParseStringLiteral();
    std::shared_ptr<ast::Expression> ParseArrayLiteral();
    std::shared_ptr<ast::Expression> ParseHashLiteral();
    std::vector<std::shared_ptr<ast::Identifier>> ParseFunctionParameters();

    std::shared_ptr<ast::Expression> ParseSynthExpression();
    std::shared_ptr<ast::Expression> ParseSampleExpression();
    std::shared_ptr<ast::Expression> ParseGranularExpression();
    ast::TimingEventType ParseTimingEventLiteral();

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
    bool PeekTokenIsPatternCommandTimerType();

  private:
    std::shared_ptr<lexer::Lexer> lexer_;
    token::Token cur_token_;
    token::Token peek_token_;

    std::vector<std::string> errors_;
};

} // namespace parser
