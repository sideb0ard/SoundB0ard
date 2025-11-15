#pragma once

#include <defjams.h>
#include <stdbool.h>

#include <array>
#include <interpreter/ast.hpp>
#include <pattern_functions.hpp>
#include <pattern_parser/ast.hpp>
#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenizer.hpp>

struct ProcessConfig {
  ProcessType type;

  //// TidalPattern
  TidalPatternTargetType tidal_target_type;
  std::shared_ptr<ast::Expression> tidal_pattern;
  std::vector<std::string> tidal_targets;
  std::vector<std::shared_ptr<PatternFunction>> tidal_functions;

  //// Computation
  std::shared_ptr<ast::Expression> computation_name;

  //// Modulator
  ModulatorTimerType mod_timer_type;
  float mod_loop_len;
  std::shared_ptr<ast::Expression> mod_pattern;
  std::string mod_command;

  mixer_timing_info tinfo;
};

class ProcessRunner {
 public:
  virtual ~ProcessRunner() = 0;
  virtual void EventNotify(mixer_timing_info tinfo) = 0;
  virtual std::string Status() = 0;
  bool started_{false};
};

class Modulator : ProcessRunner {
 public:
  Modulator(ModulatorTimerType timer_type, float loop_len,
            std::shared_ptr<ast::Expression> mod_pattern,
            std::string mod_command);
  ~Modulator() = default;
  void EventNotify(mixer_timing_info tinfo) override;
  std::string Status() override;

 private:
  ModulatorTimerType timer_type_;
  float loop_len_{1};
  std::string mod_command_{};

  float start_{0};
  float end_{0};
  float incr_{0};
  float current_val_{0};
};

class TidalPattern : ProcessRunner {
 public:
  TidalPattern(TidalPatternTargetType target_type,
               std::shared_ptr<ast::Expression> tidal_pattern,
               std::vector<std::string> tidal_targets,
               std::vector<std::shared_ptr<PatternFunction>> tidal_functions);
  ~TidalPattern() = default;
  void EventNotify(mixer_timing_info tinfo) override;
  std::string Status() override;

  void SetSpeed(float val);
  void ParsePattern();
  void EvalPattern(std::shared_ptr<pattern_parser::PatternNode> const &pattern,
                   int target_start, int target_end);
  void AppendPatternFunction(std::shared_ptr<PatternFunction> func);
  void UpdateLoopLen(int val);

 private:
  TidalPatternTargetType target_type_;
  std::vector<std::string> targets_;

  std::shared_ptr<pattern_parser::PatternNode> tidal_pattern_root_{nullptr};
  std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> tidal_events_;

  std::array<bool, PPBAR> pattern_events_played_ = {};  // for slow speed
  float cur_event_idx_{0};
  float event_incr_speed_{1};

  std::vector<std::shared_ptr<PatternFunction>> pattern_functions_;
  int loop_counter_;
};

class Computation : ProcessRunner {
 public:
  Computation(std::shared_ptr<ast::Expression> name)
      : computation_name_{name} {};
  ~Computation() = default;
  void EventNotify(mixer_timing_info tinfo) override;
  std::string Status() override;

 private:
  std::shared_ptr<ast::Expression> computation_name_;
};

class Process {
 public:
  Process() = default;
  ~Process();
  std::string Status();
  void Start();
  void Stop();
  void EventNotify(mixer_timing_info);
  void EnqueueUpdate(ProcessConfig config);

  bool active_{false};

 private:
  void Update();

  ProcessConfig pending_config_;
  bool update_pending_;

  ProcessType process_type_;
  std::string name_;
  std::unique_ptr<ProcessRunner> process_runner_;
  bool started_{false};
};
