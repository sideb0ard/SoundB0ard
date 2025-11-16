// cppcheck-suppress-file syntaxError
#include <iostream>
#include <pattern_parser/euclidean.hpp>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace {

struct EuclideanTest : public ::testing::Test {};

TEST_F(EuclideanTest, Test3_8Pattern) {
  std::string euclidean_3_8_string = generate_euclidean_string(3, 8);
  EXPECT_EQ("10010010", euclidean_3_8_string);
}

TEST_F(EuclideanTest, Test5_8Pattern) {
  std::string euclidean_5_8_string = generate_euclidean_string(5, 8);
  EXPECT_EQ("10110110", euclidean_5_8_string);
}

TEST_F(EuclideanTest, Test5_13Pattern) {
  std::string euclidean_5_13_string = generate_euclidean_string(5, 13);
  EXPECT_EQ("1001010010100", euclidean_5_13_string);
}

}  // namespace
