#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <sstream>

#include "cmdloop.h"
#include "mixer.h"
#include "process.hpp"
#include "utils.h"
#include <looper.h>
#include <tsqueue.hpp>

#include <pattern_parser/tokenizer.hpp>

extern mixer *mixr;

using Wrapper =
    std::pair<std::shared_ptr<ast::Node>, std::shared_ptr<object::Environment>>;
extern Tsqueue<Wrapper> g_queue;
extern std::shared_ptr<object::Environment> global_env;

void Process::ParsePattern()
{
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern_);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    pattern_root_ = pattern_parzer->ParsePattern();
}

Process::Process(std::string target, std::string pattern)
    : target_{target}, pattern_{pattern}, active_{true}
{
    ParsePattern();
}

Process::~Process() { std::cout << "Mixer Process deid!\n"; }

void Process::EventNotify(mixer_timing_info tinfo)
{
    if (!active_)
        return;

    if (tinfo.is_start_of_loop)
    {
        for (int i = 0; i < PPBAR; i++)
            pattern_events_[i].clear();
        EvalPattern(pattern_root_, 0, PPBAR);
        ++loop_counter_;
    }

    int cur_tick = tinfo.midi_tick % PPBAR;
    std::vector<std::shared_ptr<MusicalEvent>> &events =
        pattern_events_[cur_tick];
    for (auto e : events)
    {
        std::string cmd =
            std::string("noteOn(") + e->target_ + "," + "127, 250)";

        Interpret(cmd.data(), global_env);
    }
}

void Process::EvalPattern(std::shared_ptr<pattern_parser::PatternNode> &node,
                          int target_start, int target_end)
{

    int divisor = node->GetDivisor();
    if (divisor && (loop_counter_ % divisor != 0))
        return;

    std::shared_ptr<pattern_parser::PatternLeaf> leaf_node =
        std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(node);
    if (leaf_node)
    {
        std::string target = leaf_node->value_;
        pattern_events_[target_start].push_back(
            std::make_shared<MusicalEvent>(target));
        return;
    }

    std::shared_ptr<pattern_parser::PatternGroup> composite_node =
        std::dynamic_pointer_cast<pattern_parser::PatternGroup>(node);
    if (composite_node)
    {
        int event_groups_size = composite_node->event_groups_.size();
        for (int i = 0; i < event_groups_size; i++)
        {

            std::vector<std::shared_ptr<pattern_parser::PatternNode>> &events =
                composite_node->event_groups_[i];
            int target_len = target_end - target_start;
            int num_events = events.size();
            int spacing = target_len / num_events;

            for (int j = target_start, event_idx = 0;
                 j < target_end && event_idx < num_events;
                 j += spacing, event_idx++)
            {
                std::shared_ptr<pattern_parser::PatternNode> sub_node =
                    events[event_idx];
                EvalPattern(sub_node, j, j + spacing);
            }
        }
    }
}

void Process::Status(wchar_t *status_string)
{
    const char *PROC_COLOR = ANSI_COLOR_RESET;
    if (active_)
        PROC_COLOR = COOL_COLOR_PINK;

    swprintf(status_string, MAX_STATIC_STRING_SZ,
             WANSI_COLOR_WHITE "%sProcess: Target:%s Pattern:%s Active:%s",
             PROC_COLOR, target_.c_str(), pattern_.c_str(),
             active_ ? "true" : "false");
    wcscat(status_string, WANSI_COLOR_RESET);
}

void Process::Start() { active_ = true; }
void Process::Stop() { active_ = false; }

void Process::SetDebug(bool b) { debug_ = b; }
