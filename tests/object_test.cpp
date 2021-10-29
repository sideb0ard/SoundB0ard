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
  EXPECT_EQ(hello1->HashKey(), hello2->HashKey());
  EXPECT_EQ(diff1->HashKey(), diff2->HashKey());
  EXPECT_NE(hello1->HashKey(), diff1->HashKey());
}

TEST_F(ObjectTest, TestBooleanHashKey) {
  auto true1 = std::make_shared<object::Boolean>(true);
  auto true2 = std::make_shared<object::Boolean>(true);
  auto false1 = std::make_shared<object::Boolean>(false);
  auto false2 = std::make_shared<object::Boolean>(false);
  EXPECT_EQ(true1->HashKey(), true2->HashKey());
  EXPECT_EQ(false1->HashKey(), false2->HashKey());
  EXPECT_NE(true1->HashKey(), false1->HashKey());
}

TEST_F(ObjectTest, TestNumberHashKey) {
  auto one1 = std::make_shared<object::Number>(1);
  auto one2 = std::make_shared<object::Number>(1);
  auto two1 = std::make_shared<object::Number>(2);
  auto two2 = std::make_shared<object::Number>(2);
  EXPECT_EQ(one1->HashKey(), one2->HashKey());
  EXPECT_EQ(two1->HashKey(), two2->HashKey());
  EXPECT_NE(one1->HashKey(), two1->HashKey());
}

}  // namespace
