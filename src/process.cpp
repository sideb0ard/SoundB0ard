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

namespace
{

std::string ReplaceString(std::string subject, const std::string &search,
                          const std::string &replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

} // namespace

using Wrapper =
    std::pair<std::shared_ptr<ast::Node>, std::shared_ptr<object::Environment>>;
extern Tsqueue<Wrapper> g_queue;
extern std::shared_ptr<object::Environment> global_env;

constexpr char const *s_target_names[] = {"UNDEFINED", "ENV", "VALUES"};
constexpr char const *s_proc_types[] = {"UNDEFINED", "PATTERN", "COMMAND"};
constexpr char const *s_proc_timer_types[] = {"UNDEFINED", "EVERY", "OSC",
                                              "OVER", "RAMP"};

void Process::ParsePattern()
{
    auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern_);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    pattern_root_ = pattern_parzer->ParsePattern();
}

void Process::Update(ProcessType process_type, ProcessTimerType timer_type,
                     float loop_len, std::string command,
                     ProcessPatternTarget target_type,
                     std::vector<std::string> targets, std::string pattern,
                     std::vector<std::shared_ptr<PatternFunction>> funcz)
{
    active_ = false;
    process_type_ = process_type;
    timer_type_ = timer_type;
    loop_len_ = loop_len;
    command_ = command;
    target_type_ = target_type;
    targets_ = targets;
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

    if (process_type_ == PATTERN_PROCESS)
    {
        if (tinfo.is_start_of_loop)
        {
            for (int i = 0; i < PPBAR; i++)
                pattern_events_[i].clear();
            EvalPattern(pattern_root_, 0, PPBAR);

            // std::cout << "BEFORE:" << PatternPrint(pattern_events_) <<
            // std::endl;
            for (auto &f : pattern_functions_)
            {
                if (f->active_)
                    f->TransformPattern(pattern_events_, loop_counter_);
                // std::cout << "AFTER:" << PatternPrint(pattern_events_) <<
                // std::endl;
            }
        }

        if (tinfo.is_midi_tick)
        {
            int cur_tick = tinfo.midi_tick % PPBAR;
            if (pattern_events_[cur_tick].size() == 0)
                return;

            std::vector<std::shared_ptr<MusicalEvent>> &events =
                pattern_events_[cur_tick];

            if (target_type_ == ProcessPatternTarget::ENV)
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
            else if (target_type_ == ProcessPatternTarget::VALUES)
            {
                for (auto e : events)
                {
                    if (e->value_ == "~") // skip blank markers
                        continue;
                    for (auto t : targets_)
                    {
                        std::string cmd = std::string("noteOn(") + t + "," +
                                          e->value_ + ", 250)";
                        Interpret(cmd.data(), global_env);
                    }
                }
            }
        }
    }
    else if (process_type_ == COMMAND_PROCESS)
    {
        if (tinfo.is_midi_tick)
        {
            // TODO support fractional loops
            if (timer_type_ == ProcessTimerType::EVERY)
            {
                bool should_run{false};
                if (loop_len_ > 0 && (loop_counter_ % (int)loop_len_ == 0))
                    should_run = true;

                // if (tinfo.is_start_of_loop && should_run)
                if (tinfo.is_start_of_loop && should_run)
                {
                    for (int i = 0; i < PPBAR; i++)
                        pattern_events_[i].clear();
                    EvalPattern(pattern_root_, 0, PPBAR);

                    int cur_tick = tinfo.midi_tick % PPBAR;
                    if (pattern_events_[cur_tick].size() == 0)
                        return;

                    std::vector<std::shared_ptr<MusicalEvent>> &events =
                        pattern_events_[cur_tick];

                    if (events.size() > 0)
                    {
                        std::string new_cmd =
                            ReplaceString(command_, "%", events[0]->value_);
                        Interpret(new_cmd.data(), global_env);
                    }
                }
            }
            else if (timer_type_ == ProcessTimerType::OSCILLATE)
            {
                float low_target_range{0};
                float hi_target_range{0};
                sscanf(pattern_.c_str(), "%f %f", &low_target_range,
                       &hi_target_range);
                if (low_target_range < hi_target_range)
                {
                    float lower_source_range = 0;
                    float higher_source_range = loop_len_ * PPBAR;
                    float half_source_range = higher_source_range / 2;
                    int cur_tick = tinfo.midi_tick % (int)higher_source_range;
                    float scaled_val{0};
                    if (cur_tick < (higher_source_range / 2))
                    {
                        scaled_val = scaleybum(
                            lower_source_range, half_source_range,
                            low_target_range, hi_target_range, cur_tick);
                    }
                    else
                    {
                        int local_tick =
                            half_source_range - (cur_tick - half_source_range);

                        scaled_val = scaleybum(
                            lower_source_range, half_source_range,
                            low_target_range, hi_target_range, local_tick);
                    }
                    std::string new_cmd = ReplaceString(
                        command_, "%", std::to_string(scaled_val));
                    Interpret(new_cmd.data(), global_env);
                }
            }
            else if (timer_type_ == ProcessTimerType::OVER)
            {
                float low_target_range{0};
                float hi_target_range{0};
                sscanf(pattern_.c_str(), "%f %f", &low_target_range,
                       &hi_target_range);
                if (low_target_range < hi_target_range)
                {
                    float lower_source_range = 0;
                    float higher_source_range = loop_len_ * PPBAR;
                    int cur_tick = tinfo.midi_tick % (int)higher_source_range;
                    float scaled_val =
                        scaleybum(lower_source_range, higher_source_range,
                                  low_target_range, hi_target_range, cur_tick);
                    std::string new_cmd = ReplaceString(
                        command_, "%", std::to_string(scaled_val));
                    Interpret(new_cmd.data(), global_env);
                }
            }
            else if (timer_type_ == ProcessTimerType::RAMP)
            {
                float low_target_range{0};
                float hi_target_range{0};
                sscanf(pattern_.c_str(), "%f %f", &low_target_range,
                       &hi_target_range);
                if (low_target_range != hi_target_range)
                {
                    float lower_source_range = 0;
                    float higher_source_range = loop_len_ * PPBAR;
                    int cur_tick = tinfo.midi_tick % (int)higher_source_range;
                    float scaled_val =
                        scaleybum(lower_source_range, higher_source_range,
                                  low_target_range, hi_target_range, cur_tick);
                    if (cur_tick == higher_source_range - 1)
                        active_ = false;
                    std::string new_cmd = ReplaceString(
                        command_, "%", std::to_string(scaled_val));
                    Interpret(new_cmd.data(), global_env);
                }
            }
        }
    }

    if (tinfo.is_start_of_loop)
        ++loop_counter_;
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
        if (leafy)
        {
            std::shared_ptr<pattern_parser::PatternLeaf> leafy_copy =
                std::make_shared<pattern_parser::PatternLeaf>(leafy->value_);

            int spacing = target_len / euclidean_string.size();
            for (int i = 0, new_target_start = target_start;
                 i < (int)euclidean_string.size() &&
                 new_target_start < target_end;
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
        if (leaf_node->randomize)
        {
            if (rand() % 100 < 50)
                return;
        }
        std::string value = leaf_node->value_;
        pattern_events_[target_start].push_back(
            std::make_shared<MusicalEvent>(value, target_type_));
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

    std::cout << "Lookup up TIMER TYPE:" << timer_type_ << std::endl;
    std::cout << " is it: " << s_proc_timer_types[timer_type_] << std::endl;

    std::cout << "LOOKUP UP TARGET_TYPE:" << target_type_ << std::endl;
    std::cout << " is it: " << s_target_names[target_type_] << std::endl;

    swprintf(status_string, MAX_STATIC_STRING_SZ,
             WANSI_COLOR_WHITE "%sProcess// Type:%s TimerType:%s Target:%s "
                               "Pattern:%s LoopLen:%f Active:%s Command:%s",
             PROC_COLOR, s_proc_types[process_type_],
             s_proc_timer_types[timer_type_], s_target_names[target_type_],
             pattern_.c_str(), loop_len_, active_ ? "true" : "false",
             command_.c_str());
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
