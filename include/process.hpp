#pragma once

#include <stdbool.h>
#include <wchar.h>

#include <array>

#include <SequenceEngine.h>
#include <defjams.h>
#include <pattern_parser/ast.hpp>
#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenizer.hpp>

struct MusicalEvent
{
    MusicalEvent() = default;
    MusicalEvent(std::string target) : target_{target} {}
    std::string target_;
};

class Process
{
  public:
    Process() = default;
    ~Process();
    void Status(wchar_t *ss);
    void Start();
    void Stop();
    void EventNotify(mixer_timing_info);
    void SetDebug(bool b);
    void ParsePattern();
    void Update(std::string target, std::string pattern);
    void EvalPattern(std::shared_ptr<pattern_parser::PatternNode> &pattern,
                     int target_start, int target_end);

  public:
    std::string target_;
    std::string pattern_;

    bool active_;
    bool debug_;

  private:
    std::shared_ptr<pattern_parser::PatternNode> pattern_root_;
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR>
        pattern_events_;
    int loop_counter_;
};
