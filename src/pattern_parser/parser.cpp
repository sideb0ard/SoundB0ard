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

std::shared_ptr<pattern_parser::PatternGroup> Parser::ParsePattern()
{
    auto pattern_root = std::make_shared<pattern_parser::PatternGroup>();

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
        return_node = ParsePatternLeaf();
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
        PeekTokenIs(pattern_parser::PATTERN_DIVISOR))
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
        int mod_value = std::stoi(cur_token_.literal_);
        if (mod_value == 0)
        {
            std::cerr << "MODIFIER can't be 0!!\n";
            return nullptr;
        }

        if (is_multiplier)
        {
            auto ev_group = std::make_shared<pattern_parser::PatternGroup>();
            for (int i = 0; i < mod_value; ++i)
                ev_group->events_.push_back(return_node);
            return ev_group;
        }
        else
            return_node->divisor_value_ = mod_value;
    }

    return return_node;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternLeaf()
{

    std::cout << "  INSIDE PARSE PATTERN IDENT\n";
    std::shared_ptr<pattern_parser::PatternLeaf> node =
        std::make_shared<pattern_parser::PatternLeaf>(cur_token_.literal_);

    return node;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternGroup()
{

    std::cout << "  INSIDE PARSE PATTERN GROUP CUR TOKEN IZ:" << cur_token_
              << std::endl;
    std::shared_ptr<pattern_parser::PatternGroup> ev_group =
        std::make_shared<pattern_parser::PatternGroup>();

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
