#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

#include <audioutils.h>
#include <midimaaan.h>
#include <pattern_functions.hpp>
#include <utils.h>

namespace
{

void PrintPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &pattern)
{
    for (int i = 0; i < PPBAR; i++)
    {
        if (pattern[i].size() > 0)
        {
            std::cout << "[" << i << "] ";
            for (auto &e : pattern[i])
            {
                int str_to_val = 0;
                if (IsNote(e->value_))
                    str_to_val = get_midi_note_from_string(&e->value_[0]);
                else
                    str_to_val = std::stoi(e->value_);
                std::cout << str_to_val << " ";
            }
            std::cout << std::endl;
        }
    }
}

bool IsArpNote(int i, ArpSpeed speed)
{
    switch (speed)
    {
    case ArpSpeed::ARP_16:
        if (i % PPSIXTEENTH == 0)
            return true;
        break;
    case ArpSpeed::ARP_8:
        if (i % (PPSIXTEENTH * 2) == 0)
            return true;
        break;
    case ArpSpeed::ARP_4:
        if (i % PPQN == 0)
            return true;
        break;
    }

    return false;
}

} // namespace

void PatternEvery::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
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
    int loop_num)
{
    std::reverse(events.begin(), events.end());
    std::rotate(events.begin(), events.begin() + (PPSIXTEENTH - 1),
                events.end());
}

std::string PatternReverse::String() const { return "rev"; }

void PatternTranspose::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
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
                    int str_to_val = 60;
                    if (IsNote(e->value_))
                        str_to_val = get_midi_note_from_string(&e->value_[0]);
                    else
                        str_to_val = std::stoi(e->value_);
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
    int loop_num)
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
    int loop_num)
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

PatternMask::PatternMask(std::string mask) : mask_{mask}
{
    for (int i = 0; i < 4; i++)
    {
        char c = mask_[i];
        std::cout << "Mask char is " << c << std::endl;
        int shift_amount = (15 - (i * 4));
        int bin_rep = 0;
        if (c == 'f')
            bin_rep = 0b1111;
        else if (c == 'e')
            bin_rep = 0b1110;
        else if (c == 'd')
            bin_rep = 0b1101;
        else if (c == 'c')
            bin_rep = 0b1100;
        else if (c == 'b')
            bin_rep = 0b1011;
        else if (c == 'a')
            bin_rep = 0b1010;
        else
            bin_rep = c - '0';

        for (int j = 0; j < 4; j++)
            if (bin_rep & 1 << (3 - j))
            {
                bin_mask_ |= 1 << (shift_amount - j);
            }
    }
    auto bit_mask_string = bin_num_to_string(bin_mask_);
    std::cout << "BIT MASK is " << bit_mask_string << std::endl;
}
void PatternMask::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
{
    for (int i = 0; i < 16; i++)
    {
        int shift_amount = 15 - i;
        if (bin_mask_ & 1 << shift_amount)
        {
            int start_idx = i * PPSIXTEENTH;
            int end_idx = start_idx + PPSIXTEENTH;
            for (int j = start_idx; j < end_idx; j++)
                events[j].clear();
        }
    }
}

std::string PatternMask::String() const
{

    std::stringstream ss;
    ss << "mask ";
    ss << "\"" << mask_ << "\"";
    return ss.str();
}

void PatternArp::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
{
    int counter = 0;
    for (int i = 0; i < PPBAR; i += PPSIXTEENTH)
    {
        std::vector<std::shared_ptr<MusicalEvent>> &mevents = events[i];
        if (events[i].size() > 0)
        {
            if (mevents.size() > 0)
            {
                auto midi_event = mevents[0];
                if (midi_event->target_type_ == ProcessPatternTarget::VALUES)
                {
                    if (IsNote(midi_event->value_))
                        last_midi_note_ =
                            get_midi_note_from_string(&midi_event->value_[0]);
                    else
                        last_midi_note_ = std::stoi(midi_event->value_);
                    counter = 0;
                }
            }
        }
        else
        {
            if (last_midi_note_ != 0)
            {
                if (IsArpNote(i, speed_))
                {
                    int new_midi_note{0};
                    if (direction_ == ArpDirection::ARP_UP)
                    {
                        if ((counter % 3) == 0)
                            new_midi_note = last_midi_note_ + 4;
                        else if ((counter % 3) == 1)
                            new_midi_note = last_midi_note_ + 7;
                        else
                            new_midi_note = last_midi_note_;
                    }
                    else if (direction_ == ArpDirection::ARP_DOWN)
                    {
                        if ((counter % 3) == 0)
                            new_midi_note = last_midi_note_ + 7;
                        else if ((counter % 3) == 1)
                            new_midi_note = last_midi_note_ + 4;
                        else
                            new_midi_note = last_midi_note_;
                    }
                    else if (direction_ == ArpDirection::ARP_RAND)
                    {
                        int randy = rand() % 3;
                        if (randy == 0)
                            new_midi_note = last_midi_note_ + 7;
                        else if (randy == 1)
                            new_midi_note = last_midi_note_ + 4;
                        else
                            new_midi_note = last_midi_note_;
                    }
                    else if (direction_ == ArpDirection::ARP_REPEAT)
                        new_midi_note = last_midi_note_;

                    if (new_midi_note)
                    {
                        auto new_event = std::make_shared<MusicalEvent>(
                            std::to_string(new_midi_note),
                            ProcessPatternTarget::VALUES);
                        mevents.push_back(new_event);
                    }
                    counter++;
                }
            }
        }
    }
    if (last_midi_note_ != 0)
    {
    }
}

std::string PatternArp::String() const
{

    std::stringstream ss;
    ss << "arp";
    return ss.str();
}

void PatternBrak::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
{
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> new_events;
    for (int i = 0; i < PPBAR; i++)
    {
        if (events[i].size() > 0)
            new_events[i / 2] = events[i];
    }
    std::rotate(new_events.begin(), new_events.begin() + ((PPBAR / 2) - PPQN),
                new_events.end());
    events = new_events;
}

std::string PatternBrak::String() const
{

    std::stringstream ss;
    ss << "brak";
    return ss.str();
}

void PatternFast::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
{
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> new_events;
    for (int i = 0; i < PPBAR; i++)
    {
        if (events[i].size() > 0)
            new_events[i / 2] = events[i];
    }

    for (int i = 0; i < PPBAR / 2; i++)
    {
        if (new_events[i].size() > 0)
            std::copy(new_events[i].begin(), new_events[i].end(),
                      std::back_inserter(new_events[i + (PPBAR / 2)]));
    }

    events = new_events;
}

std::string PatternFast::String() const
{

    std::stringstream ss;
    ss << "fast";
    return ss.str();
}

void PatternSlow::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
{
}

std::string PatternSlow::String() const
{

    std::stringstream ss;
    ss << "slow";
    return ss.str();
}

void PatternChord::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
{
    for (int i = 0; i < PPBAR; i++)
    {
        if (events[i].size() > 0)
        {
            std::vector<std::shared_ptr<MusicalEvent>> &mevents = events[i];
            std::vector<std::shared_ptr<MusicalEvent>> new_events;
            for (auto &e : mevents)
            {
                if (e->target_type_ == ProcessPatternTarget::VALUES)
                {
                    new_events.push_back(e);
                    std::string midistring = e->value_;
                    if (IsNote(e->value_))
                    {
                        midistring = std::to_string(
                            get_midi_note_from_string(&e->value_[0]));
                    }
                    int third = GetThird(std::stoi(midistring), 'c');

                    std::shared_ptr<MusicalEvent> new_ev =
                        std::make_shared<MusicalEvent>(
                            std::to_string(third), e->velocity_, e->duration_,
                            e->target_type_);
                    new_events.push_back(new_ev);

                    int fifth = GetFifth(std::stoi(midistring), 'c');

                    new_ev = std::make_shared<MusicalEvent>(
                        std::to_string(fifth), e->velocity_, e->duration_,
                        e->target_type_);
                    new_events.push_back(new_ev);
                }
            }
            events[i] = new_events;
        }
    }
    // PrintPattern(events);
}

std::string PatternChord::String() const
{

    std::stringstream ss;
    ss << "chord";
    return ss.str();
}

void PatternPowerChord::TransformPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events,
    int loop_num)
{
    for (int i = 0; i < PPBAR; i++)
    {
        if (events[i].size() > 0)
        {
            std::vector<std::shared_ptr<MusicalEvent>> &mevents = events[i];
            std::vector<std::shared_ptr<MusicalEvent>> new_events;
            for (auto &e : mevents)
            {
                if (e->target_type_ == ProcessPatternTarget::VALUES)
                {
                    new_events.push_back(e);
                    std::string midistring = e->value_;
                    if (IsNote(e->value_))
                    {
                        midistring = std::to_string(
                            get_midi_note_from_string(&e->value_[0]));
                    }
                    int fifth = GetFifth(std::stoi(midistring), 'c');

                    std::shared_ptr<MusicalEvent> new_ev =
                        std::make_shared<MusicalEvent>(
                            std::to_string(fifth), e->velocity_, e->duration_,
                            e->target_type_);
                    new_events.push_back(new_ev);
                }
            }
            events[i] = new_events;
        }
    }
    // PrintPattern(events);
}

std::string PatternPowerChord::String() const
{

    std::stringstream ss;
    ss << "power";
    return ss.str();
}
