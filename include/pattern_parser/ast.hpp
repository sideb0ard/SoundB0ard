#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <pattern_parser/tokenz.hpp>

namespace pattern_parser
{

enum EventModifier
{
    NONE,
    MULTIPLY,
    DIVIDE,

};

static const char *MOD_NAMES[]{"NONE", "MULTIPLY", "DIVIDE"};

class PatternNode
{
  public:
    PatternNode() = default;
    virtual ~PatternNode() = default;
    virtual std::string String() const = 0;
    virtual int NumEvents() const = 0;

  public:
    // Token token_;
    EventModifier modifier_{EventModifier::NONE};
    int modifier_value_{0};
};

class Identifier : public PatternNode
{
  public:
    Identifier() {}
    Identifier(std::string val) : value_{val} {}
    std::string String() const override;
    int NumEvents() const override;

  public:
    std::string value_;
};

class EventGroup : public PatternNode
{
  public:
    EventGroup() = default;
    // explicit EventGroup(Token toke) : PatternNode{toke} {}
    std::string String() const override;
    int NumEvents() const override;

  public:
    std::vector<std::shared_ptr<PatternNode>> events_;
};

} // namespace pattern_parser
