#include <gtest/gtest.h>

#include "stepper.h"

class StepperTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  SBAudio::Stepper stepper_;
};

TEST_F(StepperTest, StepperInitialSettings) {
  EXPECT_EQ(stepper_.step_, 1);
  EXPECT_EQ(stepper_.idx_, 0);
  EXPECT_EQ(stepper_.sequence_.size(), 0);
}

TEST_F(StepperTest, EmptySequence) { EXPECT_EQ(stepper_.GenNext(), 0); }

TEST_F(StepperTest, SingleItem) {
  stepper_.SetSequence({1});

  EXPECT_EQ(stepper_.sequence_.size(), 1);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(stepper_.GenNext(), 1);
  }
}

TEST_F(StepperTest, SequenceOfTwoBounceBack) {
  stepper_.SetSequence({0, 1});
  EXPECT_EQ(stepper_.sequence_.size(), 2);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(stepper_.GenNext(), i % 2);
  }
}

TEST_F(StepperTest, SequenceOfTwoBounceBackStep2) {
  stepper_.SetSequence({0, 1});
  stepper_.step_ = 2;
  EXPECT_EQ(stepper_.sequence_.size(), 2);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(stepper_.GenNext(), 0);
  }
}

TEST_F(StepperTest, SequenceOfTwoCycleRound) {
  stepper_.SetSequence({0, 1});
  stepper_.behavior_ = SBAudio::BoundaryBehavior::CycleRound;

  EXPECT_EQ(stepper_.sequence_.size(), 2);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(stepper_.GenNext(), i % 2);
  }
}

TEST_F(StepperTest, SequenceOfTwoCycleRoundStep2) {
  stepper_.SetSequence({0, 1});
  stepper_.behavior_ = SBAudio::BoundaryBehavior::CycleRound;
  stepper_.step_ = 2;

  EXPECT_EQ(stepper_.sequence_.size(), 2);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(stepper_.GenNext(), 0);
  }
}

/////////// 3 ///////////////////////
TEST_F(StepperTest, SequenceOfThreeBounceBack) {
  stepper_.SetSequence({0, 1, 2});
  EXPECT_EQ(stepper_.sequence_.size(), 3);
  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 1);
  EXPECT_EQ(stepper_.GenNext(), 2);
  EXPECT_EQ(stepper_.GenNext(), 1);
  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 1);
  EXPECT_EQ(stepper_.GenNext(), 2);
}

TEST_F(StepperTest, SequenceOfThreeBounceBackStep2) {
  stepper_.SetSequence({0, 1, 2});
  stepper_.step_ = 2;
  EXPECT_EQ(stepper_.sequence_.size(), 3);
  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 2);
  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 2);
}

TEST_F(StepperTest, SequenceOfThreeBounceBackStep3) {
  stepper_.SetSequence({0, 1, 2});
  stepper_.step_ = 3;
  EXPECT_EQ(stepper_.sequence_.size(), 3);
  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 1);
  EXPECT_EQ(stepper_.GenNext(), 2);
  EXPECT_EQ(stepper_.GenNext(), 1);
  EXPECT_EQ(stepper_.GenNext(), 0);
}

TEST_F(StepperTest, SequenceOfThreeCycleRound) {
  stepper_.SetSequence({0, 1, 2});
  stepper_.behavior_ = SBAudio::BoundaryBehavior::CycleRound;

  EXPECT_EQ(stepper_.sequence_.size(), 3);
  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 1);
  EXPECT_EQ(stepper_.GenNext(), 2);
  EXPECT_EQ(stepper_.GenNext(), 0);
}

TEST_F(StepperTest, SequenceOfThreeCycleRoundStep2) {
  stepper_.SetSequence({0, 1, 2});
  stepper_.behavior_ = SBAudio::BoundaryBehavior::CycleRound;
  stepper_.step_ = 2;

  EXPECT_EQ(stepper_.sequence_.size(), 3);

  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 2);
  EXPECT_EQ(stepper_.GenNext(), 1);
  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 2);
}

TEST_F(StepperTest, SequenceOfTenBounceBack) {
  stepper_.SetSequence({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

  EXPECT_EQ(stepper_.sequence_.size(), 10);
  for (int i = 0; i < 11; i++) {
    stepper_.GenNext();
  }
  EXPECT_EQ(stepper_.GenNext(), 7);
}

TEST_F(StepperTest, SequenceOfTenCycleRound) {
  stepper_.SetSequence({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  stepper_.behavior_ = SBAudio::BoundaryBehavior::CycleRound;

  EXPECT_EQ(stepper_.sequence_.size(), 10);
  for (int i = 0; i < 20; i++) {
    EXPECT_EQ(stepper_.GenNext(), i % 10);
  }
}

TEST_F(StepperTest, SequenceOfTenBounceBackStep2) {
  stepper_.SetSequence({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  stepper_.step_ = 2;

  EXPECT_EQ(stepper_.sequence_.size(), 10);
  for (int i = 0; i < 8; i++) {
    stepper_.GenNext();
  }
  EXPECT_EQ(stepper_.GenNext(), 2);
}

TEST_F(StepperTest, SequenceOfTenCycleRoundStep2) {
  stepper_.SetSequence({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  stepper_.behavior_ = SBAudio::BoundaryBehavior::CycleRound;
  stepper_.step_ = 2;

  EXPECT_EQ(stepper_.sequence_.size(), 10);
  for (int i = 0; i < 8; i++) {
    stepper_.GenNext();
  }
  EXPECT_EQ(stepper_.GenNext(), 6);
}

TEST_F(StepperTest, SequenceOfTenBounceBackStep3) {
  stepper_.SetSequence({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  stepper_.step_ = 3;

  EXPECT_EQ(stepper_.sequence_.size(), 10);

  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 3);
  EXPECT_EQ(stepper_.GenNext(), 6);
  EXPECT_EQ(stepper_.GenNext(), 9);
  EXPECT_EQ(stepper_.GenNext(), 6);
  EXPECT_EQ(stepper_.GenNext(), 3);
  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 3);
}

TEST_F(StepperTest, SequenceOfTenCycleRoundStep3) {
  stepper_.SetSequence({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  stepper_.behavior_ = SBAudio::BoundaryBehavior::CycleRound;
  stepper_.step_ = 3;

  EXPECT_EQ(stepper_.sequence_.size(), 10);

  EXPECT_EQ(stepper_.GenNext(), 0);
  EXPECT_EQ(stepper_.GenNext(), 3);
  EXPECT_EQ(stepper_.GenNext(), 6);
  EXPECT_EQ(stepper_.GenNext(), 9);
  EXPECT_EQ(stepper_.GenNext(), 2);
  EXPECT_EQ(stepper_.GenNext(), 5);
}
