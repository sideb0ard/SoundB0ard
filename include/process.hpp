#pragma once

#include <stdbool.h>
#include <wchar.h>

#include <iostream>

#include <SequenceEngine.h>
#include <defjams.h>
#include <interpreter/ast.hpp>

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

  public:
    std::string target_;
    std::string pattern_;

    bool active_;
    bool debug_;

  private:
    SequenceEngine engine;
};
