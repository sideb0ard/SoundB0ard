#include <optional>

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenz.hpp>

namespace pattern_parser
{

Parser::Parser(std::shared_ptr<pattern_parser::Tokenizer> tokenizer)
    : tokenizer_{tokenizer}
{
    NextToken();
    NextToken();
}

std::shared_ptr<pattern_parser::EventGroup> Parser::ParsePattern()
{
    // pattern_parser::Token root_token = {pattern_parser::PATTERN_GROUP,
    // "root"};
    auto pattern_root = std::make_shared<pattern_parser::EventGroup>();

    int node_counter{0};
    while (cur_token_.type_ != pattern_parser::PATTERN_EOF)
    {
        std::cout << "\n**BING!BING! node #" << ++node_counter << std::endl;
        std::shared_ptr<pattern_parser::PatternNode> ev = ParsePatternNode();
        if (ev)
        {
            std::cout << "Node Event is " << ev->String() << std::endl;
            pattern_root->events_.push_back(ev);
        }
        else
            std::cout << "NO PATTERNODE\n";
        NextToken();
    }

    std::cout << "//// num events:" << pattern_root->events_.size()
              << std::endl;
    return pattern_root;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternNode()
{
    std::cout << "PARSE PATTERN NODE - cur token is " << cur_token_
              << std::endl;

    std::shared_ptr<pattern_parser::PatternNode> return_node;

    if (cur_token_.type_.compare(pattern_parser::PATTERN_IDENT) == 0)
    {
        std::cout << "TOKEN TYPE IS IDENT!" << std::endl;
        return_node = ParsePatternIdent();
    }
    else if (cur_token_.type_.compare(
                 pattern_parser::PATTERN_SQUARE_BRACKET_LEFT) == 0)
    {
        std::cout << "TOKEN TYPE IS GROUP!" << std::endl;
        return_node = ParsePatternGroup();
    }
    else
        return nullptr;

    if (PeekTokenIs(pattern_parser::PATTERN_MULTIPLIER) ||
        PeekTokenIs(pattern_parser::PATTERN_DIVIDER))
    {
        std::cout << "OOH, OOH, FOUND MODIFIER!\n\n";
        bool is_multiplier{false};
        if (PeekTokenIs(pattern_parser::PATTERN_MULTIPLIER))
            is_multiplier = true;

        NextToken();
        if (!ExpectPeek(pattern_parser::PATTERN_INT))
        {
            std::cerr << "NEED A NUMBER FOR A MODIFIER!!\n";
            return nullptr;
        }
        int modifier_value = std::stoi(cur_token_.literal_);
        if (modifier_value == 0)
        {
            std::cerr << "MODIFIER can't be 0!!\n";
            return nullptr;
        }

        if (is_multiplier)
        {
            auto ev_group = std::make_shared<pattern_parser::EventGroup>();
            for (int j = 0; j < modifier_value; j++)
            {
                ev_group->events_.push_back(return_node);
            }
            return ev_group;
        }
        else
        {
            std::cout << "Setting MODIFIER TO DIIIIIVvvvy\n";
            return_node->modifier_ = EventModifier::DIVIDE;
            return_node->modifier_value_ = modifier_value;
        }
    }

    return return_node;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternIdent()
{

    std::cout << "  INSIDE PARSE PATTERN IDENT\n";
    std::shared_ptr<pattern_parser::Identifier> node =
        std::make_shared<pattern_parser::Identifier>(cur_token_.literal_);

    return node;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternGroup()
{

    std::cout << "  INSIDE PARSE PATTERN GROUP\n";
    std::shared_ptr<pattern_parser::EventGroup> events =
        std::make_shared<pattern_parser::EventGroup>();

    return events;
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

bool Parser::ExpectPeek(pattern_parser::TokenType t)
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
    peek_token_ = tokenizer_->NextToken();
}

bool Parser::CurTokenIs(pattern_parser::TokenType t) const
{
    if (cur_token_.type_.compare(t) == 0)
        return true;
    return false;
}

bool Parser::PeekTokenIs(pattern_parser::TokenType t) const
{
    if (peek_token_.type_.compare(t) == 0)
        return true;
    return false;
}

void Parser::PeekError(pattern_parser::TokenType t)
{
    std::stringstream msg;
    msg << "Expected next token to be " << t << ", got " << peek_token_.type_
        << " instead";
    errors_.push_back(msg.str());
}

// static bool IsInfixOperator(token::TokenType type)
//{
//    if (type == token::SLANG_PLUS || type == token::SLANG_MINUS ||
//        type == token::SLANG_SLASH || type == token::SLANG_ASTERISK ||
//        type == token::SLANG_EQ || type == token::SLANG_NOT_EQ ||
//        type == token::SLANG_LT || type == token::SLANG_GT ||
//        type == token::SLANG_LPAREN || type == token::SLANG_LBRACKET)
//    {
//        return true;
//    }
//    return false;
//}

//////////////////////////////////////////////////////////////////

// std::shared_ptr<ast::Expression> Parser::ParseExpression(Precedence p)
//{
//    // these are the 'nuds' (null detontations) in the Vaughan Pratt paper
//    'top
//    // down operator precedence'.
//    std::shared_ptr<ast::Expression> left_expr = ParseForPrefixExpression();
//
//    if (!left_expr)
//        return nullptr;
//
//    while (!PeekTokenIs(token::SLANG_SEMICOLON) && p < PeekPrecedence())
//    {
//        if (IsInfixOperator(peek_token_.type_))
//        {
//            NextToken();
//            if (cur_token_.type_ == token::SLANG_LPAREN)
//                left_expr = ParseCallExpression(left_expr);
//            else if (cur_token_.type_ == token::SLANG_LBRACKET)
//                left_expr = ParseIndexExpression(left_expr);
//            else // these are the 'leds' (left denotation)
//                left_expr = ParseInfixExpression(left_expr);
//        }
//        else
//        {
//            return left_expr;
//        }
//    }
//    return left_expr;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseForPrefixExpression()
//{
//    if (cur_token_.type_ == token::SLANG_IDENT)
//        return ParseIdentifier();
//    else if (cur_token_.type_ == token::SLANG_INT)
//        return ParseIntegerLiteral();
//    else if (cur_token_.type_ == token::SLANG_INCREMENT)
//        return ParsePrefixExpression();
//    else if (cur_token_.type_ == token::SLANG_DECREMENT)
//        return ParsePrefixExpression();
//    else if (cur_token_.type_ == token::SLANG_BANG)
//        return ParsePrefixExpression();
//    else if (cur_token_.type_ == token::SLANG_MINUS)
//        return ParsePrefixExpression();
//    else if (cur_token_.type_ == token::SLANG_TRUE)
//        return ParseBoolean();
//    else if (cur_token_.type_ == token::SLANG_FALSE)
//        return ParseBoolean();
//    else if (cur_token_.type_ == token::SLANG_LPAREN)
//        return ParseGroupedExpression();
//    else if (cur_token_.type_ == token::SLANG_IF)
//        return ParseIfExpression();
//    else if (cur_token_.type_ == token::SLANG_FUNCTION)
//        return ParseFunctionLiteral();
//    else if (cur_token_.type_ == token::SLANG_FM_SYNTH)
//        return ParseSynthExpression();
//    else if (cur_token_.type_ == token::SLANG_SAMPLE)
//        return ParseSampleExpression();
//    else if (cur_token_.type_ == token::SLANG_PROC)
//        return ParseProcessExpression();
//    else if (cur_token_.type_ == token::SLANG_STRING)
//        return ParseStringLiteral();
//    else if (cur_token_.type_ == token::SLANG_LBRACKET)
//        return ParseArrayLiteral();
//    else if (cur_token_.type_ == token::SLANG_LBRACE)
//        return ParseHashLiteral();
//    else if (cur_token_.type_ == token::SLANG_EVERY)
//        return ParseEveryExpression();
//
//    std::cout << "No Prefix parser for " << cur_token_.type_ << std::endl;
//    return nullptr;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseIdentifier()
//{
//    return std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseBoolean()
//{
//    return std::make_shared<ast::BooleanExpression>(
//        cur_token_, CurTokenIs(token::SLANG_TRUE));
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseArrayLiteral()
//{
//    auto array_lit = std::make_shared<ast::ArrayLiteral>(cur_token_);
//    array_lit->elements_ = ParseExpressionList(token::SLANG_RBRACKET);
//    return array_lit;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseHashLiteral()
//{
//    auto hash_lit = std::make_shared<ast::HashLiteral>(cur_token_);
//
//    while (!PeekTokenIs(token::SLANG_RBRACE))
//    {
//        NextToken();
//        std::shared_ptr<ast::Expression> key =
//            ParseExpression(Precedence::LOWEST);
//
//        if (!ExpectPeek(token::SLANG_COLON))
//            return nullptr;
//
//        NextToken();
//        std::shared_ptr<ast::Expression> val =
//            ParseExpression(Precedence::LOWEST);
//
//        hash_lit->pairs_.insert({key, val});
//
//        if (!PeekTokenIs(token::SLANG_RBRACE) &&
//            !ExpectPeek(token::SLANG_COMMA))
//            return nullptr;
//    }
//    if (!ExpectPeek(token::SLANG_RBRACE))
//    {
//        return nullptr;
//    }
//
//    return hash_lit;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseIntegerLiteral()
//{
//    auto literal = std::make_shared<ast::IntegerLiteral>(cur_token_);
//    int64_t val = std::stoll(cur_token_.literal_, nullptr, 10);
//    literal->value_ = val;
//    return literal;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseIfExpression()
//{
//    auto expression = std::make_shared<ast::IfExpression>(cur_token_);
//
//    if (!ExpectPeek(token::SLANG_LPAREN))
//        return nullptr;
//
//    NextToken();
//    expression->condition_ = ParseExpression(Precedence::LOWEST);
//
//    if (!ExpectPeek(token::SLANG_RPAREN))
//        return nullptr;
//
//    if (!ExpectPeek(token::SLANG_LBRACE))
//        return nullptr;
//
//    expression->consequence_ = ParseBlockStatement();
//
//    if (PeekTokenIs(token::SLANG_ELSE))
//    {
//        NextToken();
//
//        if (!ExpectPeek(token::SLANG_LBRACE))
//            return nullptr;
//
//        expression->alternative_ = ParseBlockStatement();
//    }
//
//    return expression;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseEveryExpression()
//{
//    std::cout << "Parse EVERY\n";
//    ShowTokens();
//    auto expression = std::make_shared<ast::EveryExpression>(cur_token_);
//
//    ShowTokens();
//    if (!ExpectPeek(token::SLANG_LPAREN))
//        return nullptr;
//    NextToken();
//    ShowTokens();
//
//    expression->frequency_ = std::stoll(cur_token_.literal_, nullptr, 10);
//    std::cout << "GOT FREQUENCY?\n";
//    ShowTokens();
//
//    if (!ExpectTimingEvent())
//        return nullptr;
//    NextToken();
//    expression->event_type_ = ParseTimingEventLiteral();
//
//    ShowTokens();
//
//    if (!ExpectPeek(token::SLANG_RPAREN))
//        return nullptr;
//
//    std::cout << "AIIGHT, GOT RPAREN - whats next?\n";
//    ShowTokens();
//    if (!ExpectPeek(token::SLANG_LBRACE))
//        return nullptr;
//    // NextToken();
//
//    std::cout << "AIIGHT, GOT LBRAC E - whats next?\n";
//    ShowTokens();
//
//    expression->body_ = ParseBlockStatement();
//
//    std::cout << "GOOD!" << expression->String() << std::endl;
//
//    return expression;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseFunctionLiteral()
//{
//    auto lit = std::make_shared<ast::FunctionLiteral>(cur_token_);
//
//    if (!ExpectPeek(token::SLANG_LPAREN))
//        return nullptr;
//
//    lit->parameters_ = ParseFunctionParameters();
//
//    if (!ExpectPeek(token::SLANG_LBRACE))
//        return nullptr;
//
//    lit->body_ = ParseBlockStatement();
//
//    return lit;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseSynthExpression()
//{
//    auto synth = std::make_shared<ast::SynthExpression>(cur_token_);
//
//    if (!ExpectPeek(token::SLANG_LPAREN))
//        return nullptr;
//
//    if (!ExpectPeek(token::SLANG_RPAREN))
//        return nullptr;
//
//    std::cout << "AST SYNTH EXPRESSION ALL GOOD!\n";
//    return synth;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseSampleExpression()
//{
//    auto sample = std::make_shared<ast::SampleExpression>(cur_token_);
//
//    if (!ExpectPeek(token::SLANG_LPAREN))
//        return nullptr;
//    NextToken();
//
//    std::cout << "Cur token is " << cur_token_ << std::endl;
//    sample->path_ = ParseStringLiteral();
//
//    if (!ExpectPeek(token::SLANG_RPAREN))
//        return nullptr;
//
//    std::cout << "AST SAMPLE EXPRESSION ALL GOOD!\n";
//    return sample;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseProcessExpression()
//{
//    // std::string input = R"(let rhythm = proc(sound, "bd*3 sd"))";
//    auto process = std::make_shared<ast::ProcessExpression>(cur_token_);
//
//    if (!ExpectPeek(token::SLANG_LPAREN))
//        return nullptr;
//    NextToken();
//
//    std::cout << "Cur token is " << cur_token_ << std::endl;
//    process->target_ = ParseStringLiteral();
//
//    if (!ExpectPeek(token::SLANG_COMMA))
//        return nullptr;
//    NextToken();
//
//    std::cout << "Post comma - Cur token is " << cur_token_ << std::endl;
//
//    process->pattern_ = ParseStringLiteral();
//    std::cout << "Post Pattern Parsed - Cur token is " << cur_token_
//              << std::endl;
//
//    if (!ExpectPeek(token::SLANG_RPAREN))
//        return nullptr;
//
//    std::cout << "AST PROC EXPRESSION ALL GOOD!\n";
//    return process;
//}
//
// ast::TimingEventType Parser::ParseTimingEventLiteral()
//{
//    if (cur_token_.type_ == token::SLANG_TIMING_MIDI_TICK)
//        return ast::TimingEventType::MIDI_TICK;
//    else if (cur_token_.type_ == token::SLANG_TIMING_THIRTYSECOND)
//        return ast::TimingEventType::THIRTYSECOND;
//    else if (cur_token_.type_ == token::SLANG_TIMING_TWENTYFOURTH)
//        return ast::TimingEventType::TWENTYFOURTH;
//    else if (cur_token_.type_ == token::SLANG_TIMING_SIXTEENTH)
//        return ast::TimingEventType::SIXTEENTH;
//    else if (cur_token_.type_ == token::SLANG_TIMING_TWELTH)
//        return ast::TimingEventType::TWELTH;
//    else if (cur_token_.type_ == token::SLANG_TIMING_EIGHTH)
//        return ast::TimingEventType::EIGHTH;
//    else if (cur_token_.type_ == token::SLANG_TIMING_SIXTH)
//        return ast::TimingEventType::SIXTH;
//    else if (cur_token_.type_ == token::SLANG_TIMING_QUARTER)
//        return ast::TimingEventType::QUARTER;
//    else if (cur_token_.type_ == token::SLANG_TIMING_THIRD)
//        return ast::TimingEventType::THIRD;
//    else if (cur_token_.type_ == token::SLANG_TIMING_BAR)
//        return ast::TimingEventType::BAR;
//    else // default - should never happen?!
//        return ast::TimingEventType::BAR;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseStringLiteral()
//{
//    return std::make_shared<ast::StringLiteral>(cur_token_,
//                                                cur_token_.literal_);
//}
//
// std::shared_ptr<ast::Expression> Parser::ParsePrefixExpression()
//{
//    auto expression = std::make_shared<ast::PrefixExpression>(
//        cur_token_, cur_token_.literal_);
//
//    NextToken();
//
//    expression->right_ = ParseExpression(Precedence::PREFIX);
//
//    return expression;
//}
//
// std::shared_ptr<ast::Expression>
// Parser::ParseInfixExpression(std::shared_ptr<ast::Expression> left)
//{
//    auto expression = std::make_shared<ast::InfixExpression>(
//        cur_token_, cur_token_.literal_, left);
//
//    auto precedence = CurPrecedence();
//    NextToken();
//    expression->right_ = ParseExpression(precedence);
//
//    return expression;
//}
//
// std::shared_ptr<ast::Expression> Parser::ParseGroupedExpression()
//{
//    NextToken();
//    std::shared_ptr<ast::Expression> expr =
//    ParseExpression(Precedence::LOWEST); if (!ExpectPeek(token::SLANG_RPAREN))
//        return nullptr;
//    return expr;
//}

// Precedence Parser::PeekPrecedence() const
//{
//    auto it = precedences.find(peek_token_.type_);
//    if (it != precedences.end())
//        return it->second;
//
//    return Precedence::LOWEST;
//}
//
// Precedence Parser::CurPrecedence() const
//{
//    auto it = precedences.find(cur_token_.type_);
//    if (it != precedences.end())
//        return it->second;
//    return Precedence::LOWEST;
//}
//
// std::shared_ptr<ast::BlockStatement> Parser::ParseBlockStatement()
//{
//    auto block_stmt = std::make_shared<ast::BlockStatement>(cur_token_);
//
//    NextToken();
//    while (!CurTokenIs(token::SLANG_RBRACE) && !CurTokenIs(token::SLANG_EOFF))
//    {
//        auto stmt = ParseStatement();
//        if (stmt)
//            block_stmt->statements_.push_back(stmt);
//        NextToken();
//    }
//    return block_stmt;
//}
//
// std::vector<std::shared_ptr<ast::Identifier>>
// Parser::ParseFunctionParameters()
//{
//    std::vector<std::shared_ptr<ast::Identifier>> identifiers;
//
//    if (PeekTokenIs(token::SLANG_RPAREN))
//    {
//        NextToken();
//        return identifiers;
//    }
//
//    NextToken();
//
//    auto ident =
//        std::make_shared<ast::Identifier>(cur_token_, cur_token_.literal_);
//    identifiers.push_back(ident);
//    while (PeekTokenIs(token::SLANG_COMMA))
//    {
//        NextToken();
//        NextToken();
//        auto ident =
//            std::make_shared<ast::Identifier>(cur_token_,
//            cur_token_.literal_);
//        identifiers.push_back(ident);
//    }
//
//    if (!ExpectPeek(token::SLANG_RPAREN))
//    {
//        return std::vector<std::shared_ptr<ast::Identifier>>();
//    }
//
//    return identifiers;
//}
//
// std::shared_ptr<ast::Expression>
// Parser::ParseCallExpression(std::shared_ptr<ast::Expression> funct)
//{
//    std::shared_ptr<ast::CallExpression> expr =
//        std::make_shared<ast::CallExpression>(cur_token_, funct);
//    expr->arguments_ = ParseExpressionList(token::SLANG_RPAREN);
//    return expr;
//}
//
// std::shared_ptr<ast::Expression>
// Parser::ParseIndexExpression(std::shared_ptr<ast::Expression> left)
//{
//    std::shared_ptr<ast::IndexExpression> expr =
//        std::make_shared<ast::IndexExpression>(cur_token_, left);
//    NextToken();
//    expr->index_ = ParseExpression(Precedence::LOWEST);
//    if (!ExpectPeek(token::SLANG_RBRACKET))
//    {
//        return nullptr;
//    }
//    return expr;
//}
//
// std::vector<std::shared_ptr<ast::Expression>>
// Parser::ParseExpressionList(token::TokenType end)
//{
//    std::vector<std::shared_ptr<ast::Expression>> listy;
//    if (PeekTokenIs(end))
//    {
//        NextToken();
//        return listy;
//    }
//
//    NextToken();
//    listy.push_back(ParseExpression(Precedence::LOWEST));
//
//    while (PeekTokenIs(token::SLANG_COMMA))
//    {
//        NextToken();
//        NextToken();
//        listy.push_back(ParseExpression(Precedence::LOWEST));
//    }
//
//    if (!ExpectPeek(end))
//    {
//        return std::vector<std::shared_ptr<ast::Expression>>();
//    }
//
//    return listy;
//}

void Parser::ShowTokens()
{
    std::cout << "CUR TOKEN is " << cur_token_.type_ << " // Peek Token is "
              << peek_token_.type_ << std::endl;
}
} // namespace pattern_parser
