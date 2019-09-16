#include <optional>

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "parser.hpp"

namespace parser
{

Parser::Parser(std::shared_ptr<lexer::Lexer> lexer) : lexer_{lexer}
{
    NextToken();
    NextToken();
}

std::shared_ptr<ast::Program> Parser::ParseProgram()
{
    std::shared_ptr<ast::Program> program = std::make_shared<ast::Program>();

    while (cur_token_.type_ != token::EOFF)
    {
        std::shared_ptr<ast::Statement> stmt = ParseStatement();
        if (stmt)
            program->statements_.push_back(stmt);
        NextToken();
    }
    return program;
}

std::shared_ptr<ast::Statement> Parser::ParseStatement()
{
    if (cur_token_.type_.compare(token::LET) == 0)
        return ParseLetStatement();
    else if (cur_token_.type_.compare(token::RETURN) == 0)
        return ParseReturnStatement();
    else if (cur_token_.type_.compare(token::FOR) == 0)
    {
        std::cout << "Found FOR statement!\n";
        return ParseForStatement();
    }
    else
        return ParseExpressionStatement();
}

std::shared_ptr<ast::LetStatement> Parser::ParseLetStatement()
{

    std::shared_ptr<ast::LetStatement> stmt =
        std::make_shared<ast::LetStatement>(cur_token_);

    if (!ExpectPeek(token::IDENT))
    {
        std::cout << "NO IDENT! - returning nullopt \n";
        return nullptr;
    }

    stmt->name_ =
        std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);

    if (!ExpectPeek(token::ASSIGN))
    {
        std::cout << "NO ASSIGN! - returning nullopt \n";
        return nullptr;
    }

    NextToken();

    stmt->value_ = ParseExpression(Precedence::LOWEST);

    if (PeekTokenIs(token::SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::ReturnStatement> Parser::ParseReturnStatement()
{

    std::shared_ptr<ast::ReturnStatement> stmt =
        std::make_shared<ast::ReturnStatement>(cur_token_);

    NextToken();

    stmt->return_value_ = ParseExpression(Precedence::LOWEST);

    if (PeekTokenIs(token::SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::ForStatement> Parser::ParseForStatement()
{
    std::shared_ptr<ast::ForStatement> stmt =
        std::make_shared<ast::ForStatement>(cur_token_);

    // Starting condition //////////

    if (!ExpectPeek(token::LPAREN))
    {
        std::cout << "NO LPAREN! - returning nullptr \n";
        return nullptr;
    }

    if (!ExpectPeek(token::IDENT))
    {
        std::cout << "NO IDENT! - returning nullptr \n";
        return nullptr;
    }

    std::cout << "CUR TOKEN LITERAL is " << cur_token_.literal_ << std::endl;

    stmt->iterator_ =
        std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);

    std::cout << "My iterator Identifier is " << stmt->iterator_->String()
              << std::endl;

    std::cout << "All good so far - CUR TOKEN is " << cur_token_.literal_
              << std::endl;

    if (!ExpectPeek(token::ASSIGN))
    {
        std::cout << "NO ASSIGN! - returning nullptr \n";
        return nullptr;
    }
    NextToken();
    std::cout << "All good so far - CUR TOKEN is " << cur_token_.literal_
              << std::endl;

    stmt->iterator_value_ = ParseExpression(Precedence::LOWEST);

    if (!ExpectPeek(token::SEMICOLON))
    {
        std::cout << "NO SEMICOLON! - returning nullptr \n";
        return nullptr;
    }
    NextToken();

    std::cout << "All good so far - IDENT val is "
              << stmt->iterator_value_->String() << std::endl;

    std::cout << "SOFARLYSOGOOD - looking for TERMINATION\n";
    // Termination Condition /////////////
    std::cout << "Looking for TERMINATION CONDITION - CUR TOKEN is "
              << cur_token_.literal_ << std::endl;
    stmt->termination_condition_ = ParseExpression(Precedence::LOWEST);

    std::cout << "GOT TERMINATION CONDITION - "
              << stmt->termination_condition_->String() << ". CUR TOKEN is "
              << cur_token_.literal_ << " Now looking for INCREMENT exression"
              << std::endl;

    NextToken();
    NextToken();
    std::cout << " __ POSITION __ cur_token:" << cur_token_.literal_
              << " Peek_token:" << peek_token_.literal_ << std::endl;

    // Increment Expression
    stmt->increment_ = ParseExpression(Precedence::LOWEST);
    std::cout << "All good so far - INCR is " << stmt->increment_->String()
              << ". CUR TOKEN is " << cur_token_.literal_ << std::endl;

    // Body
    if (!ExpectPeek(token::RPAREN))
        return nullptr;
    NextToken();

    std::cout << "pARSE BLOCK! "
              << ". CUR TOKEN is " << cur_token_.literal_ << std::endl;

    stmt->body_ = ParseBlockStatement();

    return stmt;
}

std::shared_ptr<ast::ExpressionStatement> Parser::ParseExpressionStatement()
{
    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::make_shared<ast::ExpressionStatement>(cur_token_);
    stmt->expression_ = ParseExpression(Precedence::LOWEST);

    if (PeekTokenIs(token::SEMICOLON))
        NextToken();

    return stmt;
}

bool Parser::CheckErrors()
{
    if (errors_.empty())
        return false;
    std::cout << "\n======================\nParser had " << errors_.size()
              << " errors\n";
    for (auto e : errors_)
        std::cout << e << std::endl;
    std::cout << "=======================\n";
    return true;
}

bool Parser::ExpectPeek(token::TokenType t)
{
    if (PeekTokenIs(t))
    {
        NextToken();
        return true;
    }
    else
    {
        PeekError(t);
        return false;
    }
}

void Parser::NextToken()
{
    cur_token_ = peek_token_;
    peek_token_ = lexer_->NextToken();
}

bool Parser::CurTokenIs(token::TokenType t) const
{
    if (cur_token_.type_.compare(t) == 0)
        return true;
    return false;
}

bool Parser::PeekTokenIs(token::TokenType t) const
{
    if (peek_token_.type_.compare(t) == 0)
        return true;
    return false;
}

void Parser::PeekError(token::TokenType t)
{
    std::stringstream msg;
    msg << "Expected next token to be " << t << ", got " << peek_token_.type_
        << " instead";
    errors_.push_back(msg.str());
}

static bool IsInfixOperator(token::TokenType type)
{
    if (type == token::PLUS || type == token::MINUS || type == token::SLASH ||
        type == token::ASTERISK || type == token::EQ || type == token::NOT_EQ ||
        type == token::LT || type == token::GT || type == token::LPAREN ||
        type == token::LBRACKET)
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////

std::shared_ptr<ast::Expression> Parser::ParseExpression(Precedence p)
{
    // these are the 'nuds' (null detontations) in the Vaughan Pratt paper 'top
    // down operator precedence'.
    std::shared_ptr<ast::Expression> left_expr = ParseForPrefixExpression();

    if (!left_expr)
        return nullptr;

    while (!PeekTokenIs(token::SEMICOLON) && p < PeekPrecedence())
    {
        if (IsInfixOperator(peek_token_.type_))
        {
            NextToken();
            if (cur_token_.type_ == token::LPAREN)
                left_expr = ParseCallExpression(left_expr);
            else if (cur_token_.type_ == token::LBRACKET)
                left_expr = ParseIndexExpression(left_expr);
            else // these are the 'leds' (left denotation)
                left_expr = ParseInfixExpression(left_expr);
        }
        else
        {
            return left_expr;
        }
    }
    return left_expr;
}

std::shared_ptr<ast::Expression> Parser::ParseForPrefixExpression()
{
    if (cur_token_.type_ == token::IDENT)
        return ParseIdentifier();
    else if (cur_token_.type_ == token::INT)
        return ParseIntegerLiteral();
    else if (cur_token_.type_ == token::INCREMENT)
        return ParsePrefixExpression();
    else if (cur_token_.type_ == token::DECREMENT)
        return ParsePrefixExpression();
    else if (cur_token_.type_ == token::BANG)
        return ParsePrefixExpression();
    else if (cur_token_.type_ == token::MINUS)
        return ParsePrefixExpression();
    else if (cur_token_.type_ == token::TRUE)
        return ParseBoolean();
    else if (cur_token_.type_ == token::FALSE)
        return ParseBoolean();
    else if (cur_token_.type_ == token::LPAREN)
        return ParseGroupedExpression();
    else if (cur_token_.type_ == token::IF)
        return ParseIfExpression();
    else if (cur_token_.type_ == token::FUNCTION)
        return ParseFunctionLiteral();
    else if (cur_token_.type_ == token::STRING)
        return ParseStringLiteral();
    else if (cur_token_.type_ == token::LBRACKET)
        return ParseArrayLiteral();
    else if (cur_token_.type_ == token::LBRACE)
        return ParseHashLiteral();

    std::cout << "No Prefix parser for " << cur_token_.type_ << std::endl;
    return nullptr;
}

std::shared_ptr<ast::Expression> Parser::ParseIdentifier()
{
    return std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);
}

std::shared_ptr<ast::Expression> Parser::ParseBoolean()
{
    return std::make_shared<ast::BooleanExpression>(cur_token_,
                                                    CurTokenIs(token::TRUE));
}

std::shared_ptr<ast::Expression> Parser::ParseArrayLiteral()
{
    auto array_lit = std::make_shared<ast::ArrayLiteral>(cur_token_);
    array_lit->elements_ = ParseExpressionList(token::RBRACKET);
    return array_lit;
}

std::shared_ptr<ast::Expression> Parser::ParseHashLiteral()
{
    auto hash_lit = std::make_shared<ast::HashLiteral>(cur_token_);

    while (!PeekTokenIs(token::RBRACE))
    {
        NextToken();
        std::shared_ptr<ast::Expression> key =
            ParseExpression(Precedence::LOWEST);

        if (!ExpectPeek(token::COLON))
            return nullptr;

        NextToken();
        std::shared_ptr<ast::Expression> val =
            ParseExpression(Precedence::LOWEST);

        hash_lit->pairs_.insert({key, val});

        if (!PeekTokenIs(token::RBRACE) && !ExpectPeek(token::COMMA))
            return nullptr;
    }
    if (!ExpectPeek(token::RBRACE))
    {
        return nullptr;
    }

    return hash_lit;
}

std::shared_ptr<ast::Expression> Parser::ParseIntegerLiteral()
{
    auto literal = std::make_shared<ast::IntegerLiteral>(cur_token_);
    int64_t val = std::stoll(cur_token_.literal_, nullptr, 10);
    literal->value_ = val;
    return literal;
}

std::shared_ptr<ast::Expression> Parser::ParseIfExpression()
{
    auto expression = std::make_shared<ast::IfExpression>(cur_token_);

    if (!ExpectPeek(token::LPAREN))
        return nullptr;

    NextToken();
    expression->condition_ = ParseExpression(Precedence::LOWEST);

    if (!ExpectPeek(token::RPAREN))
        return nullptr;

    if (!ExpectPeek(token::LBRACE))
        return nullptr;

    expression->consequence_ = ParseBlockStatement();

    if (PeekTokenIs(token::ELSE))
    {
        NextToken();

        if (!ExpectPeek(token::LBRACE))
            return nullptr;

        expression->alternative_ = ParseBlockStatement();
    }

    return expression;
}

std::shared_ptr<ast::Expression> Parser::ParseFunctionLiteral()
{
    auto lit = std::make_shared<ast::FunctionLiteral>(cur_token_);

    if (!ExpectPeek(token::LPAREN))
        return nullptr;

    lit->parameters_ = ParseFunctionParameters();

    if (!ExpectPeek(token::LBRACE))
        return nullptr;

    lit->body_ = ParseBlockStatement();

    return lit;
}

std::shared_ptr<ast::Expression> Parser::ParseStringLiteral()
{
    return std::make_shared<ast::StringLiteral>(cur_token_,
                                                cur_token_.literal_);
}

std::shared_ptr<ast::Expression> Parser::ParsePrefixExpression()
{
    auto expression = std::make_shared<ast::PrefixExpression>(
        cur_token_, cur_token_.literal_);

    NextToken();

    expression->right_ = ParseExpression(Precedence::PREFIX);

    return expression;
}

std::shared_ptr<ast::Expression>
Parser::ParseInfixExpression(std::shared_ptr<ast::Expression> left)
{
    auto expression = std::make_shared<ast::InfixExpression>(
        cur_token_, cur_token_.literal_, left);

    auto precedence = CurPrecedence();
    NextToken();
    expression->right_ = ParseExpression(precedence);

    return expression;
}

std::shared_ptr<ast::Expression> Parser::ParseGroupedExpression()
{
    NextToken();
    std::shared_ptr<ast::Expression> expr = ParseExpression(Precedence::LOWEST);
    if (!ExpectPeek(token::RPAREN))
        return nullptr;
    return expr;
}

Precedence Parser::PeekPrecedence() const
{
    auto it = precedences.find(peek_token_.type_);
    if (it != precedences.end())
        return it->second;

    return Precedence::LOWEST;
}

Precedence Parser::CurPrecedence() const
{
    auto it = precedences.find(cur_token_.type_);
    if (it != precedences.end())
        return it->second;
    return Precedence::LOWEST;
}

std::shared_ptr<ast::BlockStatement> Parser::ParseBlockStatement()
{
    auto block_stmt = std::make_shared<ast::BlockStatement>(cur_token_);

    NextToken();
    while (!CurTokenIs(token::RBRACE) && !CurTokenIs(token::EOFF))
    {
        auto stmt = ParseStatement();
        if (stmt)
            block_stmt->statements_.push_back(stmt);
        NextToken();
    }
    return block_stmt;
}

std::vector<std::shared_ptr<ast::Identifier>> Parser::ParseFunctionParameters()
{
    std::vector<std::shared_ptr<ast::Identifier>> identifiers;

    if (PeekTokenIs(token::RPAREN))
    {
        NextToken();
        return identifiers;
    }

    NextToken();

    auto ident =
        std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);
    identifiers.push_back(ident);
    while (PeekTokenIs(token::COMMA))
    {
        NextToken();
        NextToken();
        auto ident =
            std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);
        identifiers.push_back(ident);
    }

    if (!ExpectPeek(token::RPAREN))
    {
        return std::vector<std::shared_ptr<ast::Identifier>>();
    }

    return identifiers;
}

std::shared_ptr<ast::Expression>
Parser::ParseCallExpression(std::shared_ptr<ast::Expression> funct)
{
    std::shared_ptr<ast::CallExpression> expr =
        std::make_shared<ast::CallExpression>(cur_token_, funct);
    expr->arguments_ = ParseExpressionList(token::RPAREN);
    return expr;
}

std::shared_ptr<ast::Expression>
Parser::ParseIndexExpression(std::shared_ptr<ast::Expression> left)
{
    std::shared_ptr<ast::IndexExpression> expr =
        std::make_shared<ast::IndexExpression>(cur_token_, left);
    NextToken();
    expr->index_ = ParseExpression(Precedence::LOWEST);
    if (!ExpectPeek(token::RBRACKET))
    {
        return nullptr;
    }
    return expr;
}

std::vector<std::shared_ptr<ast::Expression>>
Parser::ParseExpressionList(token::TokenType end)
{
    std::vector<std::shared_ptr<ast::Expression>> listy;
    if (PeekTokenIs(end))
    {
        NextToken();
        return listy;
    }

    NextToken();
    listy.push_back(ParseExpression(Precedence::LOWEST));

    while (PeekTokenIs(token::COMMA))
    {
        NextToken();
        NextToken();
        listy.push_back(ParseExpression(Precedence::LOWEST));
    }

    if (!ExpectPeek(end))
    {
        return std::vector<std::shared_ptr<ast::Expression>>();
    }

    return listy;
}
} // namespace parser
