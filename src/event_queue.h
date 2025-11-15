#include <interpreter/ast.hpp>
#include <interpreter/object.hpp>
#include <pattern_functions.hpp>
#include <string>
#include <vector>

enum Event {
  TIMING_EVENT,
  PROCESS_UPDATE_EVENT,
  PROCESS_SET_PARAM_EVENT,
};

struct event_queue_item {
  unsigned int type;  // timing info or process update
  // timing info
  mixer_timing_info timing_info;

  // process update
  int target_process_id;
  ProcessType process_type;

  //// TidalPattern Params
  TidalPatternTargetType tidal_target_type;
  std::shared_ptr<ast::Expression> tidal_pattern;
  std::vector<std::string> tidal_targets;
  std::vector<std::shared_ptr<PatternFunction>> tidal_functions;

  //// Computation Params
  std::shared_ptr<ast::Expression> computation_name;

  //// Modulator Params
  ModulatorTimerType mod_timer_type;
  float mod_loop_len;
  std::string mod_command;
  std::shared_ptr<ast::Expression> mod_pattern;
};
