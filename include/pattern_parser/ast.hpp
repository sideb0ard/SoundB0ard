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
    virtual ~PatternNode() = default;
    virtual std::string String() const = 0;
    virtual int NumEvents() const = 0;
    virtual int GetDivisor() const { return divisor_value_; }

  public:
    int divisor_value_{0}; // only set when a divisor is present
};

class PatternLeaf : public PatternNode
{
  public:
    PatternLeaf() {}
    PatternLeaf(std::string val) : value_{val} {}
    std::string String() const override;
    int NumEvents() const override;

  public:
    std::string value_;
};

class PatternGroup : public PatternNode
{
  public:
    PatternGroup() = default;
    std::string String() const override;
    int NumEvents() const override;

  public:
    std::vector<std::shared_ptr<PatternNode>> events_;
};

} // namespace pattern_parser
