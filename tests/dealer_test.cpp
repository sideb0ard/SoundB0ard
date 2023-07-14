#include "dealer.h"

#include <gtest/gtest.h>

class DealerTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  SBAudio::Dealer dealer_;
};

TEST_F(DealerTest, DealerInitialSettings) {
  EXPECT_EQ(dealer_.step_, 1);
  EXPECT_EQ(dealer_.idx_, 0);
  EXPECT_EQ(dealer_.sequence_.size(), 0);
}

TEST_F(DealerTest, EmptySequence) { EXPECT_EQ(dealer_.GenNext(), 0); }

TEST_F(DealerTest, SingleItem) {
  dealer_.SetSequence({1});

  EXPECT_EQ(dealer_.sequence_.size(), 1);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(dealer_.GenNext(), 1);
  }
}

TEST_F(DealerTest, SequenceOfTwoStepOneBounceBack) {
  dealer_.SetSequence({0, 1});

  EXPECT_EQ(dealer_.sequence_.size(), 2);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(dealer_.GenNext(), i % 2);
  }
}
