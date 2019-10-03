#pragma once

#include <stdbool.h>
#include <wchar.h>

#include <iostream>

#include "defjams.h"

constexpr int MAX_CMDS = 10;
constexpr int MAX_CMD_LEN = 4096;

constexpr int MAX_VAR_KEY_LEN = 128;
constexpr int MAX_VAR_VAL_LEN = 128;

constexpr int MAX_LIST_ITEMS = 64;

// enum
//{
//    INC_OP,
//    SUB_OP,
//    MULTI_OP,
//    DIV_OP,
//    RAND_OP,
//};
//
// enum
//{
//    VAR_RAND,
//    VAR_OSC,
//    VAR_STEP,
//    VAR_FOR,
//    MAX_VAR_SELECT_TYPES,
//};

// timer_type
enum
{
    EVERY,
    OVER,
    FOR,
    MAX_ALGO_PROCESS_TYPE,
};

class Timer
{
  public:
    Timer(unsigned int timer_type, unsigned int event_type, unsigned int step)
        : timer_type_{timer_type}, event_type_{event_type}, step_{step}
    {
        active_ = true;
        step_counter_ = 0;
        debug_ = false;

        std::cout << "YO, I BEEN TIMING! " << event_type << " " << timer_type
                  << " " << step << std::endl;
    }
    ~Timer() = default;
    void Status(wchar_t *ss);
    void Start();
    void Stop();
    void EventNotify(mixer_timing_info);
    void Eval();
    void SetDebug(bool b);

    unsigned int timer_type_; // EVERY, OVER
    unsigned int event_type_; // e.g. MIDI_TICK, SIXTEENTH, BAR
    unsigned int step_;
    unsigned int step_counter_;

    unsigned int counter_;         // currently just used for FOR increments
    unsigned int var_select_type_; // RAND, OSC or STEP

    bool active_;
    bool debug_;
};
