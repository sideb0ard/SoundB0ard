#include <optional>

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <defjams.h>

#include <interpreter/parser.hpp>
#include <pattern_functions.hpp>

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

    while (cur_token_.type_ != token::SLANG_EOFF)
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
    if (cur_token_.type_.compare(token::SLANG_LET) == 0)
        return ParseLetStatement();
    else if (cur_token_.type_.compare(token::SLANG_RETURN) == 0)
        return ParseReturnStatement();
    else if (cur_token_.type_.compare(token::SLANG_LS) == 0)
        return ParseLsStatement();
    else if (cur_token_.type_.compare(token::SLANG_PS) == 0)
        return ParsePsStatement();
    else if (cur_token_.type_.compare(token::SLANG_HELP) == 0)
        return ParseHelpStatement();
    else if (cur_token_.type_.compare(token::SLANG_STRATEGY) == 0)
        return ParseStrategyStatement();
    else if (cur_token_.type_.compare(token::SLANG_FOR) == 0)
        return ParseForStatement();
    else if (cur_token_.type_.compare(token::SLANG_PAN) == 0)
        return ParsePanStatement();
    else if (cur_token_.type_.compare(token::SLANG_PLAY) == 0)
        return ParsePlayStatement();
    else if (cur_token_.type_.compare(token::SLANG_PROC_ID) == 0)
        return ParseProcessStatement();
    else if (cur_token_.type_.compare(token::SLANG_SET) == 0)
        return ParseSetStatement();
    else if (cur_token_.type_.compare(token::SLANG_VOLUME) == 0)
        return ParseVolumeStatement();
    else if (cur_token_.type_.compare(token::SLANG_BPM) == 0)
        return ParseBpmStatement();
    else if (cur_token_.type_.compare(token::SLANG_INFO) == 0)
        return ParseInfoStatement();
    else
        return ParseExpressionStatement();
}

std::shared_ptr<ast::LetStatement> Parser::ParseLetStatement()
{

    auto stmt = std::make_shared<ast::LetStatement>(cur_token_);
    stmt->is_new_item = true;

    if (!ExpectPeek(token::SLANG_IDENT))
    {
        std::cerr << "NO IDENT! - returning nullptr \n";
        return nullptr;
    }

    stmt->name_ =
        std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);

    if (!ExpectPeek(token::SLANG_ASSIGN))
    {
        std::cerr << "NO ASSIGN! - returning nullopt \n";
        return nullptr;
    }

    NextToken();

    stmt->value_ = ParseExpression(Precedence::LOWEST);

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::ReturnStatement> Parser::ParseReturnStatement()
{

    std::shared_ptr<ast::ReturnStatement> stmt =
        std::make_shared<ast::ReturnStatement>(cur_token_);

    NextToken();

    stmt->return_value_ = ParseExpression(Precedence::LOWEST);

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::LsStatement> Parser::ParseLsStatement()
{

    std::shared_ptr<ast::LsStatement> stmt =
        std::make_shared<ast::LsStatement>(cur_token_);

    // TODO - make a list of paths
    if (!PeekTokenIs(token::SLANG_SEMICOLON))
    {
        NextToken();
        stmt->path_ = ParseStringLiteral();
    }

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::StrategyStatement> Parser::ParseStrategyStatement()
{

    std::shared_ptr<ast::StrategyStatement> stmt =
        std::make_shared<ast::StrategyStatement>(cur_token_);

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::PlayStatement> Parser::ParsePlayStatement()
{
    std::shared_ptr<ast::PlayStatement> stmt =
        std::make_shared<ast::PlayStatement>(cur_token_);

    NextToken();

    std::stringstream ss;
    while (!CurTokenIs(token::SLANG_EOFF) &&
           !CurTokenIs(token::SLANG_SEMICOLON))
    {
        ss << cur_token_.literal_;
        NextToken();
    }
    stmt->path_ = ss.str();

    return stmt;
}

std::shared_ptr<ast::SetStatement> Parser::ParseSetStatement()
{
    std::shared_ptr<ast::SetStatement> stmt =
        std::make_shared<ast::SetStatement>(cur_token_);

    if (!ExpectPeek(token::SLANG_IDENT))
    {
        std::cerr << "NOT GOT TARGET ! Peek token is " << peek_token_
                  << std::endl;
        return nullptr;
    }
    stmt->target_ = ParseExpression(Precedence::LOWEST);

    if (!ExpectPeek(token::SLANG_COLON))
    {
        std::cerr << "NOT GOT COLON ! Peek token is " << peek_token_
                  << std::endl;
        return nullptr;
    }

    if (!ExpectPeek(token::SLANG_IDENT) && !ExpectPeek(token::SLANG_VOLUME))
    {
        std::cerr << "NOT GOT PARAM ! Peek token is " << peek_token_
                  << std::endl;
        return nullptr;
    }
    if (cur_token_.literal_.rfind("fx", 0) == 0)
    {
        if (cur_token_.literal_.size() > 2)
        {
            int fx_num = std::stoi(cur_token_.literal_.substr(2));
            stmt->fx_num_ = fx_num;
            if (!ExpectPeek(token::SLANG_COLON))
            {
                std::cerr << "NOT GOT COLON ! Peek token is " << peek_token_
                          << std::endl;
                return nullptr;
            }
            NextToken();
        }
        else
        {
            std::cerr << "Tried FX - no num!\n";
            return nullptr;
        }
    }
    stmt->param_ = cur_token_.literal_;

    NextToken();
    stmt->value_ = ParseExpression(Precedence::LOWEST);
    // if (!ParseSetStatementValue(stmt->value_))
    // {
    //     std::cerr << "NOT GOT NU<M ! Peek token is " << peek_token_
    //               << std::endl;
    //     return nullptr;
    // }

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}
std::shared_ptr<ast::BpmStatement> Parser::ParseBpmStatement()
{
    std::shared_ptr<ast::BpmStatement> stmt =
        std::make_shared<ast::BpmStatement>(cur_token_);

    NextToken();
    stmt->bpm_val_ = ParseExpression(Precedence::LOWEST);
    if (!stmt->bpm_val_)
        return nullptr;

    return stmt;
}

std::shared_ptr<ast::InfoStatement> Parser::ParseInfoStatement()
{
    std::shared_ptr<ast::InfoStatement> stmt =
        std::make_shared<ast::InfoStatement>(cur_token_);

    NextToken();
    stmt->soundgen_identifier_ = ParseExpression(Precedence::LOWEST);
    if (!stmt->soundgen_identifier_)
        return nullptr;

    return stmt;
}

// bool Parser::ParseSetStatementValue(std::string &value)
//{
//    if (PeekTokenIs(token::SLANG_IDENT) || PeekTokenIs(token::SLANG_NUMBER))
//    {
//        NextToken();
//        value = cur_token_.literal_;
//        return true;
//    }
//    else if (PeekTokenIs(token::SLANG_MINUS))
//    {
//        NextToken();
//        if (PeekTokenIs(token::SLANG_NUMBER))
//        {
//            NextToken();
//            value = "-" + cur_token_.literal_;
//            return true;
//        }
//    }
//    return false;
//}

std::shared_ptr<ast::VolumeStatement> Parser::ParseVolumeStatement()
{
    std::shared_ptr<ast::VolumeStatement> stmt =
        std::make_shared<ast::VolumeStatement>(cur_token_);

    if (PeekTokenIs(token::SLANG_IDENT))
    {
        NextToken();
        stmt->target_ = ParseIdentifier();
    }

    if (PeekTokenIs(token::SLANG_NUMBER))
    {
        NextToken();
        stmt->value_ = cur_token_.literal_;
    }
    else
    {
        std::cerr << "NOT GOT NU<M ! Peek token is " << peek_token_
                  << std::endl;
        return nullptr;
    }

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::PanStatement> Parser::ParsePanStatement()
{
    std::shared_ptr<ast::PanStatement> stmt =
        std::make_shared<ast::PanStatement>(cur_token_);

    if (!ExpectPeek(token::SLANG_IDENT))
    {
        std::cerr << "NOT GOT TARGET ! Peek token is " << peek_token_
                  << std::endl;
        return nullptr;
    }
    stmt->target_ = ParseIdentifier();

    if (PeekTokenIs(token::SLANG_NUMBER))
    {
        NextToken();
        stmt->value_ = cur_token_.literal_;
    }
    else if (PeekTokenIs(token::SLANG_MINUS))
    {
        NextToken();
        NextToken();
        stmt->value_ = "-" + cur_token_.literal_;
    }
    else
    {
        std::cerr << "NOT GOT NU<M ! Peek token is " << peek_token_
                  << std::endl;
        return nullptr;
    }

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::HelpStatement> Parser::ParseHelpStatement()
{

    std::shared_ptr<ast::HelpStatement> stmt =
        std::make_shared<ast::HelpStatement>(cur_token_);

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::PsStatement> Parser::ParsePsStatement()
{

    std::shared_ptr<ast::PsStatement> stmt =
        std::make_shared<ast::PsStatement>(cur_token_);

    if (PeekTokenIs(token::SLANG_IDENT) && peek_token_.literal_ == "all")
    {
        stmt->all_ = true;
        NextToken();
    }

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return stmt;
}

std::shared_ptr<ast::ForStatement> Parser::ParseForStatement()
{
    std::shared_ptr<ast::ForStatement> stmt =
        std::make_shared<ast::ForStatement>(cur_token_);

    // Starting condition //////////

    if (!ExpectPeek(token::SLANG_LPAREN))
    {
        std::cerr << "NO LPAREN! - returning nullptr \n";
        return nullptr;
    }

    if (!ExpectPeek(token::SLANG_IDENT))
    {
        std::cerr << "NO IDENT! - returning nullptr \n";
        return nullptr;
    }

    stmt->iterator_ =
        std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);

    if (!ExpectPeek(token::SLANG_ASSIGN))
    {
        std::cerr << "NO ASSIGN! - returning nullptr \n";
        return nullptr;
    }
    NextToken();

    stmt->iterator_value_ = ParseExpression(Precedence::LOWEST);

    if (!ExpectPeek(token::SLANG_SEMICOLON))
    {
        std::cerr << "NO SEMICOLON! - returning nullptr \n";
        return nullptr;
    }
    NextToken();

    // Termination Condition /////////////
    stmt->termination_condition_ = ParseExpression(Precedence::LOWEST);

    NextToken();
    NextToken();

    // Increment Expression
    stmt->increment_ = ParseExpression(Precedence::LOWEST);

    // Body
    if (!ExpectPeek(token::SLANG_RPAREN))
        return nullptr;
    NextToken();

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    stmt->body_ = ParseBlockStatement();
    return stmt;
}

std::shared_ptr<ast::Statement> Parser::ParseExpressionStatement()
{
    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::make_shared<ast::ExpressionStatement>(cur_token_);
    stmt->expression_ = ParseExpression(Precedence::LOWEST);

    // if expression_ == IDENTIFIER && PeekToken is ASSIGN
    // convert to Let Statement with is_new_item=false - i.e. assign to
    // existing variable
    auto ident = std::dynamic_pointer_cast<ast::Identifier>(stmt->expression_);
    if (ident)
    {
        if (PeekTokenIs(token::SLANG_ASSIGN))
        {
            NextToken();
            Token let_toke(token::SLANG_LET, "let");
            auto stmt = std::make_shared<ast::LetStatement>(let_toke);
            stmt->is_new_item = false;
            stmt->name_ = ident;

            NextToken();
            stmt->value_ = ParseExpression(Precedence::LOWEST);

            if (PeekTokenIs(token::SLANG_SEMICOLON))
                NextToken();

            return stmt;
        }
    }

    if (PeekTokenIs(token::SLANG_SEMICOLON))
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
    if (type == token::SLANG_PLUS || type == token::SLANG_MINUS ||
        type == token::SLANG_SLASH || type == token::SLANG_ASTERISK ||
        type == token::SLANG_MODULO || type == token::SLANG_EQ ||
        type == token::SLANG_NOT_EQ || type == token::SLANG_LT ||
        type == token::SLANG_GT || type == token::SLANG_LPAREN ||
        type == token::SLANG_AND || type == token::SLANG_OR ||
        type == token::SLANG_LBRACKET)
        return true;
    return false;
}

//////////////////////////////////////////////////////////////////

std::shared_ptr<ast::Expression> Parser::ParseExpression(Precedence p)
{
    // these are the 'nuds' (null detontations) in the Vaughan Pratt paper
    // 'top down operator precedence'.
    std::shared_ptr<ast::Expression> left_expr = ParseForPrefixExpression();

    if (!left_expr)
        return nullptr;

    while (!PeekTokenIs(token::SLANG_SEMICOLON) && p < PeekPrecedence())
    {

        if (IsInfixOperator(peek_token_.type_))
        {
            NextToken();
            if (cur_token_.type_ == token::SLANG_LPAREN)
                left_expr = ParseCallExpression(left_expr);
            else if (cur_token_.type_ == token::SLANG_LBRACKET)
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
    if (cur_token_.type_ == token::SLANG_IDENT)
        return ParseIdentifier();
    else if (cur_token_.type_ == token::SLANG_NUMBER)
        return ParseNumberLiteral();
    else if (cur_token_.type_ == token::SLANG_INCREMENT)
        return ParsePrefixExpression();
    else if (cur_token_.type_ == token::SLANG_DECREMENT)
        return ParsePrefixExpression();
    else if (cur_token_.type_ == token::SLANG_BANG)
        return ParsePrefixExpression();
    else if (cur_token_.type_ == token::SLANG_MINUS)
        return ParsePrefixExpression();
    else if (cur_token_.type_ == token::SLANG_TRUE)
        return ParseBoolean();
    else if (cur_token_.type_ == token::SLANG_DURATION)
        return ParseDurationExpression();
    else if (cur_token_.type_ == token::SLANG_VELOCITY)
        return ParseVelocityExpression();
    else if (cur_token_.type_ == token::SLANG_FALSE)
        return ParseBoolean();
    else if (cur_token_.type_ == token::SLANG_LPAREN)
        return ParseGroupedExpression();
    else if (cur_token_.type_ == token::SLANG_IF)
        return ParseIfExpression();
    else if (cur_token_.type_ == token::SLANG_FUNCTION)
        return ParseFunctionLiteral();
    else if (cur_token_.type_ == token::SLANG_GENERATOR)
        return ParseGeneratorLiteral();
    else if (cur_token_.type_ == token::SLANG_GRANULAR ||
             cur_token_.type_ == token::SLANG_GRAIN ||
             cur_token_.type_ == token::SLANG_LOOP)
        return ParseGranularExpression();
    else if (cur_token_.type_ == token::SLANG_FM_SYNTH ||
             cur_token_.type_ == token::SLANG_MOOG_SYNTH ||
             cur_token_.type_ == token::SLANG_DIGI_SYNTH ||
             cur_token_.type_ == token::SLANG_DRUM_SYNTH)
        return ParseSynthExpression();
    else if (cur_token_.type_ == token::SLANG_PATTERN)
        return ParsePatternExpression();
    else if (cur_token_.type_ == token::SLANG_SAMPLE)
        return ParseSampleExpression();
    else if (cur_token_.type_ == token::SLANG_STRING)
        return ParseStringLiteral();
    else if (cur_token_.type_ == token::SLANG_LBRACKET)
        return ParseArrayLiteral();
    else if (cur_token_.type_ == token::SLANG_LBRACE)
        return ParseHashLiteral();

    return nullptr;
}

std::shared_ptr<ast::Expression> Parser::ParseIdentifier()
{
    return std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);
}

std::shared_ptr<ast::Expression> Parser::ParseBoolean()
{
    return std::make_shared<ast::BooleanExpression>(
        cur_token_, CurTokenIs(token::SLANG_TRUE));
}

std::shared_ptr<ast::Expression> Parser::ParseArrayLiteral()
{
    auto array_lit = std::make_shared<ast::ArrayLiteral>(cur_token_);
    array_lit->elements_ = ParseExpressionList(token::SLANG_RBRACKET);
    return array_lit;
}

std::shared_ptr<ast::Expression> Parser::ParseHashLiteral()
{
    auto hash_lit = std::make_shared<ast::HashLiteral>(cur_token_);

    while (!PeekTokenIs(token::SLANG_RBRACE))
    {
        NextToken();
        std::shared_ptr<ast::Expression> key =
            ParseExpression(Precedence::LOWEST);

        if (!ExpectPeek(token::SLANG_COLON))
            return nullptr;

        NextToken();
        std::shared_ptr<ast::Expression> val =
            ParseExpression(Precedence::LOWEST);

        hash_lit->pairs_.insert({key, val});

        if (!PeekTokenIs(token::SLANG_RBRACE) &&
            !ExpectPeek(token::SLANG_COMMA))
            return nullptr;
    }
    if (!ExpectPeek(token::SLANG_RBRACE))
    {
        return nullptr;
    }

    return hash_lit;
}

std::shared_ptr<ast::Expression> Parser::ParseNumberLiteral()
{
    auto literal = std::make_shared<ast::NumberLiteral>(cur_token_);
    double val = std::stod(cur_token_.literal_);
    literal->value_ = val;
    return literal;
}

std::shared_ptr<ast::Expression> Parser::ParseDurationExpression()
{
    auto dur = std::make_shared<ast::DurationExpression>(cur_token_);
    if (!ExpectPeek(token::SLANG_ASSIGN))
        return nullptr;

    NextToken();
    dur->duration_val = ParseExpression(Precedence::LOWEST);
    return dur;
}

std::shared_ptr<ast::Expression> Parser::ParseVelocityExpression()
{
    auto vel = std::make_shared<ast::VelocityExpression>(cur_token_);
    if (!ExpectPeek(token::SLANG_ASSIGN))
        return nullptr;

    NextToken();
    vel->velocity_val = ParseExpression(Precedence::LOWEST);
    return vel;
}

std::shared_ptr<ast::Expression> Parser::ParseIfExpression()
{
    auto expression = std::make_shared<ast::IfExpression>(cur_token_);

    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;

    NextToken();
    expression->condition_ = ParseExpression(Precedence::LOWEST);

    if (!ExpectPeek(token::SLANG_RPAREN))
        return nullptr;

    if (!ExpectPeek(token::SLANG_LBRACE))
        return nullptr;

    expression->consequence_ = ParseBlockStatement();

    if (PeekTokenIs(token::SLANG_ELSE))
    {
        NextToken();

        if (!ExpectPeek(token::SLANG_LBRACE))
            return nullptr;

        expression->alternative_ = ParseBlockStatement();
    }

    return expression;
}

std::shared_ptr<ast::Expression> Parser::ParseEveryExpression()
{
    std::cout << "Parse EVERY\n";
    ShowTokens();
    auto expression = std::make_shared<ast::EveryExpression>(cur_token_);

    return expression;
}

std::shared_ptr<ast::Expression> Parser::ParseFunctionLiteral()
{
    auto lit = std::make_shared<ast::FunctionLiteral>(cur_token_);

    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;

    lit->parameters_ = ParseFunctionParameters();

    if (!ExpectPeek(token::SLANG_LBRACE))
        return nullptr;

    lit->body_ = ParseBlockStatement();

    return lit;
}

std::shared_ptr<ast::Expression> Parser::ParseGeneratorLiteral()
{
    auto lit = std::make_shared<ast::GeneratorLiteral>(cur_token_);

    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;

    // (currently) unused parens
    lit->parameters_ = ParseFunctionParameters();

    if (!ExpectPeek(token::SLANG_LBRACE))
        return nullptr;

    if (!ExpectPeek(token::SLANG_GENERATOR_SETUP))
        return nullptr;

    // Discard (currently) unused parens
    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;
    if (!ExpectPeek(token::SLANG_RPAREN))
        return nullptr;
    if (!ExpectPeek(token::SLANG_LBRACE))
        return nullptr;

    lit->setup_ = ParseBlockStatement();

    if (!ExpectPeek(token::SLANG_GENERATOR_RUN))
        return nullptr;

    // Discard (currently) unused parens
    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;
    if (!ExpectPeek(token::SLANG_RPAREN))
        return nullptr;
    if (!ExpectPeek(token::SLANG_LBRACE))
        return nullptr;

    lit->run_ = ParseBlockStatement();

    return lit;
}

std::shared_ptr<ast::Expression> Parser::ParseSynthExpression()
{

    auto synth = std::make_shared<ast::SynthExpression>(cur_token_);

    if (PeekTokenIs(token::SLANG_IDENT))
    {
        if (peek_token_.literal_ == "presets")
        {
            std::cout << "PRESETS!\n";
            auto expr =
                std::make_shared<ast::SynthPresetExpression>(cur_token_);
            NextToken();
            return expr;
        }
        else if (peek_token_.literal_ == "load" ||
                 peek_token_.literal_ == "save")
        {
            NextToken();
            if (!ExpectPeek(token::SLANG_IDENT))
            {
                std::cerr << "Need a name for preset!\n";
                return nullptr;
            }

            if (peek_token_.literal_ == "load")
            {
                std::cout << "LOAD!\n";
                auto expression =
                    std::make_shared<ast::SynthLoadExpression>(cur_token_);
                NextToken();
                expression->preset_name_ = cur_token_.literal_;
                return expression;
            }
            else
            {
                std::cout << "SAVE!\n";
                auto expression =
                    std::make_shared<ast::SynthSaveExpression>(cur_token_);
                NextToken();
                expression->preset_name_ = cur_token_.literal_;
                return expression;
            }
        }
    }

    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;
    NextToken();

    if (synth->token_.literal_ == "digi")
    {
        std::cout << "DIGI SYNTH YO!!\n";
        std::stringstream ss;
        while (!CurTokenIs(token::SLANG_EOFF) &&
               !CurTokenIs(token::SLANG_RPAREN))
        {
            ss << cur_token_.literal_;
            NextToken();
        }
        synth->sample_path_ = ss.str();
        std::cout << "SAMPLE PATH: " << synth->sample_path_ << std::endl;
    }

    if (!CurTokenIs(token::SLANG_RPAREN))
    {
        std::cerr << "OOFT! where ya PAREN?\n";
        return nullptr;
    }
    return synth;
}

std::shared_ptr<ast::Expression> Parser::ParseGranularExpression()
{
    auto granular = std::make_shared<ast::GranularExpression>(cur_token_);
    if (cur_token_.type_ == token::SLANG_LOOP)
    {
        std::cout << "GOT LOOP TOKEN\n";
        granular->loop_mode_ = true;
    }
    std::cout << "__ LOOP MODE SET TO " << granular->loop_mode_ << "\n";

    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;
    NextToken();

    std::stringstream ss;
    while (!CurTokenIs(token::SLANG_EOFF) && !CurTokenIs(token::SLANG_RPAREN))
    {
        ss << cur_token_.literal_;
        NextToken();
    }
    granular->path_ = ss.str();

    if (!CurTokenIs(token::SLANG_RPAREN))
    {
        std::cout << "OOFT! where ya PAREN?\n";
        return nullptr;
    }

    std::cout << "AST Granular EXPRESSION ALL GOOD!\n";
    return granular;
}

std::shared_ptr<ast::Expression> Parser::ParsePatternExpression()
{
    auto pattern = std::make_shared<ast::PatternExpression>(cur_token_);

    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;
    NextToken();

    std::stringstream ss;
    while (!CurTokenIs(token::SLANG_EOFF) && !CurTokenIs(token::SLANG_RPAREN))
    {
        ss << cur_token_.literal_;
        NextToken();
    }
    pattern->string_pattern = ss.str();

    if (!CurTokenIs(token::SLANG_RPAREN))
    {
        std::cout << "OOFT! where ya PAREN?\n";
        return nullptr;
    }

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    std::cout << "YO, returning ya PATTERN!:" << pattern->string_pattern
              << std::endl;
    return pattern;
}

std::shared_ptr<ast::Expression> Parser::ParseSampleExpression()
{
    auto sample = std::make_shared<ast::SampleExpression>(cur_token_);

    if (!ExpectPeek(token::SLANG_LPAREN))
        return nullptr;
    NextToken();

    std::stringstream ss;
    while (!CurTokenIs(token::SLANG_EOFF) && !CurTokenIs(token::SLANG_RPAREN))
    {
        ss << cur_token_.literal_;
        NextToken();
    }
    sample->path_ = ss.str();

    if (!CurTokenIs(token::SLANG_RPAREN))
    {
        std::cout << "OOFT! where ya PAREN?\n";
        return nullptr;
    }

    if (PeekTokenIs(token::SLANG_SEMICOLON))
        NextToken();

    return sample;
}

void Parser::ConsumePatternFunctions(
    std::shared_ptr<ast::ProcessStatement> proc)
{
    // discard pipe '|'
    NextToken();

    // first token is name of function
    auto func = std::make_shared<ast::PatternFunctionExpression>(cur_token_);
    NextToken();

    // the rest are arguments
    while (!CurTokenIs(token::SLANG_PIPE) && !CurTokenIs(token::SLANG_EOFF))
    {
        auto arg = ParseExpression(Precedence::LOWEST);
        if (arg)
            func->arguments_.push_back(arg);
        NextToken();
    }
    proc->functions_.push_back(func);
}

std::shared_ptr<ast::ProcessStatement> Parser::ParseProcessStatement()
{
    auto process = std::make_shared<ast::ProcessStatement>(cur_token_);

    if (PeekTokenIs(token::SLANG_DOLLAR))
    {
        NextToken();
        NextToken();

        process->process_type_ = PATTERN_PROCESS;
        process->target_type_ = ProcessPatternTarget::ENV;

        process->pattern_expression_ = ParseExpression(Precedence::PREFIX);
        NextToken();
    }
    else if (PeekTokenIs(token::SLANG_HASH))
    {
        NextToken();
        NextToken();

        process->process_type_ = PATTERN_PROCESS;
        process->target_type_ = ProcessPatternTarget::VALUES;

        // process->pattern_ = ParseStringLiteral();
        process->pattern_expression_ = ParseExpression(Precedence::PREFIX);

        if (!ExpectPeek(token::SLANG_IDENT))
        {
            // std::cerr << "btw - Nae IDENTs!\n";
            // return nullptr;
        }
        else
        {
            auto target = std::make_shared<ast::Identifier>(
                cur_token_, cur_token_.literal_);
            if (target)
                process->targets_.push_back(target->value_);
            while (PeekTokenIs(token::SLANG_COMMA))
            {
                NextToken();
                NextToken();
                if (CurTokenIs(token::SLANG_IDENT))
                {
                    auto target = std::make_shared<ast::Identifier>(
                        cur_token_, cur_token_.literal_);
                    if (target)
                        process->targets_.push_back(target->value_);
                }
            }
        }
        NextToken();
    }
    else if (PeekTokenIs(token::SLANG_LT))
    {
        process->process_type_ = COMMAND_PROCESS;
        NextToken();
        if (!PeekTokenIsPatternCommandTimerType())
        {
            std::cerr
                << "Need a Pattern Command TYpe! Over, Every, Osc, While or "
                   "Ramp!\n";
            return nullptr;
        }
        NextToken();
        if (cur_token_.type_ == token::SLANG_EVERY)
            process->process_timer_type_ = ProcessTimerType::EVERY;
        else if (cur_token_.type_ == token::SLANG_OSC)
            process->process_timer_type_ = ProcessTimerType::OSCILLATE;
        else if (cur_token_.type_ == token::SLANG_OVER)
            process->process_timer_type_ = ProcessTimerType::OVER;
        else if (cur_token_.type_ == token::SLANG_RAMP)
            process->process_timer_type_ = ProcessTimerType::RAMP;
        // else is checked for above in the PeekTokenIsPatternCommandTimerType()

        NextToken();
        if (!CurTokenIs(token::SLANG_NUMBER))
        {
            std::cerr << "Need a NUMBER after timer type\n";
            return nullptr;
        }

        process->loop_len_ = std::stof(cur_token_.literal_);
        NextToken();

        process->pattern_expression_ = ParseExpression(Precedence::PREFIX);
        NextToken();

        process->command_ = cur_token_.literal_;
        NextToken();
    }

    while (CurTokenIs(token::SLANG_PIPE))
        ConsumePatternFunctions(process);

    if (!process->pattern_expression_)
    {
        std::cout << "WUFF< COULDNT WORK OUT EXPRESSSION!" << std::endl;
        return nullptr;
    }

    process->name = process->pattern_expression_->String();

    return process;
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
    if (!ExpectPeek(token::SLANG_RPAREN))
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
    while (!CurTokenIs(token::SLANG_RBRACE) && !CurTokenIs(token::SLANG_EOFF))
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

    if (PeekTokenIs(token::SLANG_RPAREN))
    {
        NextToken();
        return identifiers;
    }

    NextToken();

    auto ident =
        std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);
    identifiers.push_back(ident);
    while (PeekTokenIs(token::SLANG_COMMA))
    {
        NextToken();
        NextToken();
        auto ident =
            std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);
        identifiers.push_back(ident);
    }

    if (!ExpectPeek(token::SLANG_RPAREN))
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
    expr->arguments_ = ParseExpressionList(token::SLANG_RPAREN);
    return expr;
}

std::shared_ptr<ast::Expression>
Parser::ParseIndexExpression(std::shared_ptr<ast::Expression> left)
{
    std::shared_ptr<ast::IndexExpression> expr =
        std::make_shared<ast::IndexExpression>(cur_token_, left);
    NextToken();
    expr->index_ = ParseExpression(Precedence::LOWEST);
    if (!ExpectPeek(token::SLANG_RBRACKET))
    {
        std::cerr << "OOGT< NOT GOT RIGHT BRACKET\n";
        return nullptr;
    }
    if (PeekTokenIs(token::SLANG_ASSIGN))
    {
        NextToken();
        NextToken();
        expr->new_value_ = ParseExpression(Precedence::LOWEST);
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

    while (PeekTokenIs(token::SLANG_COMMA))
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

void Parser::ShowTokens()
{
    std::cout << "CUR TOKEN is " << cur_token_.type_ << " // Peek Token is "
              << peek_token_.type_ << std::endl;
}

bool Parser::PeekTokenIsPatternCommandTimerType()
{
    if (PeekTokenIs(token::SLANG_EVERY) || PeekTokenIs(token::SLANG_OVER) ||
        PeekTokenIs(token::SLANG_RAMP) || PeekTokenIs(token::SLANG_OSC))
        return true;
    return false;
}

} // namespace parser
