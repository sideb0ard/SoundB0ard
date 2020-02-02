#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

#include <pattern_functions.hpp>

namespace
{

void PrintPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &pattern)
{
    for (int i = 0; i < PPBAR; i++)
    {
        if (pattern[i].size() > 0)
            std::cout << i << std::endl;
    }
}

} // namespace

void PatternEvery::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    if (loop_num % every_n_ == 0)
        func_->TransformPattern(events, loop_num);
}

std::string PatternEvery::String() const
{
    std::stringstream ss;
    ss << "every " << every_n_ << " " << func_->String();
    return ss.str();
}

void PatternReverse::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    std::reverse(events.begin(), events.end());
    std::rotate(events.begin(), events.begin() + (PPSIXTEENTH - 1),
                events.end());
}

std::string PatternReverse::String() const { return "rev"; }

void PatternTranspose::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    int num_midi_notes_to_adjust = num_octaves_ * 12;
    for (int i = 0; i < PPBAR; i++)
    {
        if (events[i].size() > 0)
        {
            std::vector<std::shared_ptr<MusicalEvent>> &mevents = events[i];
            for (auto &e : mevents)
            {
                if (e->target_type_ == ProcessPatternTarget::VALUES)
                {
                    int str_to_val = std::stoi(e->value_);
                    if (direction_ == UP)
                        str_to_val += num_midi_notes_to_adjust;
                    else
                        str_to_val -= num_midi_notes_to_adjust;
                    e->value_ = std::to_string(str_to_val);
                }
            }
        }
    }
}
std::string PatternTranspose::String() const
{

    std::stringstream ss;
    if (direction_ == UP)
        ss << "up ";
    else
        ss << "down ";
    ss << num_octaves_;
    return ss.str();
}

void PatternRotate::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    // std::cout << "ROTATTRRRR! " << direction_ << ":" << num_sixteenth_steps_
    //          << std::endl;
    // std::cout << "B4:\n";
    // PrintPattern(events);
    if (direction_ == LEFT)
        std::rotate(events.begin(),
                    events.begin() + (PPSIXTEENTH * num_sixteenth_steps_),
                    events.end());
    else if (direction_ == RIGHT)
        std::rotate(events.begin(),
                    events.begin() +
                        (PPBAR - (PPSIXTEENTH * num_sixteenth_steps_)),
                    events.end());
    // std::cout << "AFTER:\n";
    // PrintPattern(events);
}
std::string PatternRotate::String() const
{
    std::stringstream ss;
    if (direction_ == LEFT)
        ss << "rotl ";
    else
        ss << "rotr ";
    ss << num_sixteenth_steps_;
    return ss.str();
}

void PatternSwing::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num) const
{
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> new_events;
    bool even16th = true;
    for (int i = 0; i < PPBAR; i += PPSIXTEENTH)
    {
        for (int j = 0; j < PPSIXTEENTH; j++)
        {
            int idx = i + j;
            if (events[idx].size() > 0)
            {
                if (even16th)
                {
                    // clean copy
                    new_events[idx] = events[idx];
                }
                else
                {
                    int new_idx =
                        idx + swing_setting_ * 19; // TODO magic number 19 midi
                                                   // ticks per 4% swing
                    while (new_idx < 0)
                        new_idx = PPBAR - new_idx;
                    while (new_idx >= PPBAR)
                        new_idx = new_idx - PPBAR;
                    new_events[new_idx] = events[idx];
                }
            }
        }
        even16th = 1 - even16th;
    }
    // PrintPattern(new_events);
    events = new_events;
}
std::string PatternSwing::String() const
{

    std::stringstream ss;
    ss << "swing ";
    ss << swing_setting_;
    return ss.str();
}
