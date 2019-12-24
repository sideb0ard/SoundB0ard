#include <variant>

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenizer.hpp>

namespace
{

struct PatternParserTest : public ::testing::Test
{
    void SetUp(void) { std::cout << "Setup!\n"; }
};

TEST_F(PatternParserTest, TestEmptyPattern)
{

    std::string pattern{""};
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    std::shared_ptr<pattern_parser::PatternNode> pattern_root =
        pattern_parzer->ParsePattern();

    EXPECT_EQ(0, pattern_root->NumEvents());
}

TEST_F(PatternParserTest, TestSingleEventPattern)
{

    std::string pattern{"bd"};
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    std::shared_ptr<pattern_parser::PatternNode> pattern_root =
        pattern_parzer->ParsePattern();

    EXPECT_EQ(1, pattern_root->NumEvents());

    std::shared_ptr<pattern_parser::PatternGroup> events =
        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
    if (!events)
        FAIL() << "Cannot cast pattern_root to PatternGroup!";
}

TEST_F(PatternParserTest, TestPatternMultiplier)
{

    std::string pattern{"bd*3 sd"};
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    std::shared_ptr<pattern_parser::PatternNode> pattern_root =
        pattern_parzer->ParsePattern();

    EXPECT_EQ(2, pattern_root->NumEvents());

    std::shared_ptr<pattern_parser::PatternGroup> events =
        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
    if (!events)
        FAIL() << "Cannot cast pattern_root to PatternGroup!";

    auto nested_bd = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
        events->events_[0]);
    if (!nested_bd)
        FAIL() << "Cannot cast nested_bd to PatternGroup!";
    EXPECT_EQ(3, nested_bd->NumEvents());

    // snare
    EXPECT_EQ(1, events->events_[1]->NumEvents());
}

TEST_F(PatternParserTest, TestPatternGroupingSingleLevel)
{

    std::string pattern{"[bd bd bd] sd"};
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    std::shared_ptr<pattern_parser::PatternNode> pattern_root =
        pattern_parzer->ParsePattern();

    EXPECT_EQ(2, pattern_root->NumEvents());

    std::shared_ptr<pattern_parser::PatternGroup> events =
        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
    if (!events)
        FAIL() << "Cannot cast pattern_root to PatternGroup!";

    auto nested_bd = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
        events->events_[0]);
    if (!nested_bd)
        FAIL() << "Cannot cast nested_bd to PatternGroup!";
    EXPECT_EQ(3, nested_bd->NumEvents());

    // snare
    EXPECT_EQ(1, events->events_[1]->NumEvents());
}

TEST_F(PatternParserTest, TestPatternGroupingNestedLevel)
{

    std::string pattern{"[bd bd [bd bd]] sd"};
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    std::shared_ptr<pattern_parser::PatternNode> pattern_root =
        pattern_parzer->ParsePattern();

    EXPECT_EQ(2, pattern_root->NumEvents());

    std::shared_ptr<pattern_parser::PatternGroup> events =
        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
    if (!events)
        FAIL() << "Cannot cast pattern_root to PatternGroup!";

    auto nested_bd = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
        events->events_[0]);
    if (!nested_bd)
        FAIL() << "Cannot cast nested_bd to PatternGroup!";
    EXPECT_EQ(3, nested_bd->NumEvents());

    auto nested_nested_bd =
        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
            nested_bd->events_[2]);
    if (!nested_nested_bd)
        FAIL() << "Cannot cast nested_nested_bd to PatternGroup!";
    EXPECT_EQ(2, nested_nested_bd->NumEvents());

    // snare
    EXPECT_EQ(1, events->events_[1]->NumEvents());
}

TEST_F(PatternParserTest, TestPatternDivisor)
{

    std::string pattern{"[bd bd [bd bd]] sd"};
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    std::shared_ptr<pattern_parser::PatternNode> pattern_root =
        pattern_parzer->ParsePattern();

    EXPECT_EQ(2, pattern_root->NumEvents());

    std::shared_ptr<pattern_parser::PatternGroup> events =
        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
    if (!events)
        FAIL() << "Cannot cast pattern_root to PatternGroup!";

    auto nested_bd = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
        events->events_[0]);
    if (!nested_bd)
        FAIL() << "Cannot cast nested_bd to PatternGroup!";
    EXPECT_EQ(3, nested_bd->NumEvents());

    auto nested_nested_bd =
        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
            nested_bd->events_[2]);
    if (!nested_nested_bd)
        FAIL() << "Cannot cast nested_nested_bd to PatternGroup!";
    EXPECT_EQ(2, nested_nested_bd->NumEvents());

    // snare
    EXPECT_EQ(1, events->events_[1]->NumEvents());
}

} // namespace
