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

    while (cur_token_.type_ != pattern_parser::PATTERN_EOF)
    {
        std::shared_ptr<pattern_parser::PatternNode> ev = ParsePatternNode();
        if (ev)
            pattern_root->event_groups_[0].push_back(ev);
        else
            std::cerr << "NO PATTERNODE\n";
        NextToken();
    }

    return pattern_root;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternNode()
{
    std::shared_ptr<pattern_parser::PatternNode> return_node;

    if (cur_token_.type_.compare(pattern_parser::PATTERN_IDENT) == 0 ||
        cur_token_.type_.compare(pattern_parser::PATTERN_NUMBER) == 0 ||
        cur_token_.type_.compare(pattern_parser::PATTERN_TILDE) == 0)
        return_node = ParsePatternLeaf();
    else if (cur_token_.type_.compare(
                 pattern_parser::PATTERN_SQUARE_BRACKET_LEFT) == 0)
        return_node = ParsePatternGroup();
    else if (cur_token_.type_.compare(
                 pattern_parser::PATTERN_OPEN_ANGLE_BRACKET) == 0)
        return_node = ParsePatternMultiStep();
    else
    {
        std::cerr << "RETURNING NULL... cos TOKEN IS " << cur_token_
                  << std::endl;
        return nullptr;
    }

    if (PeekTokenIs(pattern_parser::PATTERN_MULTIPLIER) ||
        PeekTokenIs(pattern_parser::PATTERN_DIVISOR))
    {
        bool is_multiplier{false};
        if (PeekTokenIs(pattern_parser::PATTERN_MULTIPLIER))
            is_multiplier = true;

        NextToken();
        if (!ExpectPeek(pattern_parser::PATTERN_NUMBER))
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
                ev_group->event_groups_[0].push_back(return_node);
            return ev_group;
        }
        else
            return_node->divisor_value_ = mod_value;
    }
    else if (PeekTokenIs(pattern_parser::PATTERN_OPEN_PAREN))
    {
        std::cout << "GOT EUCLIDEAN!" << std::endl;
        // Euclidean e.g. '(3,8)'
        // discard open paren
        NextToken();

        if (!ExpectPeek(pattern_parser::PATTERN_NUMBER))
        {
            std::cerr << "NEED A NUMBER FOR A STEP!!\n";
            return nullptr;
        }

        int num_hits = std::stoi(cur_token_.literal_);

        if (!ExpectPeek(pattern_parser::PATTERN_COMMA))
            return nullptr;

        if (!ExpectPeek(pattern_parser::PATTERN_NUMBER))
        {
            std::cerr << "NEED A NUMBER FOR A LEN!!\n";
            return nullptr;
        }
        int num_steps = std::stoi(cur_token_.literal_);

        if (!ExpectPeek(pattern_parser::PATTERN_CLOSE_PAREN))
            return nullptr;

        return_node->euclidean_hits_ = num_hits;
        return_node->euclidean_steps_ = num_steps;

        std::cout << "ALL GOOD!\n";
    }

    return return_node;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternLeaf()
{

    std::shared_ptr<pattern_parser::PatternLeaf> node =
        std::make_shared<pattern_parser::PatternLeaf>(cur_token_.literal_);

    return node;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternMultiStep()
{

    std::shared_ptr<pattern_parser::PatternMultiStep> node =
        std::make_shared<pattern_parser::PatternMultiStep>();

    while (!PeekTokenIs(pattern_parser::PATTERN_CLOSE_ANGLE_BRACKET))
    {
        NextToken();
        auto nnode = ParsePatternNode();
        if (nnode)
            node->values_.push_back(nnode);
    }

    NextToken();

    return node;
}

std::shared_ptr<pattern_parser::PatternNode> Parser::ParsePatternGroup()
{

    std::shared_ptr<pattern_parser::PatternGroup> ev_group =
        std::make_shared<pattern_parser::PatternGroup>();

    int event_group_idx = 0;
    while (!PeekTokenIs(pattern_parser::PATTERN_SQUARE_BRACKET_RIGHT))
    {
        NextToken();
        auto nnode = ParsePatternNode();
        if (nnode)
            ev_group->event_groups_[event_group_idx].push_back(nnode);
        if (PeekTokenIs(pattern_parser::PATTERN_COMMA))
        {
            event_group_idx++;
            std::vector<std::shared_ptr<pattern_parser::PatternNode>>
                new_events;
            ev_group->event_groups_.push_back(new_events);
            NextToken();
        }
    }
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
