tests / ramp_test.cpp
#include "fx/ramp.h"

#include <gtest/gtest.h>

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
  EXPECT_EQ(val, 1);
}
