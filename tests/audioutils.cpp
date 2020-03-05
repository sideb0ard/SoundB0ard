#include <iostream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include <audioutils.h>

namespace
{

struct AudioUtilsTest : public ::testing::Test
{
};

TEST_F(AudioUtilsTest, TestCDegrees)
{

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

TEST_F(AudioUtilsTest, TestDDegrees)
{

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

TEST_F(AudioUtilsTest, TestPerfectFifth)
{

    int midi_F = 65;
    int perfect_fifth = GetNthDegree(midi_F, 4, 'c');
    EXPECT_EQ(72, perfect_fifth);

    int midi_C = 72;
    perfect_fifth = GetNthDegree(midi_C, 4, 'c');
    EXPECT_EQ(79, perfect_fifth);

    // key of 'D'
    int midi_Csharp = 73;
    perfect_fifth = GetNthDegree(midi_Csharp, 4, 'd');
    EXPECT_EQ(79, perfect_fifth);

    perfect_fifth = GetFifth(midi_Csharp, 'd');
    EXPECT_EQ(79, perfect_fifth);
}

} // namespace
