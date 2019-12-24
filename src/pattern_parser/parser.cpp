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
              << " DONE WITH PARSEPATTERN\n"
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
        std::cout << "TOKEN TYPE IS IDENT!" << cur_token_ << std::endl;
        return_node = ParsePatternIdent();
    }
    else if (cur_token_.type_.compare(
                 pattern_parser::PATTERN_SQUARE_BRACKET_LEFT) == 0)
    {
        std::cout << "TOKEN TYPE IS GROUP!" << std::endl;
        return_node = ParsePatternGroup();
    }
    else
    {
        std::cout << "RETURNING NULL..." << std::endl;
        return nullptr;
    }

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
            std::cout << "Got MULtiplier! " << modifier_value << std::endl;
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

    std::cout << "  INSIDE PARSE PATTERN GROUP CUR TOKEN IZ:" << cur_token_
              << std::endl;
    std::shared_ptr<pattern_parser::EventGroup> ev_group =
        std::make_shared<pattern_parser::EventGroup>();

    while (!PeekTokenIs(pattern_parser::PATTERN_SQUARE_BRACKET_RIGHT))
    {
        NextToken();
        ev_group->events_.push_back(ParsePatternNode());
    }
    std::cout << "  FINISHED!! PARSE PATTERN GROUP CUR TOKEN IZ:" << cur_token_
              << std::endl;
    // discard RIGHT BRACKET
    NextToken();

    return ev_group;
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

void Parser::ShowTokens()
{
    std::cout << "CUR TOKEN is " << cur_token_.type_ << " // Peek Token is "
              << peek_token_.type_ << std::endl;
}
} // namespace pattern_parser
