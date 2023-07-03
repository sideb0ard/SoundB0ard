#include <audioutils.h>
#include <cmdloop.h>
#include <looper.h>
#include <midimaaan.h>
#include <mixer.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include <interpreter/evaluator.hpp>
#include <iostream>
#include <pattern_parser/euclidean.hpp>
#include <pattern_parser/tokenizer.hpp>
#include <process.hpp>
#include <set>
#include <sstream>
#include <tsqueue.hpp>

extern Mixer *mixr;
extern Tsqueue<std::string> eval_command_queue;
extern std::shared_ptr<object::Environment> global_env;

namespace {

std::string ReplaceString(std::string subject, const std::string &search,
                          const std::string &replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
  return subject;
}

}  // namespace

constexpr char const *s_proc_timer_types[] = {"UNDEFINED", "every", "osc",
                                              "over", "ramp"};

void Process::ParsePattern() {
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(pattern_);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  auto new_pattern_root = pattern_parzer->ParsePattern();
  pattern_root_ = new_pattern_root;
}

void Process::EnqueueUpdate(ProcessConfig config) {
  active_ = true;
  pending_config_ = config;
  update_pending_ = true;
}

void Process::Update() {
  active_ = false;
  started_ = false;

  for (int j = 0; j < PPBAR; j++) pattern_events_played_[j] = true;

  name = pending_config_.name;
  process_type_ = pending_config_.process_type;
  timer_type_ = pending_config_.timer_type;
  loop_len_ = pending_config_.loop_len;
  command_ = pending_config_.command;
  target_type_ = pending_config_.target_type;
  targets_ = pending_config_.targets;
  pattern_ = pending_config_.pattern;
  pattern_expression_ = pending_config_.pattern_expression;

  auto new_env = std::make_shared<object::Environment>(global_env);
  auto pattern_obj = evaluator::Eval(pattern_expression_, new_env);
  if (pattern_obj->Type() == "STRING") {
    std::shared_ptr<object::String> pattern =
        std::dynamic_pointer_cast<object::String>(pattern_obj);
    pattern_ = pattern->value_;

    if (timer_type_ == ProcessTimerType::RAMP ||
        timer_type_ == ProcessTimerType::OVER ||
        timer_type_ == ProcessTimerType::OSCILLATE) {
      sscanf(pattern_.c_str(), "%f %f", &start_, &end_);
      current_val_ = start_;
      float diff = std::abs(start_ - end_);
      incr_ = diff / (loop_len_ * PPBAR);
      if (incr_ == 0) {
        std::cout << "Nah mate, nae zeros allowed!\n";
        return;
      }

      if (start_ > end_) incr_ *= -1;
    }
    ParsePattern();
  }

  for (auto &oldfz : pattern_functions_) oldfz->active_ = false;
  for (auto fz : pending_config_.funcz) pattern_functions_.push_back(fz);
  active_ = true;
}

Process::~Process() { std::cout << "Mixer Process deid!\n"; }

void Process::EventNotify(mixer_timing_info tinfo) {
  if (!active_) return;

  if (tinfo.is_start_of_loop && update_pending_) {
    Update();
    update_pending_ = false;
  }

  // if (tinfo.is_start_of_loop && pattern_expression_) {
  if (pattern_expression_) {
    auto new_env = std::make_shared<object::Environment>(global_env);
    auto pattern_obj = evaluator::Eval(pattern_expression_, new_env);
    if (pattern_obj->Type() == "GENERATOR") {
      auto gen_obj = std::dynamic_pointer_cast<object::Generator>(pattern_obj);
      if (gen_obj) {
        if (tinfo.is_midi_tick) {
          if (tinfo.is_start_of_loop) started_ = true;
          if (started_) {
            auto pattern_obj =
                evaluator::ApplyGeneratorSignalGenerator(gen_obj);
            auto pattern_string =
                std::dynamic_pointer_cast<object::String>(pattern_obj);
            if (pattern_string) {
              pattern_ = pattern_string->value_;
              ParsePattern();
            }
          }
          if (tinfo.is_start_of_loop) {
            auto pattern_obj = evaluator::ApplyGeneratorRun(gen_obj);
            auto pattern_string =
                std::dynamic_pointer_cast<object::String>(pattern_obj);
            if (pattern_string) {
              pattern_ = pattern_string->value_;
              ParsePattern();
            }
          }
        }
      }
    }
  }

  // FUNCTION_PROCESS i.e. '#' or '$'
  if (process_type_ == FUNCTION_PROCESS) {
    if (tinfo.is_start_of_loop) {
      if (!started_) {
        started_ = true;
        cur_event_idx_ = 0;
        event_incr_speed_ = 1;
      }

      for (int i = 0; i < PPBAR; i++) pattern_events_[i].clear();
      if (pattern_root_) EvalPattern(pattern_root_, 0, PPBAR);

      for (auto &f : pattern_functions_) {
        if (f->func_type_ == PatternFunctionType::PATTERN_OP && f->active_) {
          f->TransformPattern(pattern_events_, loop_counter_, tinfo);
        } else if (f->func_type_ == PatternFunctionType::PROCESS_OP &&
                   f->active_) {
          auto speed_func = std::dynamic_pointer_cast<PatternSpeed>(f);
          if (speed_func) {
            event_incr_speed_ = speed_func->speed_multiplier_;
          }
        }
      }
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

        if (pattern_events_[i].size() > 0 && !pattern_events_played_[i]) {
          pattern_events_played_[i] = true;

          std::vector<std::shared_ptr<MusicalEvent>> &events =
              pattern_events_[i];

          if (target_type_ == ProcessPatternTarget::ENV) {
            for (auto e : events) {
              if (e->value_ == "~")  // skip blank markers
                continue;
              std::stringstream ss;
              ss << "note_on(" << e->value_ << "," << /* midi middle C */ 60
                 << "," << e->velocity_ << "," << e->duration_ << ")";

              eval_command_queue.push(ss.str());
            }
          } else if (target_type_ == ProcessPatternTarget::VALUES) {
            for (auto e : events) {
              if (e->value_ == "~")  // skip blank markers
                continue;
              for (auto t : targets_) {
                std::string midistring = e->value_;
                if (IsNote(e->value_)) {
                  midistring =
                      std::to_string(get_midi_note_from_string(&e->value_[0]));
                }

                std::stringstream ss;
                ss << "note_on(" << t << "," << midistring << ","
                   << e->velocity_ << "," << e->duration_ << ")";

                eval_command_queue.push(ss.str());
              }
            }
          }
        }
      }
    }
  }
  // TIMED_PROCESS i.e. '<'
  else if (process_type_ == TIMED_PROCESS) {
    if (tinfo.is_midi_tick) {
      // TODO support fractional loops
      if (timer_type_ == ProcessTimerType::EVERY) {
        bool should_run{false};
        if (loop_len_ > 0 && (loop_counter_ % (int)loop_len_ == 0))
          should_run = true;

        // if (tinfo.is_start_of_loop && should_run)
        if (tinfo.is_start_of_loop && should_run) {
          for (int i = 0; i < PPBAR; i++) pattern_events_[i].clear();
          EvalPattern(pattern_root_, 0, PPBAR);

          int cur_tick = tinfo.midi_tick % PPBAR;
          if (pattern_events_[cur_tick].size() > 0) {
            std::vector<std::shared_ptr<MusicalEvent>> &events =
                pattern_events_[cur_tick];

            if (events.size() > 0) {
              std::string new_cmd =
                  ReplaceString(command_, "%", events[0]->value_);

              eval_command_queue.push(new_cmd);
            }
          }
        }
      } else {
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
    }
  }

  if (tinfo.is_start_of_loop) {
    // std::cout << "PROC LOOP COUNTER:" << loop_counter_ << std::endl;
    ++loop_counter_;
  }
}

void Process::EvalPattern(
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

std::string Process::Status() {
  std::stringstream ss;

  const char *PROC_COLOR = ANSI_COLOR_RESET;
  if (active_) {
    if (process_type_ == ProcessType::FUNCTION_PROCESS) {
      if (target_type_ == ProcessPatternTarget::ENV)
        PROC_COLOR = ANSI_COLOR_CYAN;
      else
        PROC_COLOR = ANSI_COLOR_GREEN_TOO;
    } else if (process_type_ == ProcessType::TIMED_PROCESS)
      PROC_COLOR = ANSI_COLOR_GREEN;
  }
  ss << PROC_COLOR;

  if (process_type_ == ProcessType::FUNCTION_PROCESS) {
    if (target_type_ == ProcessPatternTarget::ENV) {
      ss << "$ \"" << name << "\"";
    } else if (target_type_ == ProcessPatternTarget::VALUES) {
      ss << "# \"" << name << "\" ";
      bool firscht = true;
      for (auto &t : targets_) {
        if (!firscht) ss << ",";
        firscht = false;
        ss << t;
      }
    }
  } else if (process_type_ == ProcessType::TIMED_PROCESS) {
    ss << "< " << s_proc_timer_types[timer_type_] << " " << loop_len_ << " \""
       << pattern_ << "\" "
       << " \"" << command_ << "\"";
  }

  for (auto f : pattern_functions_) {
    if (f->active_) ss << " | " << f->String();
  }

  ss << ANSI_COLOR_RESET;

  return ss.str();
}

void Process::Start() { active_ = true; }
void Process::Stop() { active_ = false; }

void Process::SetDebug(bool b) { debug_ = b; }
void Process::SetSpeed(float val) { event_incr_speed_ = val; }

void Process::AppendPatternFunction(std::shared_ptr<PatternFunction> func) {
  pattern_functions_.push_back(func);
}

void Process::UpdateLoopLen(int val) {
  loop_len_ = val;
  float diff = std::abs(start_ - end_);
  incr_ = diff / (loop_len_ * PPBAR);
  if (incr_ == 0) {
    std::cout << "Nah mate, nae zeros allowed!\n";
    return;
  }
}
