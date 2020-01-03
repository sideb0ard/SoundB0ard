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
#include <pattern_utils.h>
#include <tsqueue.hpp>

#include <pattern_parser/euclidean.hpp>
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

void Process::Update(std::string target, std::string pattern,
                     std::vector<std::shared_ptr<PatternFunction>> funcz)
{
    active_ = false;
    target_ = target;
    pattern_ = pattern;
    ParsePattern();

    for (auto &oldfz : pattern_functions_)
        oldfz->active_ = false;
    for (auto fz : funcz)
        pattern_functions_.push_back(fz);
    active_ = true;
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

        // std::cout << "BEFORE:" << PatternPrint(pattern_events_) << std::endl;
        for (auto &f : pattern_functions_)
        {
            if (f->active_)
                f->TransformPattern(pattern_events_, loop_counter_);
            // std::cout << "AFTER:" << PatternPrint(pattern_events_) <<
            // std::endl;
        }

        ++loop_counter_;
    }

    int cur_tick = tinfo.midi_tick % PPBAR;
    if (pattern_events_[cur_tick].size() == 0)
        return;

    std::vector<std::shared_ptr<MusicalEvent>> &events =
        pattern_events_[cur_tick];

    if (target_ == "sample")
    {
        for (auto e : events)
        {
            if (e->value_ == "~") // skip blank markers
                continue;
            std::string cmd =
                std::string("noteOn(") + e->value_ + "," + "127, 250)";

            Interpret(cmd.data(), global_env);
        }
    }
    else if (target_ == "synth")
    {
        for (auto e : events)
        {
            if (e->value_ == "~") // skip blank markers
                continue;
            std::string cmd = std::string("noteOn(fmm,") + e->value_ + ", 250)";

            Interpret(cmd.data(), global_env);
        }
    }
    else if (target_ == "gran")
    {
        for (auto e : events)
        {
            if (e->value_ == "~") // skip blank markers
                continue;
            std::string cmd = std::string("noteOn(str,") + e->value_ + ", 250)";

            Interpret(cmd.data(), global_env);
        }
    }
}

void Process::EvalPattern(
    std::shared_ptr<pattern_parser::PatternNode> const &node, int target_start,
    int target_end)
{

    int target_len = target_end - target_start;

    int divisor = node->GetDivisor();
    if (divisor && (loop_counter_ % divisor != 0))
        return;
    else if (node->euclidean_hits_ && node->euclidean_steps_)
    {
        std::string euclidean_string = generate_euclidean_string(
            node->euclidean_hits_, node->euclidean_steps_);
        // copy node without hits and steps - TODO - this is only implememnted
        // for Leaf nodes currently.
        auto leafy =
            std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(node);
        std::shared_ptr<pattern_parser::PatternLeaf> leafy_copy =
            std::make_shared<pattern_parser::PatternLeaf>(leafy->value_);

        int spacing = target_len / euclidean_string.size();
        for (int i = 0, new_target_start = target_start;
             i < (int)euclidean_string.size() && new_target_start < target_end;
             i++, new_target_start += spacing)
        {
            if (euclidean_string[i] == '1')
            {
                EvalPattern(leafy_copy, new_target_start,
                            new_target_start + spacing);
            }
        }
        return;
    }

    std::shared_ptr<pattern_parser::PatternLeaf> leaf_node =
        std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(node);
    if (leaf_node)
    {
        if (target_start >= PPBAR)
        {
            std::cerr << "MISTAKETHER'NELLIE! idx:" << target_start
                      << " Is >0 PPBAR:" << PPBAR << std::endl;
            return;
        }
        std::string value = leaf_node->value_;
        pattern_events_[target_start].push_back(
            std::make_shared<MusicalEvent>(target_, value));
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
            int num_events = events.size();
            if (!num_events)
                continue;
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

    std::shared_ptr<pattern_parser::PatternMultiStep> multi_node =
        std::dynamic_pointer_cast<pattern_parser::PatternMultiStep>(node);
    if (multi_node)
    {
        int idx = multi_node->current_val_idx_++ % multi_node->values_.size();
        std::shared_ptr<pattern_parser::PatternNode> sub_node =
            multi_node->values_[idx];
        EvalPattern(sub_node, target_start, target_end);
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

    std::cout << "PROC - i got " << pattern_functions_.size() << " funcszz\n";
    for (auto f : pattern_functions_)
        std::cout << f->String() << std::endl;
}

void Process::Start() { active_ = true; }
void Process::Stop() { active_ = false; }

void Process::SetDebug(bool b) { debug_ = b; }

void Process::AppendPatternFunction(std::shared_ptr<PatternFunction> func)
{
    pattern_functions_.push_back(func);
}
