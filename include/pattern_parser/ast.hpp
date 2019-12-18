#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <pattern_parser/tokenz.hpp>

namespace pattern_parser
{

class PatternNode
{
  public:
    PatternNode() = default;
    explicit PatternNode(pattern_parser::Token toke) : token_{toke} {}
    virtual ~PatternNode() = default;
    virtual std::string TokenLiteral() const { return token_.literal_; }
    virtual std::string String() const = 0;

  public:
    Token token_;
};

class Identifier : public PatternNode
{
  public:
    Identifier() {}
    Identifier(pattern_parser::Token token, std::string val)
        : PatternNode{token}, value_{val}
    {
    }
    std::string String() const override;
    std::string value_;
};

class IntegerLiteral : public PatternNode
{
  public:
    IntegerLiteral() {}
    explicit IntegerLiteral(pattern_parser::Token token) : PatternNode{token} {}
    IntegerLiteral(pattern_parser::Token token, int64_t val)
        : PatternNode{token}, value_{val}
    {
    }
    std::string String() const override;

  public:
    int64_t value_;
};

class EventGroup : public PatternNode
{
  public:
    explicit EventGroup(Token toke) : PatternNode{toke} {}
    std::string String() const override;

  public:
    std::vector<std::shared_ptr<PatternNode>> events_;
};

} // namespace pattern_parser
