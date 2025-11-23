// cppcheck-suppress-file syntaxError
#include "fx/ramp.h"

#include <gtest/gtest.h>

#include <iostream>

class RampTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  Ramp ramp_;
};

TEST_F(RampTest, RampInitialSettings) {
  EXPECT_EQ(ramp_.counter_, 0);
  EXPECT_EQ(ramp_.frames_per_second_, SAMPLE_RATE);
  EXPECT_EQ(ramp_.signal_, 0);
}

TEST_F(RampTest, RampMaxVal) {
  const int frames_per_sec = 16;
  ramp_.Reset(frames_per_sec);

  double val = 0;
  for (int i = 0; i < frames_per_sec; ++i) {
    val = ramp_.Generate();
    std::cout << "RAMP i:" << i << ":" << val << std::endl;
  }
  // Ramp generates values 0, 1/n, 2/n, ..., (n-1)/n, then wraps
  // After 16 iterations, val = 15/16 = 0.9375
  EXPECT_DOUBLE_EQ(val, 15.0 / 16.0);
}
