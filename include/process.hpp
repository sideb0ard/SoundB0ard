#pragma once

#include <stdbool.h>
#include <wchar.h>

#include <iostream>

#include <SequenceEngine.h>
#include <defjams.h>
#include <pattern_parser/ast.hpp>
#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenizer.hpp>

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

  public:
    std::string target_;
    std::string pattern_;

    bool active_;
    bool debug_;

  private:
    SequenceEngine engine_;
    std::vector<pattern_parser::PatternNode> pattern_root;
    // pattern_parser::Parser parser_;
};
