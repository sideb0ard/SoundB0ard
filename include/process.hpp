#pragma once

#include <stdbool.h>
#include <wchar.h>

#include <array>

#include <SequenceEngine.h>
#include <defjams.h>
#include <pattern_parser/ast.hpp>
#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenizer.hpp>

class MusicalEvent
{
    std::string target_;
};

class Process
{
  public:
    Process(std::string target, std::string pattern);
    ~Process() = default;
    void Status(wchar_t *ss);
    void Start();
    void Stop();
    void EventNotify(mixer_timing_info);
    void SetDebug(bool b);
    void ParsePattern();
    void EvalPattern(
        std::vector<std::shared_ptr<pattern_parser::PatternNode>> &pattern,
        int target_start, int target_end);

  public:
    std::string target_;
    std::string pattern_;

    bool active_;
    bool debug_;

  private:
    // SequenceEngine engine_;
    std::shared_ptr<pattern_parser::EventGroup> pattern_root_;
    std::array<MusicalEvent, PPBAR> pattern_events_;
    int loop_counter_;
    // pattern_parser::Parser parser_;
};
