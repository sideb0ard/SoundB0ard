// cppcheck-suppress-file syntaxError
#include <iostream>
#include <memory>
#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenizer.hpp>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

namespace {

struct PatternParserTest : public ::testing::Test {
  void SetUp(void) {
    std::cout << "Setup!\n";
  }
};

TEST_F(PatternParserTest, TestEmptyPattern) {
  std::string pattern{""};
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(0, events->event_groups_[0].size());
}

TEST_F(PatternParserTest, TestSingleEventPattern) {
  std::string pattern{"bd"};
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";

  EXPECT_EQ(1, events->event_groups_[0].size());
}

TEST_F(PatternParserTest, TestPatternMultiplier) {
  std::string pattern{"bd*3 sd"};
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(2, events->event_groups_[0].size());

  auto nested_bd = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
      events->event_groups_[0][0]);
  if (!nested_bd) FAIL() << "Cannot cast nested_bd to PatternGroup!";
  EXPECT_EQ(3, nested_bd->event_groups_[0].size());

  // snare
  auto sd = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(
      events->event_groups_[0][1]);
  if (!sd) FAIL() << "Cannot cast events->event_groups[0][1] to PatternLeaf!";
}

TEST_F(PatternParserTest, TestPatternGroupingSingleLevel) {
  std::string pattern{"[bd bd bd] sd"};
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(2, events->event_groups_[0].size());

  auto nested_bd = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
      events->event_groups_[0][0]);
  if (!nested_bd) FAIL() << "Cannot cast nested_bd to PatternGroup!";
  EXPECT_EQ(3, nested_bd->event_groups_[0].size());

  // snare
  auto sd = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(
      events->event_groups_[0][1]);
  if (!sd) FAIL() << "Cannot cast events->event_groups[0][1] to PatternLeaf!";
}

TEST_F(PatternParserTest, TestPatternGroupingNestedLevel) {
  std::string pattern{"[bd bd [bd bd]] sd"};
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(2, events->event_groups_[0].size());

  auto nested_bd = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
      events->event_groups_[0][0]);
  if (!nested_bd) FAIL() << "Cannot cast nested_bd to PatternGroup!";
  EXPECT_EQ(3, nested_bd->event_groups_[0].size());

  auto nested_nested_bd =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
          nested_bd->event_groups_[0][2]);
  if (!nested_nested_bd)
    FAIL() << "Cannot cast nested_nested_bd to PatternGroup!";
  EXPECT_EQ(2, nested_nested_bd->event_groups_[0].size());

  // snare
  auto sd = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(
      events->event_groups_[0][1]);
  if (!sd) FAIL() << "Cannot cast events->event_groups[0][1] to PatternLeaf!";
}

TEST_F(PatternParserTest, TestPatternDivisor) {
  std::string pattern{"bd*3 sd/2"};
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(2, events->event_groups_[0].size());

  auto nested_bd = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
      events->event_groups_[0][0]);
  if (!nested_bd) FAIL() << "Cannot cast nested_bd to PatternGroup!";
  EXPECT_EQ(3, nested_bd->event_groups_[0].size());

  // snare
  auto sd = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(
      events->event_groups_[0][1]);
  if (!sd) FAIL() << "Cannot cast events->event_groups[0][1] to PatternLeaf!";
  EXPECT_EQ(2, sd->GetDivisor());
}

TEST_F(PatternParserTest, TestPatternPolyrhythms) {
  std::string pattern{"[bd bd, cp cp cp] sd/2"};
  // std::string pattern{"[bd bd cp cp cp] sd/2"};
  std::cout << "Testing: " << pattern << std::endl;
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(2, events->event_groups_[0].size());

  auto polyrhythm = std::dynamic_pointer_cast<pattern_parser::PatternGroup>(
      events->event_groups_[0][0]);
  if (!polyrhythm)
    FAIL() << "Cannot cast events->event_groups[0][0] to PatternGroup!";
  EXPECT_EQ(2, polyrhythm->event_groups_.size());
  EXPECT_EQ(2, polyrhythm->event_groups_[0].size());
  EXPECT_EQ(3, polyrhythm->event_groups_[1].size());

  // snare
  auto sd = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(
      events->event_groups_[0][1]);
  if (!sd) FAIL() << "Cannot cast events->event_groups[0][1] to PatternLeaf!";
  EXPECT_EQ(2, sd->GetDivisor());
}

TEST_F(PatternParserTest, TestPatternMultiStep) {
  std::string pattern{"bd <sn snd cp>"};
  std::cout << "Testing: " << pattern << std::endl;
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(2, events->event_groups_[0].size());

  auto multi = std::dynamic_pointer_cast<pattern_parser::PatternMultiStep>(
      events->event_groups_[0][1]);
  if (!multi)
    FAIL() << "Cannot cast events->event_groups[0][1] to PatternMultiStep!";
  EXPECT_EQ(3, multi->values_.size());
}

TEST_F(PatternParserTest, TestSingleEuclideanPattern) {
  std::string pattern{"bd(3,8)"};
  std::cout << "Testing: " << pattern << std::endl;
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  ASSERT_EQ(1, events->event_groups_[0].size());

  auto leaf = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(
      events->event_groups_[0][0]);
  if (!leaf) FAIL() << "Cannot cast events->event_groups[0][1] to PatternLeaf!";

  EXPECT_EQ(3, leaf->euclidean_hits_);
  EXPECT_EQ(8, leaf->euclidean_steps_);
}

TEST_F(PatternParserTest, TestEuclideanAndLeafPattern) {
  std::string pattern{"bd(3,8) sn"};
  std::cout << "Testing: " << pattern << std::endl;
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  ASSERT_EQ(2, events->event_groups_[0].size());

  auto euclid = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(
      events->event_groups_[0][0]);
  if (!euclid)
    FAIL() << "Cannot cast events->event_groups[0][0] to PatternLeaf!";

  EXPECT_EQ(3, euclid->euclidean_hits_);
  EXPECT_EQ(8, euclid->euclidean_steps_);

  auto leaf = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(
      events->event_groups_[0][1]);
  if (!leaf) FAIL() << "Cannot cast events->event_groups[0][1] to PatternLeaf!";

  EXPECT_EQ(0, leaf->euclidean_hits_);
  EXPECT_EQ(0, leaf->euclidean_steps_);
}

TEST_F(PatternParserTest, TestPatternMultiStepAndEuclidean) {
  std::string pattern{"bd <sn snd cp>(3,8)"};
  std::cout << "Testing: " << pattern << std::endl;
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(2, events->event_groups_[0].size());

  auto multi = std::dynamic_pointer_cast<pattern_parser::PatternMultiStep>(
      events->event_groups_[0][1]);
  if (!multi)
    FAIL() << "Cannot cast events->event_groups[0][1] to PatternMultiStep!";
  EXPECT_EQ(3, multi->values_.size());
}

TEST_F(PatternParserTest, TestIntsInPattern) {
  std::string pattern{"34 47 <43 50> 23"};
  std::cout << "Testing: " << pattern << std::endl;
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(4, events->event_groups_[0].size());

  auto multi = std::dynamic_pointer_cast<pattern_parser::PatternMultiStep>(
      events->event_groups_[0][2]);
  if (!multi)
    FAIL() << "Cannot cast events->event_groups[0][1] to PatternMultiStep!";
  EXPECT_EQ(2, multi->values_.size());
}

TEST_F(PatternParserTest, TestFloatsInPattern) {
  std::string pattern{"0.1 4.3 <43.3 50> 2.3"};
  std::cout << "Testing: " << pattern << std::endl;
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  std::shared_ptr<pattern_parser::PatternNode> pattern_root =
      pattern_parzer->ParsePattern();

  std::shared_ptr<pattern_parser::PatternGroup> events =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
  if (!events) FAIL() << "Cannot cast pattern_root to PatternGroup!";
  EXPECT_EQ(4, events->event_groups_[0].size());

  auto multi = std::dynamic_pointer_cast<pattern_parser::PatternMultiStep>(
      events->event_groups_[0][2]);
  if (!multi)
    FAIL() << "Cannot cast events->event_groups[0][1] to PatternMultiStep!";
  EXPECT_EQ(2, multi->values_.size());
}

// TEST_F(PatternParserTest, TestCaretSimplePattern)
//{
//
//    std::string pattern{"c0^3 d0"};
//    std::cout << "Testing: " << pattern << std::endl;
//    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
//    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
//    std::shared_ptr<pattern_parser::PatternNode> pattern_root =
//        pattern_parzer->ParsePattern();
//
//    std::shared_ptr<pattern_parser::PatternGroup> events =
//        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
//    if (!events)
//        FAIL() << "Cannot cast pattern_root to PatternGroup!";
//    EXPECT_EQ(4, events->event_groups_[0].size());
//    for (auto &e : events->event_groups_[0])
//        std::cout << e->String() << std::endl;
//}

// I forgot what i meant to do with Carets!
// TEST_F(PatternParserTest, TestCaretNestedPattern)
//{
//
//    std::string pattern{"<[c0 f0]^3 [f1 g2]>"};
//    std::cout << "Testing: " << pattern << std::endl;
//    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern);
//    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
//    std::shared_ptr<pattern_parser::PatternNode> pattern_root =
//        pattern_parzer->ParsePattern();
//
//    std::shared_ptr<pattern_parser::PatternGroup> events =
//        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(pattern_root);
//    if (!events)
//        FAIL() << "Cannot cast pattern_root to PatternGroup!";
//    EXPECT_EQ(1, events->event_groups_[0].size());
//    for (auto &e : events->event_groups_[0])
//        std::cout << e->String() << std::endl;
//
//    auto multi = std::dynamic_pointer_cast<pattern_parser::PatternMultiStep>(
//        events->event_groups_[0][0]);
//    if (!multi)
//        FAIL() << "Cannot cast events->event_groups[0][0] to
//        PatternMultiStep!";
//    EXPECT_EQ(4, multi->values_.size());
//}

}  // namespace
