#include <utils.h>

#include <interpreter/object.hpp>
#include <iostream>
#include <pattern_parser/parser.hpp>
#include <pattern_parser/tokenizer.hpp>
#include <sstream>
#include <tsqueue.hpp>

extern Tsqueue<std::string> eval_command_queue;

namespace object {

Pattern::Pattern(std::string string_pattern) : string_pattern{string_pattern} {
  ParseStringPattern();
  EvalPattern(pattern_root, 0, PPBAR);
}
std::string Pattern::Inspect() {
  std::stringstream ss;
  ss << "Pattern:";
  ss << string_pattern;
  return ss.str();
}

void Pattern::ParseStringPattern() {
  auto tokenizer = std::make_shared<pattern_parser::Tokenizer>(string_pattern);
  auto pattern_parzer = std::make_shared<pattern_parser::Parser>(tokenizer);
  auto new_pattern_root = pattern_parzer->ParsePattern();
  pattern_root = new_pattern_root;
}

ObjectType Pattern::Type() {
  return PATTERN_OBJ;
}

std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> Pattern::Eval() {
  for (int i = 0; i < PPBAR; i++) pattern_events[i].clear();
  EvalPattern(pattern_root, 0, PPBAR);
  ++eval_counter;

  return pattern_events;
}

std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> Pattern::Print() {
  return pattern_events;
}

void Pattern::EvalPattern(
    std::shared_ptr<pattern_parser::PatternNode> const &node, int target_start,
    int target_end) {
  int target_len = target_end - target_start;

  int divisor = node->GetDivisor();
  if (divisor && (eval_counter % divisor != 0))
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
      for (int i = 0, new_target_start = target_start;
           i < (int)euclidean_rhythm.size() && new_target_start < target_end;
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
      pattern_events[target_start].push_back(
          std::make_shared<MusicalEvent>(value));
      // if (amp)
      //{
      //    if (dur)
      //        pattern_events[target_start].push_back(
      //            std::make_shared<MusicalEvent>(value, amp, dur,
      //                                           target_type_));
      //    else
      //        pattern_events[target_start].push_back(
      //            std::make_shared<MusicalEvent>(value, amp,
      //                                           target_type_));
      //}
      // else
      //    pattern_events[target_start].push_back(
      //        std::make_shared<MusicalEvent>(value, target_type_));
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
}  // namespace object
