#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include <defjams.h>
#include <pattern_functions.hpp>

namespace
{

class PatternFunctionsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        std::shared_ptr<MusicalEvent> new_ev = std::make_shared<MusicalEvent>(
            "c", 127, 300, ProcessPatternTarget::VALUES);
        events[0].push_back(new_ev);
    }
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> events;
};

TEST_F(PatternFunctionsTest, Test)
{
    std::cout << "[BEFORE] Size of EVENTS:" << events[0].size() << std::endl;

    PatternPowerChord pfunk;
    pfunk.TransformPattern(events, 1);

    EXPECT_EQ(2, events[0].size());

    // std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> tevents;
    // std::shared_ptr<MusicalEvent> new_ev = std::make_shared<MusicalEvent>(
    //    "c", 127, 300, ProcessPatternTarget::VALUES);
    // tevents[0].push_back(new_ev);
    // new_ev = std::make_shared<MusicalEvent>("67", 127, 300,
    //                                        ProcessPatternTarget::VALUES);
    // tevents[0].push_back(new_ev);

    // EXPECT_TRUE(tevents == events);

    std::cout << "[AFTER] Size of EVENTS:" << events[0].size() << std::endl;
}

} // namespace