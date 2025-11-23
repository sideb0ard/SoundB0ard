// cppcheck-suppress-file syntaxError
#include "interpreter/object.hpp"

#include <memory>

#include "gtest/gtest.h"

namespace {

struct ObjectTest : public ::testing::Test {};

TEST_F(ObjectTest, TestStringHashKey) {
  auto hello1 = std::make_shared<object::String>("Hello World");
  auto hello2 = std::make_shared<object::String>("Hello World");
  auto diff1 = std::make_shared<object::String>("My name is johnny");
  auto diff2 = std::make_shared<object::String>("My name is johnny");
  EXPECT_EQ(hello1->GetHashKey(), hello2->GetHashKey());
  EXPECT_EQ(diff1->GetHashKey(), diff2->GetHashKey());
  EXPECT_NE(hello1->GetHashKey(), diff1->GetHashKey());
}

TEST_F(ObjectTest, TestBooleanHashKey) {
  auto true1 = std::make_shared<object::Boolean>(true);
  auto true2 = std::make_shared<object::Boolean>(true);
  auto false1 = std::make_shared<object::Boolean>(false);
  auto false2 = std::make_shared<object::Boolean>(false);
  EXPECT_EQ(true1->GetHashKey(), true2->GetHashKey());
  EXPECT_EQ(false1->GetHashKey(), false2->GetHashKey());
  EXPECT_NE(true1->GetHashKey(), false1->GetHashKey());
}

TEST_F(ObjectTest, TestNumberHashKey) {
  auto one1 = std::make_shared<object::Number>(1);
  auto one2 = std::make_shared<object::Number>(1);
  auto two1 = std::make_shared<object::Number>(2);
  auto two2 = std::make_shared<object::Number>(2);
  EXPECT_EQ(one1->GetHashKey(), one2->GetHashKey());
  EXPECT_EQ(two1->GetHashKey(), two2->GetHashKey());
  EXPECT_NE(one1->GetHashKey(), two1->GetHashKey());
}

}  // namespace
