#include <algorithm>
#include <array>
#include <iostream>

#include <pattern_functions.hpp>

void PatternEvery::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    if (loop_num % every_n_ == (every_n_ - 1))
        func_->TransformPattern(events, loop_num);
}

std::string PatternEvery::String() const { return "PatternEvery"; }

void PatternReverse::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    std::reverse(events.begin(), events.end());
    std::rotate(events.begin(), events.begin() + (PPSIXTEENTH - 1),
                events.end());
}

std::string PatternReverse::String() const { return "PatternReeeeeverse"; }

void PatternTranspose::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
}
std::string PatternTranspose::String() const { return "PatternTranzzzpose"; }

void PatternRotate::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
}
std::string PatternRotate::String() const { return "PatternRoooootate!"; }
