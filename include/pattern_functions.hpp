#pragma once

#include <pattern_parser/ast.hpp>

class PatternFunction
{
  public:
    PatternFunction() = default;
    virtual ~PatternFunction() = default;
    virtual void
    TransformPattern(pattern_parser::PatternGroup &pattern) const = 0;
};

class PatternEvery : public PatternFunction
{
  public:
    PatternEvery(int every_n, std::shared_ptr<PatternFunction> func)
        : every_n_{every_n}, func_{func}
    {
    }
    void TransformPattern(pattern_parser::PatternGroup &pattern) const override;

  private:
    int every_n_;
    std::shared_ptr<PatternFunction> func_;
};

class PatternReverse : public PatternFunction
{
  public:
    PatternReverse() = default;
    void TransformPattern(pattern_parser::PatternGroup &pattern) const override;
};

class PatternRotate : public PatternFunction
{
  public:
    PatternRotate(unsigned int direction) : direction_{direction} {};
    void TransformPattern(pattern_parser::PatternGroup &pattern) const override;

  private:
    unsigned int direction_;
};
