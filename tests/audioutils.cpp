#include <audioutils.h>

#include <iostream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace {

struct AudioUtilsTest : public ::testing::Test {};

TEST_F(AudioUtilsTest, TestCDegrees) {
  int midi_C = 60;

  int midi_D = GetNthDegree(midi_C, 1, 'c');
  EXPECT_EQ(62, midi_D);

  int midi_E = GetNthDegree(midi_C, 2, 'c');
  EXPECT_EQ(64, midi_E);

  int midi_F = GetNthDegree(midi_C, 3, 'c');
  EXPECT_EQ(65, midi_F);

  int midi_G = GetNthDegree(midi_C, 4, 'c');
  EXPECT_EQ(67, midi_G);

  int midi_A = GetNthDegree(midi_C, 5, 'c');
  EXPECT_EQ(69, midi_A);

  int midi_B = GetNthDegree(midi_C, 6, 'c');
  EXPECT_EQ(71, midi_B);

  int midi_next_C = GetNthDegree(midi_C, 7, 'c');
  EXPECT_EQ(72, midi_next_C);
}

TEST_F(AudioUtilsTest, TestDDegrees) {
  int midi_D = 62;

  int midi_E = GetNthDegree(midi_D, 1, 'd');
  EXPECT_EQ(64, midi_E);

  int midi_Fsharp = GetNthDegree(midi_D, 2, 'd');
  EXPECT_EQ(66, midi_Fsharp);

  int midi_G = GetNthDegree(midi_D, 3, 'd');
  EXPECT_EQ(67, midi_G);

  int midi_A = GetNthDegree(midi_D, 4, 'd');
  EXPECT_EQ(69, midi_A);

  int midi_B = GetNthDegree(midi_D, 5, 'd');
  EXPECT_EQ(71, midi_B);

  int midi_Csharp = GetNthDegree(midi_D, 6, 'd');
  EXPECT_EQ(73, midi_Csharp);

  int midi_next_D = GetNthDegree(midi_D, 7, 'd');
  EXPECT_EQ(74, midi_next_D);
}

TEST_F(AudioUtilsTest, TestPerfectFifthC) {
  // c
  int perfect_fifth = GetNthDegree(60, 4, 'c');
  EXPECT_EQ(67, perfect_fifth);

  // d
  perfect_fifth = GetNthDegree(62, 4, 'c');
  EXPECT_EQ(69, perfect_fifth);

  // e
  perfect_fifth = GetNthDegree(64, 4, 'c');
  EXPECT_EQ(71, perfect_fifth);

  // f
  perfect_fifth = GetNthDegree(65, 4, 'c');
  EXPECT_EQ(72, perfect_fifth);

  // g
  perfect_fifth = GetNthDegree(67, 4, 'c');
  EXPECT_EQ(74, perfect_fifth);

  // a
  perfect_fifth = GetNthDegree(69, 4, 'c');
  EXPECT_EQ(76, perfect_fifth);

  // b
  perfect_fifth = GetNthDegree(71, 4, 'c');
  EXPECT_EQ(77, perfect_fifth);

  // c
  perfect_fifth = GetNthDegree(72, 4, 'c');
  EXPECT_EQ(79, perfect_fifth);
}

TEST_F(AudioUtilsTest, TestPerfectFifthD) {
  //  d
  int perfect_fifth = GetNthDegree(62, 4, 'd');
  EXPECT_EQ(69, perfect_fifth);

  // e
  perfect_fifth = GetFifth(64, 'd');
  EXPECT_EQ(71, perfect_fifth);

  // f#
  perfect_fifth = GetFifth(66, 'd');
  EXPECT_EQ(73, perfect_fifth);

  // g
  perfect_fifth = GetFifth(67, 'd');
  EXPECT_EQ(74, perfect_fifth);

  // a
  perfect_fifth = GetFifth(69, 'd');
  EXPECT_EQ(76, perfect_fifth);

  // b
  perfect_fifth = GetFifth(71, 'd');
  EXPECT_EQ(78, perfect_fifth);

  // c#
  perfect_fifth = GetFifth(73, 'd');
  EXPECT_EQ(79, perfect_fifth);

  // d
  // perfect_fifth = GetFifth(74, 'd');
  // EXPECT_EQ(81, perfect_fifth);
}

TEST_F(AudioUtilsTest, TestNotesInChord) {}

}  // namespace
