#include <algorithm>
#include <array>
#include <iostream>

#include <pattern_functions.hpp>

void PatternEvery::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    std::cout << "PATTERN EVERY TRANFORMR CALLED!\n";
    if (loop_num % every_n_ == 0)
    {
        std::cout << "TRANSFORMMMMA!\n";
        func_->TransformPattern(events, loop_num);
    }
    else
        std::cout << "SKIPPINNNNN!\n";
}

std::string PatternEvery::String() const { return "PatternEvery"; }

void PatternReverse::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    std::cout << "PATTERN REVERSE TRANSFORM CALLED!\n";
    std::reverse(events.begin(), events.end());
}

std::string PatternReverse::String() const { return "PatternReeeeeverse"; }
