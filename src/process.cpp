#include <audioutils.h>
#include <cmdloop.h>
#include <granulator.h>
#include <midimaaan.h>
#include <mixer.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include <interpreter/evaluator.hpp>
#include <iostream>
#include <memory>
#include <pattern_parser/euclidean.hpp>
#include <pattern_parser/tokenizer.hpp>
#include <process.hpp>
#include <set>
#include <sstream>
#include <tsqueue.hpp>

extern std::unique_ptr<Mixer> global_mixr;
extern Tsqueue<std::string> eval_command_queue;
extern std::shared_ptr<object::Environment> global_env;

namespace {
const char *ModTimerToString(ModulatorTimerType t) {
  switch (t) {
    case ModulatorTimerType::Every return "EVERY";
        case ModulatorTimerType::Oscillate return "OSC";
        case ModulatorTimerType::Ramp return "RAMP";
        case ModulatorTimerType::While return "WHILE"; default:
      return "UNKNOWN";
  }
}
}  // namespace

Modulator::Modulator(ModulatorTimerType timer_type, float loop_len,
                     std::shared_ptr<ast::Expression> mod_vals_exp,
                     std::string mod_command)
    : timer_type_{timer_type}, loop_len_{loop_len}, command_{mod_command} {
  auto new_env = std::make_shared<object::Environment>(global_env);
  auto mod_vals_obj = evaluator::Eval(mod_vals_exp, new_env);
  if (mod_vals_obj->Type() == "STRING") {
    std::shared_ptr<object::String> pattern =
        std::dynamic_pointer_cast<object::String>(pattern_obj);
    std::string mod_vals = pattern->value_;

    sscanf(mod_vals.c_str(), "%f %f", &start_, &end_);
    current_val_ = start_;
    float diff = std::abs(start_ - end_);
    incr_ = diff / (loop_len_ * PPBAR);
    if (incr_ == 0) {
      std::cout << "Nah mate, nae zeros allowed!\n";
      return;
    }

    if (start_ > end_) incr_ *= -1;
  }
}

TidalPattern::TidalPattern(
    TidalPatternTargetType target_type,
    std::shared_ptr<ast::Expression> tidal_pattern,
    std::vector<std::string> targets,
    std::vector<std::shared_ptr<PatternFunction>> tidal_functions)
    : target_type_{target_type},
      tidal_pattern_{tidal_pattern},
      targets_{targets} {
  for (int j = 0; j < PPBAR; j++) pattern_events_played_[j] = true;
  auto new_env = std::make_shared<object::Environment>(global_env);
  auto tidal_pattern_obj = evaluator::Eval(tidal_pattern, new_env);
  if (tidal_pattern_obj->Type() == "STRING") {
    std::shared_ptr<object::String> pattern =
        std::dynamic_pointer_cast<object::String>(pattern_obj);
    auto tokenizer =
        std::make_shared<pattern_parser::Tokenizer>(pattern->value_);
    auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
    tidal_pattern_root_ = pattern_parzer->ParsePattern();
  }
}

void TidalPattern::Run(mixer_timing_info tinfo) {
  if (tinfo.is_start_of_loop) {
    if (!started_) {
      started_ = true;
      cur_event_idx_ = 0;
      event_incr_speed_ = 1;
    }

    for (int i = 0; i < PPBAR; i++) pattern_events_[i].clear();
    if (tidal_pattern_root_) EvalPattern(tidal_pattern_root_, 0, PPBAR);
  }

  if (tinfo.is_midi_tick) {
    if (!started_) return;

    int cur_tick = (int)cur_event_idx_;
    // increment for next step
    cur_event_idx_ = fmodf(cur_event_idx_ + event_incr_speed_, PPBAR);

    for (int i = cur_tick; i < cur_event_idx_; i++) {
      int next_idx = (int)cur_event_idx_;
      if (i == 0) {
        if ((next_idx - i) >= 1) {
          for (int j = 0; j < PPBAR; j++) pattern_events_played_[j] = false;
        }
      }

      // Add bounds check to prevent array access out of bounds
      if (i >= 0 && i < PPBAR && pattern_events_[i].size() > 0 &&
          !pattern_events_played_[i]) {
        pattern_events_played_[i] = true;

        std::vector<std::shared_ptr<MusicalEvent>> &events = pattern_events_[i];

        if (tidal_target_type_ == TidalPatternTargetType::Sample) {
          for (auto e : events) {
            if (e->value_ == "~")  // skip blank markers
              continue;
            std::stringstream ss;
            ss << "note_on(" << e->value_ << "," << /* midi middle C */
                60 << "," << e->velocity_ << "," << e->duration_ << ")";

            eval_command_queue.push(ss.str());
          }
        } else if (tidal_target_type_ == TidalPatternTargetType::MidiNote) {
          for (auto e : events) {
            if (e->value_ == "~") continue;
            std::string midistring = e->value_;
            if (IsNote(e->value_)) {
              for (auto t : targets_) {
                midistring =
                    std::to_string(get_midi_note_from_string(&e->value_[0]));
              }

              std::stringstream ss;
              ss << "note_on(" << t << "," << midistring << "," << e->velocity_
                 << "," << e->duration_ << ")";

              eval_command_queue.push(ss.str());
            }
          }
        }
      }
    }
  }
}

void Computation::Run(mixer_timing_info tinfo) {
  if (tinfo.is_start_of_loop && computation_name_) {
    auto new_env = std::make_shared<object::Environment>(global_env);
    auto computation_obj = evaluator::Eval(computation_name_, new_env);
    if (computation_obj->Type() == "GENERATOR") {
      auto gen_obj =
          std::dynamic_pointer_cast<object::Generator>(computation_obj);
      if (gen_obj) {
        evaluator::ApplyGeneratorRun(
            gen_obj, std::vector<std::shared_ptr<object::Object>>());
      }
    }
  }
}

void Modulator::Run(mixer_timing_info tinfo) {
  current_val_ += incr_;
  switch (timer_type_) {
    case ProcessTimerType::OSCILLATE:
      if (current_val_ < start_) {
        current_val_ = start_;
        incr_ *= -1;
      } else if (current_val_ > end_) {
        current_val_ = end_;
        incr_ *= -1;
      }
      break;
    case ProcessTimerType::OVER:
      if ((incr_ < 0 && current_val_ < end_) ||
          (incr_ > 0 && current_val_ > end_))
        current_val_ = start_;
      break;
    case ProcessTimerType::RAMP:
    default:
      if ((incr_ < 0 && current_val_ < end_) ||
          (incr_ > 0 && current_val_ > end_)) {
        current_val_ = end_;
        if (!update_pending_) active_ = false;
      }
  }

  std::string new_cmd =
      ReplaceString(command_, "%", std::to_string(current_val_));

  eval_command_queue.push(new_cmd);
}

void Process::EnqueueUpdate(ProcessConfig config) {
  active_ = true;
  pending_config_ = config;
  update_pending_ = true;
}

void Process::Update() {
  active_ = false;

  name_ = pending_config_.name;
  process_type_ = pending_config_.process_type;

  switch (process_type_) {
    case ProcessType::Modulator:
      process_runner_ = std::make_unique<Modulator>(
          ModulatorTimerType mod_timer_type, float mod_loop_len,
          std::shared_ptr<ast::Expression> mod_pattern,
          std::string mod_command);
      break;
    case ProcessType::TidalPattern:
      process_runner_ = std::make_unique<TidalPattern>(
          pending_config_.tidal_target_type, pending_config_.tidal_pattern,
          pending_config_.tidal_targets, pending_config_.tidal_functions);
      break;
    case ProcessType::Computation:
      process_runner_ =
          std::make_unique<Computation>(pending_config_.computation_name);
      break;
    default:
  }

  active_ = true;
}

Process::~Process() {
  // std::cout << "Mixer Process deid!\n";
}

void Process::EventNotify(mixer_timing_info tinfo) {
  if (!active_) return;

  if (tinfo.is_start_of_loop && update_pending_) {
    Update();
    update_pending_ = false;
  }

  process_runner_->EventNotify(tinfo);
}

void TidalPattern::EvalPattern(
    std::shared_ptr<pattern_parser::PatternNode> const &node, int target_start,
    int target_end) {
  int target_len = target_end - target_start;

  int divisor = node->GetDivisor();
  if (divisor && (loop_counter_ % divisor != 0))
    return;
  else if (node->euclidean_hits_ && node->euclidean_steps_) {
    auto euclidean_rhythm =
        GenerateBjork(node->euclidean_hits_, node->euclidean_steps_);
    // copy node without hits and steps - TODO - this is only implememnted
    // for Leaf nodes currently.
    auto leafy = std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(node);
    if (leafy) {
      std::shared_ptr<pattern_parser::PatternLeaf> leafy_copy =
          std::make_shared<pattern_parser::PatternLeaf>(leafy->value_);

      int spacing = target_len / euclidean_rhythm.size();
      for (unsigned long i = 0, new_target_start = target_start;
           i < euclidean_rhythm.size() && new_target_start < target_end;
           i++, new_target_start += spacing) {
        if (euclidean_rhythm[i]) {
          EvalPattern(leafy_copy, new_target_start, new_target_start + spacing);
        }
      }
      return;
    }
  }

  float amp = 0;
  if (node->amplitude_.size() > 0) {
    amp = node->amplitude_[node->amplitude_idx_];
    if (++(node->amplitude_idx_) == (int)node->amplitude_.size())
      node->amplitude_idx_ = 0;
  }
  float dur = 0;
  if (node->duration_.size() > 0) {
    dur = node->duration_[node->duration_idx_];
    if (++(node->duration_idx_) == (int)node->duration_.size())
      node->duration_idx_ = 0;
  }

  std::shared_ptr<pattern_parser::PatternLeaf> leaf_node =
      std::dynamic_pointer_cast<pattern_parser::PatternLeaf>(node);
  if (leaf_node) {
    if (target_start >= PPBAR) {
      std::cerr << "MISTAKETHER'NELLIE! idx:" << target_start
                << " Is >0 PPBAR:" << PPBAR << std::endl;
      return;
    }
    if (leaf_node->randomize_) {
      if (rand() % 100 < 50) return;
    }
    std::string value = leaf_node->value_;
    if (value != "~") {
      if (amp) {
        if (dur)
          pattern_events_[target_start].push_back(
              std::make_shared<MusicalEvent>(value, amp, dur, target_type_));
        else
          pattern_events_[target_start].push_back(
              std::make_shared<MusicalEvent>(value, amp, target_type_));
      } else
        pattern_events_[target_start].push_back(
            std::make_shared<MusicalEvent>(value, target_type_));
    }
    return;
  }

  std::shared_ptr<pattern_parser::PatternGroup> composite_node =
      std::dynamic_pointer_cast<pattern_parser::PatternGroup>(node);
  if (composite_node) {
    int event_groups_size = composite_node->event_groups_.size();
    for (int i = 0; i < event_groups_size; i++) {
      std::vector<std::shared_ptr<pattern_parser::PatternNode>> &events =
          composite_node->event_groups_[i];
      int num_events = events.size();
      if (!num_events) continue;
      int spacing = target_len / num_events;

      for (int j = target_start, event_idx = 0;
           j < target_end && event_idx < num_events;
           j += spacing, event_idx++) {
        std::shared_ptr<pattern_parser::PatternNode> sub_node =
            events[event_idx];
        if (amp) {
          sub_node->amplitude_.clear();
          sub_node->amplitude_.push_back(amp);
        }
        if (dur) {
          sub_node->duration_.clear();
          sub_node->duration_.push_back(dur);
        }

        EvalPattern(sub_node, j, j + spacing);
      }
    }
  }

  std::shared_ptr<pattern_parser::PatternMultiStep> multi_node =
      std::dynamic_pointer_cast<pattern_parser::PatternMultiStep>(node);
  if (multi_node) {
    int idx = multi_node->current_val_idx_++ % multi_node->values_.size();
    std::shared_ptr<pattern_parser::PatternNode> sub_node =
        multi_node->values_[idx];
    if (amp) {
      sub_node->amplitude_.clear();
      sub_node->amplitude_.push_back(amp);
    }
    if (dur) {
      sub_node->duration_.clear();
      sub_node->duration_.push_back(dur);
    }
    EvalPattern(sub_node, target_start, target_end);
  }
}

std::string TidalPattern::Status() {
  std::stringstream ss;
  ss << ANSI_COLOR_CYAN;

  ss << "$ \"" << name << "\"";
  bool firscht = true;
  for (auto &t : targets_) {
    if (!firscht) ss << ",";
    firscht = false;
    ss << t;
  }
  for (auto f : pattern_functions_) {
    if (f->active_) ss << " | " << f->String();
  }

  ss << ANSI_COLOR_RESET;
  return ss.str();
}

std::string Computation::Status() {
  std::stringstream ss;
  ss << ANSI_COLOR_GREEN_TOO ss << "# \"" << name << "\"";
  ss << ANSI_COLOR_RESET;
  return ss.str();
}

std::string Modulator::Status() {
  std::stringstream ss;
  ss << ANSI_COLOR_GREEN;

  ss << "< " << ModTimerToString(timer_type_) << " " << loop_len_ << " \""
     << pattern_ << "\" "
     << " \"" << command_ << "\"";
  ss << ANSI_COLOR_RESET;
  return ss.str();
}

void TidalPattern::SetSpeed(float val) {
  event_incr_speed_ = val;
}

void TidalPattern::AppendPatternFunction(
    std::shared_ptr<PatternFunction> func) {
  pattern_functions_.push_back(func);
}

void TidalPattern::UpdateLoopLen(int val) {
  loop_len_ = val;
  float diff = std::abs(start_ - end_);
  incr_ = diff / (loop_len_ * PPBAR);
  if (incr_ == 0) {
    std::cout << "Nah mate, nae zeros allowed!\n";
    return;
  }
}

std::string Process::Status() {
  return process_runner_->Status();
}

void Process::Start() {
  active_ = true;
}

void Process::Stop() {
  active_ = false;
}
