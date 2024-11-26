#pragma once

#include <interpreter/ast.hpp>
#include <interpreter/lexer.hpp>
#include <interpreter/token.hpp>
#include <memory>
#include <process.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace parser {

enum class Precedence {
  _,
  LOWEST,
  LOGICALANDOR,
  BITWISEOR,
  BITWISEXOR,
  BITWISEAND,
  EQUALS,
  LESSGREATER,
  BITSHIFT,
  SUM,
  PRODUCT,
  PREFIX,
  POSTFIX,
  CALL,
  INDEX
};

const std::unordered_map<token::TokenType, Precedence> precedences{
    {token::SLANG_BITWISE_AND, Precedence::BITWISEAND},
    {token::SLANG_BITWISE_OR, Precedence::BITWISEOR},
    {token::SLANG_BITWISE_XOR, Precedence::BITWISEXOR},
    {token::SLANG_BITWISE_LEFTSHIFT, Precedence::BITSHIFT},
    {token::SLANG_BITWISE_RIGHTSHIFT, Precedence::BITSHIFT},
    {token::SLANG_AND, Precedence::LOGICALANDOR},
    {token::SLANG_OR, Precedence::LOGICALANDOR},
    {token::SLANG_EQ, Precedence::EQUALS},
    {token::SLANG_NOT_EQ, Precedence::EQUALS},
    {token::SLANG_LT, Precedence::LESSGREATER},
    {token::SLANG_GT, Precedence::LESSGREATER},
    {token::SLANG_LT_OR_EQ, Precedence::LESSGREATER},
    {token::SLANG_GT_OR_EQ, Precedence::LESSGREATER},
    {token::SLANG_PLUS, Precedence::SUM},
    {token::SLANG_MINUS, Precedence::SUM},
    {token::SLANG_SLASH, Precedence::PRODUCT},
    {token::SLANG_MODULO, Precedence::PRODUCT},
    {token::SLANG_ASTERISK, Precedence::PRODUCT},
    {token::SLANG_LPAREN, Precedence::CALL},
    {token::SLANG_LBRACKET, Precedence::INDEX},
    {token::SLANG_INCREMENT, Precedence::POSTFIX},
    {token::SLANG_DECREMENT, Precedence::POSTFIX},
};

class Parser {
 public:
  explicit Parser(std::shared_ptr<lexer::Lexer> lexer);

  std::shared_ptr<ast::Program> ParseProgram();
  bool CheckErrors();
  void ShowTokens();

 private:
  std::shared_ptr<ast::Statement> ParseStatement();
  std::shared_ptr<ast::BreakStatement> ParseBreakStatement();
  std::shared_ptr<ast::LetStatement> ParseLetStatement();
  std::shared_ptr<ast::ReturnStatement> ParseReturnStatement();
  std::shared_ptr<ast::ForStatement> ParseForStatement();
  std::shared_ptr<ast::Statement> ParseIfStatement();
  std::shared_ptr<ast::HelpStatement> ParseHelpStatement();
  std::shared_ptr<ast::LsStatement> ParseLsStatement();
  std::shared_ptr<ast::StrategyStatement> ParseStrategyStatement();
  std::shared_ptr<ast::InfoStatement> ParseInfoStatement();
  std::shared_ptr<ast::BpmStatement> ParseBpmStatement();
  std::shared_ptr<ast::PsStatement> ParsePsStatement();
  std::shared_ptr<ast::Statement> ParseSetStatement();
  // bool ParseSetStatementValue(std::string &value_result);
  std::shared_ptr<ast::PanStatement> ParsePanStatement();
  std::shared_ptr<ast::PlayStatement> ParsePlayStatement();
  std::shared_ptr<ast::Statement> ParseProcessStatement();
  std::shared_ptr<ast::Statement> ParseProcessSetStatement();
  void ConsumePatternFunctions(std::shared_ptr<ast::ProcessStatement> proc);
  std::shared_ptr<ast::VolumeStatement> ParseVolumeStatement();

  std::shared_ptr<ast::Statement> ParseExpressionStatement();

  std::shared_ptr<ast::BitOpExpression> ParseBitOpExpression();
  std::shared_ptr<ast::Expression> ParseExpression(Precedence p);
  std::shared_ptr<ast::Expression> ParseIdentifier();
  std::shared_ptr<ast::Expression> ParseNumberLiteral();
  std::shared_ptr<ast::Expression> ParseBoolean();
  std::shared_ptr<ast::Expression> ParseForPrefixExpression();
  std::shared_ptr<ast::Expression> ParsePrefixExpression();
  std::shared_ptr<ast::Expression> ParseInfixExpression(
      std::shared_ptr<ast::Expression> left);
  std::shared_ptr<ast::Expression> ParsePostfixExpression(
      std::shared_ptr<ast::Expression> left);
  std::shared_ptr<ast::Expression> ParseGroupedExpression();
  std::shared_ptr<ast::Expression> ParseEveryExpression();
  std::shared_ptr<ast::Expression> ParseAtExpression();
  std::shared_ptr<ast::Expression> ParseDurationExpression();
  std::shared_ptr<ast::Expression> ParseVelocityExpression();

  std::shared_ptr<ast::Expression> ParseIndexExpression(
      std::shared_ptr<ast::Expression> left);

  std::shared_ptr<ast::Expression> ParseFunctionLiteral();
  std::shared_ptr<ast::Expression> ParseGeneratorLiteral();
  std::shared_ptr<ast::Expression> ParsePhasorLiteral();
  std::shared_ptr<ast::Expression> ParseStringLiteral();
  std::shared_ptr<ast::Expression> ParseArrayLiteral();
  std::shared_ptr<ast::Expression> ParseHashLiteral();
  std::vector<std::shared_ptr<ast::Identifier>> ParseCallParameters();

  std::shared_ptr<ast::Expression> ParseMidiArrayExpression();
  std::shared_ptr<ast::Expression> ParsePatternExpression();
  std::shared_ptr<ast::Expression> ParseSynthExpression();
  std::shared_ptr<ast::Expression> ParseStepSequencerExpression();
  std::shared_ptr<ast::Expression> ParseSampleExpression();
  std::shared_ptr<ast::Expression> ParseGranularExpression();
  ast::TimingEventType ParseTimingEventLiteral();

  std::shared_ptr<ast::Expression> ParseCallExpression(
      std::shared_ptr<ast::Expression> funct);
  std::vector<std::shared_ptr<ast::Expression>> ParseExpressionList(
      token::TokenType end);

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

}  // namespace parser
