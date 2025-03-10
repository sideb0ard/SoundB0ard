#include <audio_action_queue.h>
#include <audioutils.h>
#include <midi_cmds.h>
#include <mixer.h>
#include <utils.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <interpreter/builtins.hpp>
#include <interpreter/evaluator.hpp>
#include <interpreter/sound_cmds.hpp>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tsqueue.hpp>
#include <unordered_map>
#include <vector>

#include "PerlinNoise.hpp"
#include "filebuffer.h"
#include "midi_device.h"
#include "midi_freq_table.h"

namespace fs = std::filesystem;

extern Mixer *mixr;
extern Tsqueue<std::unique_ptr<AudioActionItem>> audio_queue;
extern Tsqueue<std::string> eval_command_queue;
extern Tsqueue<std::string> repl_queue;
extern Tsqueue<int> audio_reply_queue;
extern siv::PerlinNoise perlinGenerator;

const std::vector<std::string> FILES_TO_IGNORE = {".DS_Store"};

namespace {

bool HasPresets(int sg_type) {
  if (sg_type == MINISYNTH_TYPE || sg_type == DXSYNTH_TYPE ||
      sg_type == DRUMSYNTH_TYPE)
    return true;
  return false;
}

double GetRandomBetweenNegativeOneAndOne() {
  double f = (double)rand() / RAND_MAX;
  double val = -1 + (f * 2);
  return val;
}

std::shared_ptr<object::Array> ExtractArrayFromPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR>
        evaluated_pat) {
  auto return_array = std::make_shared<object::Array>(
      std::vector<std::shared_ptr<object::Object>>());

  for (auto ep : evaluated_pat) {
    auto midi_tick_array = std::make_shared<object::Array>(
        std::vector<std::shared_ptr<object::Object>>());
    if (!ep.empty()) {
      for (auto p : ep) {
        midi_tick_array->elements_.push_back(
            std::make_shared<object::String>(p->value_));
      }
    }

    return_array->elements_.push_back(midi_tick_array);
  }

  return return_array;
}
std::array<std::vector<int>, PPBAR> ExtractIntsFromEvalPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR>
        evaluated_pat) {
  std::array<std::vector<int>, PPBAR> return_array;

  for (int i = 0; i < PPBAR; ++i) {
    auto ep = evaluated_pat[i];
    if (!ep.empty()) {
      for (auto p : ep) {
        int p_val = 1;
        try {
          p_val = std::stoi(p->value_);
        } catch (...) {
          // expected sometimes.
        }

        return_array[i].push_back(p_val);
      }
    }
  }

  return return_array;
}

std::array<std::vector<std::string>, PPBAR> ExtractStringsFromEvalPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR>
        evaluated_pat) {
  std::array<std::vector<std::string>, PPBAR> return_array;

  for (int i = 0; i < PPBAR; ++i) {
    auto ep = evaluated_pat[i];
    if (!ep.empty()) {
      for (auto p : ep) return_array[i].push_back(p->value_);
    }
  }

  return return_array;
}

bool ShouldIgnore(std::string filename) {
  auto result =
      std::find(begin(FILES_TO_IGNORE), end(FILES_TO_IGNORE), filename);
  if (result == std::end(FILES_TO_IGNORE)) {
    return false;
  }
  return true;
}

std::vector<int> GetMidiNotes(std::shared_ptr<object::Object> &numz) {
  std::vector<int> midi_nums{};
  auto int_object = std::dynamic_pointer_cast<object::Number>(numz);
  if (int_object) {
    midi_nums.push_back(int_object->value_);
  }

  auto array_object = std::dynamic_pointer_cast<object::Array>(numz);
  if (array_object) {
    for (auto e : array_object->elements_) {
      auto int_object = std::dynamic_pointer_cast<object::Number>(e);
      if (int_object) {
        midi_nums.push_back(int_object->value_);
      }
    }
  }
  return midi_nums;
}

void SendMixerActionGeneral(AudioAction action_type, bool val) {
  std::cout << "Sending action type: " << action_type << " - set to " << val
            << std::endl;
  auto action = std::make_unique<AudioActionItem>(action_type);
  action->general_val = val;

  audio_queue.push(std::move(action));
}

void SendMidiMappingShow() {
  std::cout << "sending a midi map SHOW\n";
  auto action = std::make_unique<AudioActionItem>(AudioAction::MIDI_MAP_SHOW);
  audio_queue.push(std::move(action));
}

void SendMidiMapping(int mapped_id, std::string mapped_param) {
  std::cout << "sending a midi map\n";
  auto action = std::make_unique<AudioActionItem>(AudioAction::MIDI_MAP);
  action->mapped_id = mapped_id;
  action->mapped_param = mapped_param;
  audio_queue.push(std::move(action));
}

void note_on_at(int sgid, std::vector<int> midi_nums, int note_start_time,
                int vel, int dur) {
  auto action =
      std::make_unique<AudioActionItem>(AudioAction::MIDI_NOTE_ON_DELAYED);
  action->soundgen_num = sgid;
  action->notes = midi_nums;
  action->velocity = vel;
  action->duration = dur;
  action->note_start_time = note_start_time;
  audio_queue.push(std::move(action));
}
void midi_event_at(int sgid, midi_event ev, int start_time) {
  auto action =
      std::make_unique<AudioActionItem>(AudioAction::RECORDED_MIDI_EVENT);
  action->has_midi_event = true;
  action->event = ev;
  action->mixer_soundgen_idx = sgid;
  action->start_at = start_time;
  audio_queue.push(std::move(action));
}

void note_on(int sgid, std::vector<int> midi_nums, int vel, int dur) {
  auto action = std::make_unique<AudioActionItem>(AudioAction::MIDI_NOTE_ON);
  action->soundgen_num = sgid;
  action->notes = midi_nums;
  action->velocity = vel;
  action->duration = dur;
  audio_queue.push(std::move(action));
}

void note_off(int sgid, std::vector<int> midi_nums) {
  auto action = std::make_unique<AudioActionItem>(AudioAction::MIDI_NOTE_OFF);
  action->soundgen_num = sgid;
  action->notes = midi_nums;
  audio_queue.push(std::move(action));
}
void note_off_at(int sgid, std::vector<int> midi_nums, int note_stop_time) {
  auto action =
      std::make_unique<AudioActionItem>(AudioAction::MIDI_NOTE_OFF_DELAYED);
  action->soundgen_num = sgid;
  action->notes = midi_nums;
  action->note_start_time = note_stop_time;
  audio_queue.push(std::move(action));
}

template <typename T>
std::array<T, 16> ShrinkPatternToStepSequence(
    std::array<std::vector<T>, PPBAR> &pattern) {
  std::array<T, 16> shrunk{};
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 240; ++j) {
      if (pattern[i * 240 + j].size() > 0) shrunk[i] = pattern[i * 240 + j][0];
    }
  }
  return shrunk;
}

std::array<std::vector<int>, PPBAR> ExtractIntsFromObjectArray(
    std::shared_ptr<object::Array> play_pattern) {
  std::array<std::vector<int>, PPBAR> return_pattern;
  for (int i = 0; i < PPBAR; ++i) {
    auto ppattern =
        std::dynamic_pointer_cast<object::Array>(play_pattern->elements_[i]);
    if (ppattern) {
      for (auto item : ppattern->elements_) {
        auto sg_string = std::dynamic_pointer_cast<object::String>(item);
        if (sg_string) {
          int sg_val = 1;
          try {
            sg_val = std::stoi(sg_string->value_);
          } catch (...) {
            // expected sometimes.
          }

          return_pattern[i].push_back(sg_val);
        }
      }
    }
  }
  return return_pattern;
}

void RecursiveScaler(std::array<std::vector<int>, PPBAR> &scaled_pattern,
                     std::shared_ptr<object::Array> play_pattern, int midi_len,
                     int offset) {
  int nlen = play_pattern->elements_.size();
  int pulses_per = midi_len / nlen;

  for (int i = 0; i < nlen; ++i) {
    auto item = play_pattern->elements_[i];
    auto val = std::dynamic_pointer_cast<object::Number>(item);
    if (val) scaled_pattern[offset + i * pulses_per].push_back(val->value_);

    auto aval = std::dynamic_pointer_cast<object::Array>(item);
    if (aval) RecursiveScaler(scaled_pattern, aval, pulses_per, i * pulses_per);
  }
}

template <typename T>
void PrintPattern(std::array<std::vector<T>, PPBAR> &patternn) {
  for (int i = 0; i < PPBAR; ++i) {
    if (patternn[i].size() > 0) {
      for (auto item : patternn[i])
        std::cout << "[" << i << "] " << item << std::endl;
    }
  }
}

std::array<std::vector<int>, PPBAR> ScalePattern(
    std::shared_ptr<object::Array> play_pattern) {
  std::array<std::vector<int>, PPBAR> scaled_pattern;
  int midi_len = PPBAR;
  RecursiveScaler(scaled_pattern, play_pattern, midi_len, 0);

  return scaled_pattern;
}

void play_map_on(std::shared_ptr<object::Object> soundgen,
                 std::shared_ptr<object::Object> pattern_map, int dur,
                 int vel) {
  auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(soundgen);
  if (!sg) return;

  auto play_map = std::dynamic_pointer_cast<object::Hash>(pattern_map);

  if (play_map) {
    for (const auto &[_, hv] : play_map->pairs_) {
      auto k = std::dynamic_pointer_cast<object::Number>(hv.key_);
      auto v = std::dynamic_pointer_cast<object::Number>(hv.value_);
      if (k && v) {
        note_on_at(sg->soundgen_id_, {int(v->value_)}, k->value_, vel, dur);
      }
    }
  }
}

void play_array_on(std::shared_ptr<object::Object> soundgen,
                   std::shared_ptr<object::Object> pattern, float speed,
                   int dur, int vel) {
  auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(soundgen);
  if (!sg) return;

  std::shared_ptr<object::Array> play_pattern;
  auto pat_obj = std::dynamic_pointer_cast<object::Pattern>(pattern);
  if (pat_obj)
    play_pattern = ExtractArrayFromPattern(pat_obj->Eval());
  else
    play_pattern = std::dynamic_pointer_cast<object::Array>(pattern);

  if (play_pattern) {
    std::array<std::vector<int>, PPBAR> live_pattern;

    if (play_pattern->elements_.size() >= PPBAR &&
        play_pattern->elements_[0]->Type() == "ARRAY")
      live_pattern = ExtractIntsFromObjectArray(play_pattern);
    else if (play_pattern->elements_.size() > 0 &&
             play_pattern->elements_[0]->Type() == "NUMBER")
      live_pattern = ScalePattern(play_pattern);

    for (int j = 0; j < speed; ++j) {
      float j_offset = (int)live_pattern.size() / speed * j;

      // PrintPattern(live_pattern);
      for (int i = 0; i < (int)live_pattern.size(); ++i) {
        float i_offset = i / speed;

        auto ppattern = live_pattern[i];

        if (ppattern.size() > 0) {
          std::vector<int> midi_nums;
          for (auto item : ppattern) {
            if (item > 0) midi_nums.push_back(item);
          }

          if (midi_nums.size() > 0)
            note_on_at(sg->soundgen_id_, midi_nums, j_offset + i_offset, vel,
                       dur);
        }
      }
    }
  }
  auto multi_midi_pat_obj =
      std::dynamic_pointer_cast<object::MidiArray>(pattern);
  if (multi_midi_pat_obj) {
    for (auto &e : multi_midi_pat_obj->notes_on_) {
      midi_event_at(sg->soundgen_id_, e, e.playback_tick);
    }
    for (auto &e : multi_midi_pat_obj->control_messages_) {
      midi_event_at(sg->soundgen_id_, e, e.playback_tick);
    }
  }
}

void play_pattern_array(std::shared_ptr<object::Array> play_pattern,
                        float speed, int dur, int vel) {
  if (play_pattern->elements_.size() != PPBAR) return;

  for (int j = 0; j < speed; ++j) {
    float j_offset = PPBAR / speed * j;
    for (int i = 0; i < PPBAR; ++i) {
      float i_offset = i / speed;
      auto ppattern =
          std::dynamic_pointer_cast<object::Array>(play_pattern->elements_[i]);
      if (ppattern) {
        for (auto item : ppattern->elements_) {
          auto sg_string = std::dynamic_pointer_cast<object::String>(item);
          if (sg_string) {
            auto sg_name = sg_string->value_;
            std::stringstream ss;
            ss << "note_on_at(" << sg_name << ", 1, " << j_offset + i_offset
               << ", dur=" << dur << ", vel=" << vel << ")";
            eval_command_queue.push(ss.str());
          }
        }
      }
    }
  }
}

// Assuming this is a Tidal Notation Pattern
void play_array(std::shared_ptr<object::Object> pattern, float speed, int dur,
                int vel) {
  std::shared_ptr<object::Array> play_pattern;
  auto pat_obj = std::dynamic_pointer_cast<object::Pattern>(pattern);
  if (pat_obj)
    play_pattern = ExtractArrayFromPattern(pat_obj->Eval());
  else  // Already Eval()'d
    play_pattern = std::dynamic_pointer_cast<object::Array>(pattern);
  if (play_pattern) {
    if (play_pattern->elements_.size() == PPBAR) {
      play_pattern_array(play_pattern, speed, dur, vel);
      return;
    }
  }
}

std::array<int, 16> BitsFromNumber(int num) {
  std::array<int, 16> bits{};
  for (int i = 0; i < 16; i++) {
    if (num & (1 << i)) {
      bits[15 - i] = 1;
    }
  }
  return bits;
}

std::string HexFromNumber(int num) {
  std::string hex_val{};
  const int mymask = 15;

  // auto bits = BitsFromNumber(mymask);
  // for (const auto &b : bits) {
  //   std::cout << b << " ";
  // }

  for (int i = 0; i < 4; i++) {
    int shiftr = 4 * (3 - i);
    int nibble = (num & (mymask << (shiftr))) >> shiftr;
    if (nibble == 0)
      hex_val.append("0");
    else if (nibble == 1)
      hex_val.append("1");
    else if (nibble == 2)
      hex_val.append("2");
    else if (nibble == 3)
      hex_val.append("3");
    else if (nibble == 4)
      hex_val.append("4");
    else if (nibble == 5)
      hex_val.append("5");
    else if (nibble == 6)
      hex_val.append("6");
    else if (nibble == 7)
      hex_val.append("7");
    else if (nibble == 8)
      hex_val.append("8");
    else if (nibble == 9)
      hex_val.append("9");
    else if (nibble == 10)
      hex_val.append("A");
    else if (nibble == 11)
      hex_val.append("B");
    else if (nibble == 12)
      hex_val.append("C");
    else if (nibble == 13)
      hex_val.append("D");
    else if (nibble == 14)
      hex_val.append("E");
    else if (nibble == 15)
      hex_val.append("F");
    else
      hex_val.append("0");
  }
  return hex_val;
}

}  // namespace

namespace builtin {

std::unordered_map<std::string, std::shared_ptr<object::BuiltIn>> built_ins = {
    {"bits", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object> {
                   if (input.size() != 1)
                     return evaluator::NewError(
                         "Too many arguments for 'bits' - can only accept "
                         "one");

                   auto num_obj =
                       std::dynamic_pointer_cast<object::Number>(input[0]);
                   if (num_obj) {
                     auto bits = BitsFromNumber(int(num_obj->value_));
                     auto bit_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());
                     auto zero = std::make_shared<object::Number>(0);
                     auto one = std::make_shared<object::Number>(1);
                     for (int i = 0; i < 16; i++) {
                       if (bits[i] == 1) {
                         bit_array->elements_.push_back(one);
                       } else {
                         bit_array->elements_.push_back(zero);
                       }
                     }
                     return bit_array;
                   }
                   return evaluator::NULLL;
                 })},
    {"hex", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> input)
                    -> std::shared_ptr<object::Object> {
                  if (input.size() != 1)
                    return evaluator::NewError(
                        "Too many arguments for 'hex' - can only accept "
                        "one");

                  auto num_obj =
                      std::dynamic_pointer_cast<object::Number>(input[0]);
                  if (num_obj) {
                    auto hex_val = HexFromNumber(int(num_obj->value_));
                    auto hex_string = std::make_shared<object::String>(hex_val);
                    return hex_string;
                  }
                  return evaluator::NULLL;
                })},
    {"len",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 1)
             return evaluator::NewError(
                 "Too many arguments for len - can only accept "
                 "one");

           std::shared_ptr<object::String> str_obj =
               std::dynamic_pointer_cast<object::String>(input[0]);
           if (str_obj) {
             return std::make_shared<object::Number>(str_obj->value_.size());
           }

           std::shared_ptr<object::Array> array_obj =
               std::dynamic_pointer_cast<object::Array>(input[0]);
           if (array_obj) {
             return std::make_shared<object::Number>(
                 array_obj->elements_.size());
           }

           std::shared_ptr<object::Hash> map_obj =
               std::dynamic_pointer_cast<object::Hash>(input[0]);
           if (map_obj) {
             return std::make_shared<object::Number>(map_obj->pairs_.size());
           }

           return evaluator::NewError("argument to `len` not supported, got %s",
                                      input[0]->Type());
         })},
    {"keys", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object> {
                   if (input.size() != 1)
                     return evaluator::NewError(
                         "Too many arguments for keys - can only accept "
                         "one");

                   std::shared_ptr<object::Hash> map_obj =
                       std::dynamic_pointer_cast<object::Hash>(input[0]);

                   if (map_obj) {
                     auto return_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());

                     for (const auto &pair : map_obj->pairs_) {
                       return_array->elements_.push_back(pair.second.key_);
                     }

                     return return_array;
                   }

                   return evaluator::NewError(
                       "argument to `keys` not supported, got %s",
                       input[0]->Type());
                 })},
    {"head", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object> {
                   if (input.size() != 1)
                     return evaluator::NewError(
                         "Too many arguments for len - can only accept "
                         "one");

                   std::shared_ptr<object::Array> array_obj =
                       std::dynamic_pointer_cast<object::Array>(input[0]);
                   if (!array_obj) {
                     return evaluator::NewError(
                         "argument to `head` must be an array - got %s",
                         input[0]->Type());
                   }

                   if (array_obj->elements_.size() > 0)
                     return array_obj->elements_[0];

                   return evaluator::NULLL;
                 })},
    {"floor", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
                    if (args.size() != 1)
                      return evaluator::NewError("Need WAN arg for floor!");
                    auto number =
                        std::dynamic_pointer_cast<object::Number>(args[0]);
                    if (number) {
                      int floor_num = floor(number->value_);
                      return std::make_shared<object::Number>(floor_num);
                    }
                    return evaluator::NULLL;
                  })},
    {"log", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object> {
                  if (args.size() != 1)
                    return evaluator::NewError("Need WAN arg for log!");
                  auto number =
                      std::dynamic_pointer_cast<object::Number>(args[0]);
                  if (number) {
                    int log_2_num = std::log2(number->value_);
                    return std::make_shared<object::Number>(log_2_num);
                  }
                  return evaluator::NULLL;
                })},
    {"take_n",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 2)
             return evaluator::NewError(
                 "`take_n` requires two args - an array "
                 "plus a number of values to return.");

           auto number = std::dynamic_pointer_cast<object::Number>(input[1]);

           std::shared_ptr<object::Array> array_obj =
               std::dynamic_pointer_cast<object::Array>(input[0]);
           if (array_obj && number) {
             auto num_to_take =
                 std::min(static_cast<unsigned long>(number->value_),
                          array_obj->elements_.size());

             auto return_array = std::make_shared<object::Array>(
                 std::vector<std::shared_ptr<object::Object>>());

             for (unsigned long i = 0; i < num_to_take; i++) {
               return_array->elements_.push_back(array_obj->elements_[i]);
             }

             return return_array;
           }

           return evaluator::NULLL;
         })},
    {"take_random_n",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 2)
             return evaluator::NewError(
                 "`take_random_n` requires two args - an array "
                 "plus a number of values to return.");

           auto number = std::dynamic_pointer_cast<object::Number>(input[1]);

           std::shared_ptr<object::Array> array_obj =
               std::dynamic_pointer_cast<object::Array>(input[0]);
           if (array_obj && number) {
             auto num_to_take =
                 std::min(static_cast<unsigned long>(number->value_),
                          array_obj->elements_.size());

             auto return_array = std::make_shared<object::Array>(
                 std::vector<std::shared_ptr<object::Object>>());

             unsigned int num_taken = 0;
             while (num_taken < num_to_take) {
               auto idx = rand() % array_obj->elements_.size();
               auto it = find(return_array->elements_.begin(),
                              return_array->elements_.end(),
                              array_obj->elements_[idx]);
               if (it != return_array->elements_.end()) {
                 continue;
               }
               return_array->elements_.push_back(array_obj->elements_[idx]);
               ++num_taken;
             }

             return return_array;
           }

           return evaluator::NULLL;
         })},
    {"stepn", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> input)
                      -> std::shared_ptr<object::Object> {
                    if (input.size() != 1)
                      return evaluator::NewError(
                          "`stepn` requires one arg - a Step Sequencer.");

                    auto step_obj =
                        std::dynamic_pointer_cast<object::StepSequencer>(
                            input[0]);

                    if (step_obj) {
                      double retval = step_obj->steppa_.GenNext();
                      return std::make_shared<object::Number>(retval);
                    }

                    return evaluator::NULLL;
                  })},
    {"sched",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() != 5)
             return evaluator::NewError(
                 "`sched` requires 5 arg - when, start_val, end_val, "
                 "time_to_take and action_to_take(string).");

           auto when = std::dynamic_pointer_cast<object::Number>(args[0]);
           auto start_val = std::dynamic_pointer_cast<object::Number>(args[1]);
           auto end_val = std::dynamic_pointer_cast<object::Number>(args[2]);
           auto time_taken = std::dynamic_pointer_cast<object::Number>(args[3]);
           auto action_to_take =
               std::dynamic_pointer_cast<object::String>(args[4]);

           if (when && start_val && end_val && time_taken && action_to_take) {
             Action action = Action(start_val->value_, end_val->value_,
                                    time_taken->value_, action_to_take->value_);
             mixr->ScheduleAction(when->value_, action);
           }
           return evaluator::NULLL;
         })},
    {"is_in",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 2)
             return evaluator::NewError(
                 "`is_in` requires two args - an array "
                 "plus an item to find. It will return a boolean to answer if "
                 "item is in array.");

           auto item = input[1];

           std::shared_ptr<object::Array> array_obj =
               std::dynamic_pointer_cast<object::Array>(input[0]);

           if (item && array_obj) {
             for (const auto &e : array_obj->elements_) {
               if (item->Inspect() == e->Inspect()) {
                 return std::make_shared<object::Boolean>(true);
               }
             }
             return std::make_shared<object::Boolean>(false);
           }

           return evaluator::NULLL;
         })},
    {"incr",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() != 3)
             return evaluator::NewError(
                 "Too many arguments for incr - need three - "
                 "number to "
                 "incr, min and max");
           auto number = std::dynamic_pointer_cast<object::Number>(args[0]);
           auto min = std::dynamic_pointer_cast<object::Number>(args[1]);
           auto max = std::dynamic_pointer_cast<object::Number>(args[2]);
           if (number && min && max) {
             int incr_num = number->value_;
             incr_num++;
             if (incr_num >= max->value_) {
               incr_num = min->value_;
             }
             return std::make_shared<object::Number>(incr_num);
           }
           return evaluator::NULLL;
         })},
    {"rincr",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() != 3)
             return evaluator::NewError(
                 "Too many arguments for incr - need three - "
                 "number to "
                 "incr, min and max");
           auto number = std::dynamic_pointer_cast<object::Number>(args[0]);
           auto min = std::dynamic_pointer_cast<object::Number>(args[1]);
           auto max = std::dynamic_pointer_cast<object::Number>(args[2]);
           if (number && min && max) {
             int incr_num = number->value_;
             incr_num--;
             if (incr_num < min->value_) {
               incr_num = max->value_ - 1;
             }
             return std::make_shared<object::Number>(incr_num);
           }
           return evaluator::NULLL;
         })},
    {"dincr",  // drunk incr
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() != 3)
             return evaluator::NewError(
                 "Too many arguments for incr - need three - "
                 "number to "
                 "incr, min and max");
           auto number = std::dynamic_pointer_cast<object::Number>(args[0]);
           auto min = std::dynamic_pointer_cast<object::Number>(args[1]);
           auto max = std::dynamic_pointer_cast<object::Number>(args[2]);
           if (number && min && max) {
             int incr_num = number->value_;
             if (rand() % 100 > 50) {
               incr_num++;
             } else {
               incr_num--;
             }
             if (incr_num >= max->value_) {
               incr_num = min->value_;
             }
             if (incr_num < min->value_) {
               incr_num = max->value_;
             }
             return std::make_shared<object::Number>(incr_num);
           }
           return evaluator::NULLL;
         })},
    {"tail", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object> {
                   if (input.size() != 1)
                     return evaluator::NewError(
                         "Too many arguments for `tail` - can only "
                         "accept one");

                   std::shared_ptr<object::Array> array_obj =
                       std::dynamic_pointer_cast<object::Array>(input[0]);
                   if (!array_obj) {
                     return evaluator::NewError(
                         "argument to `tail` must be an array - got %s",
                         input[0]->Type());
                   }

                   int len_elems = array_obj->elements_.size();
                   if (len_elems > 0) {
                     auto return_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());

                     for (int i = 1; i < len_elems; i++)
                       return_array->elements_.push_back(
                           array_obj->elements_[i]);
                     return return_array;
                   }
                   return evaluator::NULLL;
                 })},
    {"last", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object> {
                   if (input.size() != 1)
                     return evaluator::NewError(
                         "Too many arguments for `last` - can only "
                         "accept one");

                   std::shared_ptr<object::Array> array_obj =
                       std::dynamic_pointer_cast<object::Array>(input[0]);
                   if (!array_obj) {
                     return evaluator::NewError(
                         "argument to `last` must be an array - got %s",
                         input[0]->Type());
                   }

                   int len_elems = array_obj->elements_.size();
                   if (len_elems > 0) {
                     return array_obj->elements_[len_elems - 1];
                   }

                   return evaluator::NULLL;
                 })},
    {"lowercase",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 1)
             return evaluator::NewError(
                 "Too many arguments for `lowercase` - can only "
                 "accept one");

           auto string_obj =
               std::dynamic_pointer_cast<object::String>(input[0]);
           if (!string_obj) {
             return evaluator::NewError(
                 "argument to `lowercase` must be a string - got %s",
                 input[0]->Type());
           }

           std::string lcstring = string_obj->value_;
           std::transform(lcstring.begin(), lcstring.end(), lcstring.begin(),
                          [](unsigned char c) { return std::tolower(c); });

           return std::make_shared<object::String>(lcstring);
         })},
    {"uppercase",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 1)
             return evaluator::NewError(
                 "Too many arguments for `uppercase` - can only "
                 "accept one");

           auto string_obj =
               std::dynamic_pointer_cast<object::String>(input[0]);
           if (!string_obj) {
             return evaluator::NewError(
                 "argument to `uppercase` must be a string - got %s",
                 input[0]->Type());
           }

           std::string ucstring = string_obj->value_;
           std::transform(ucstring.begin(), ucstring.end(), ucstring.begin(),
                          [](unsigned char c) { return std::toupper(c); });

           return std::make_shared<object::String>(ucstring);
         })},
    {"max",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 2)
             return evaluator::NewError("Need 2 args for 'max' function");

           auto first_num = std::dynamic_pointer_cast<object::Number>(input[0]);
           auto second_num =
               std::dynamic_pointer_cast<object::Number>(input[1]);
           if (!first_num || !second_num) {
             return evaluator::NewError(
                 "argument to `max` must be two numbers.");
           }
           return std::make_shared<object::Number>(
               std::max(first_num->value_, second_num->value_));
         })},
    {"min",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 2)
             return evaluator::NewError("Need 2 args for 'min' function");

           auto first_num = std::dynamic_pointer_cast<object::Number>(input[0]);
           auto second_num =
               std::dynamic_pointer_cast<object::Number>(input[1]);
           if (!first_num || !second_num) {
             return evaluator::NewError(
                 "argument to `min` must be two numbers.");
           }
           return std::make_shared<object::Number>(
               std::min(first_num->value_, second_num->value_));
         })},
    {"midi_ref", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> args)
                         -> std::shared_ptr<object::Object> {
                       (void)args;
                       mixr->PrintMidiInfo();
                       return evaluator::NULLL;
                     })},
    {"now", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object> {
                  auto action =
                      std::make_unique<AudioActionItem>(AudioAction::NOW);
                  audio_queue.push(std::move(action));
                  int val = -1;
                  auto current_tick = audio_reply_queue.pop();
                  if (current_tick) val = current_tick.value();
                  return std::make_shared<object::Number>(val);
                })},
    {"algoz", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
                    (void)args;
                    mixr->PrintDxAlgos();
                    return evaluator::NULLL;
                  })},
    {"ratioz", std::make_shared<object::BuiltIn>(
                   [](std::vector<std::shared_ptr<object::Object>> args)
                       -> std::shared_ptr<object::Object> {
                     (void)args;
                     mixr->PrintDxRatioz();
                     return evaluator::NULLL;
                   })},
    {"push", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object> {
                   if (input.size() != 2)
                     return evaluator::NewError(
                         "`push` requires two arguments - array and "
                         "object");

                   std::shared_ptr<object::Array> array_obj =
                       std::dynamic_pointer_cast<object::Array>(input[0]);
                   if (!array_obj) {
                     return evaluator::NewError(
                         "argument to `push` must be an array - got %s",
                         input[0]->Type());
                   }

                   // auto return_array = std::make_shared<object::Array>(
                   //    std::vector<std::shared_ptr<object::Object>>());

                   // int len_elems = array_obj->elements_.size();
                   // for (int i = 0; i < len_elems; i++)
                   //    return_array->elements_.push_back(
                   //        array_obj->elements_[i]);

                   array_obj->elements_.push_back(input[1]);

                   return array_obj;
                 })},
    {"print", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
                    std::stringstream out;
                    for (auto &o : args) {
                      out << o->Inspect();
                    }

                    std::cout << out.str() << std::endl;

                    return evaluator::NULLL;
                  })},
    {"funcz", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
                    (void)args;
                    mixr->PrintFuncAndGenInfo();
                    return evaluator::NULLL;
                  })},
    {"timing_info", std::make_shared<object::BuiltIn>(
                        [](std::vector<std::shared_ptr<object::Object>> args)
                            -> std::shared_ptr<object::Object> {
                          (void)args;
                          mixr->PrintTimingInfo();
                          return evaluator::NULLL;
                        })},
    {"reverse",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 1)
             return evaluator::NewError(
                 "`reverse` requires a single array or string "
                 "argument.");

           std::shared_ptr<object::Array> array_obj =
               std::dynamic_pointer_cast<object::Array>(input[0]);
           if (array_obj) {
             auto return_array =
                 std::make_shared<object::Array>(array_obj->elements_);

             std::reverse(return_array->elements_.begin(),
                          return_array->elements_.end());
             return return_array;
           }

           auto midi_array =
               std::dynamic_pointer_cast<object::MidiArray>(input[0]);
           if (midi_array) {
             auto return_array =
                 std::make_shared<object::MidiArray>(midi_array->notes_on_);

             for (auto &e : return_array->notes_on_) {
               e.playback_tick = PPBAR - e.playback_tick;
             }

             return return_array;
           }

           std::shared_ptr<object::String> string_obj =
               std::dynamic_pointer_cast<object::String>(input[0]);
           if (string_obj) {
             std::string reversed_string = string_obj->value_;
             reverse(reversed_string.begin(), reversed_string.end());
             return std::make_shared<object::String>(reversed_string);
           }

           return evaluator::NULLL;
         })},
    {"rotate",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 2)
             return evaluator::NewError(
                 "`rotate` requires two args - an array or "
                 "string plus a num of positions to rotate.");

           auto number = std::dynamic_pointer_cast<object::Number>(input[1]);

           auto pat_obj = std::dynamic_pointer_cast<object::Pattern>(input[0]);
           if (pat_obj && number) {
             auto evaluated_pat = pat_obj->Eval();
             auto return_array = ExtractArrayFromPattern(evaluated_pat);

             auto rotate_by =
                 (int)number->value_ % return_array->elements_.size();

             if (return_array->elements_.size() == 3840 && rotate_by < 16) {
               rotate_by *= 240;
             }

             std::rotate(return_array->elements_.begin(),
                         return_array->elements_.begin() + rotate_by,
                         return_array->elements_.end());

             return return_array;
           }

           std::shared_ptr<object::Array> array_obj =
               std::dynamic_pointer_cast<object::Array>(input[0]);
           if (array_obj && number) {
             auto return_array =
                 std::make_shared<object::Array>(array_obj->elements_);

             auto rotate_by =
                 (int)number->value_ % return_array->elements_.size();

             if (array_obj->elements_.size() == 3840 && rotate_by < 16) {
               rotate_by *= 240;
             }

             std::rotate(return_array->elements_.begin(),
                         return_array->elements_.begin() + rotate_by,
                         return_array->elements_.end());

             return return_array;
           }

           auto midi_array =
               std::dynamic_pointer_cast<object::MidiArray>(input[0]);
           if (midi_array && number) {
             auto return_array =
                 std::make_shared<object::MidiArray>(midi_array->notes_on_);

             auto rotate_by = number->value_ * 240;

             std::cout << "ROTATING MIDI ARRAY BY " << number->value_ << " ("
                       << rotate_by << ")" << std::endl;

             for (auto &e : return_array->notes_on_) {
               e.playback_tick = (int)(e.playback_tick + rotate_by) % PPBAR;
             }

             return return_array;
           }

           std::shared_ptr<object::String> string_obj =
               std::dynamic_pointer_cast<object::String>(input[0]);
           if (string_obj && number) {
             std::string rotated_string = string_obj->value_;
             std::rotate(rotated_string.begin(),
                         rotated_string.begin() + number->value_,
                         rotated_string.end());
             return std::make_shared<object::String>(rotated_string);
           }

           return evaluator::NULLL;
         })},
    {"send",
     std::make_shared<
         object::BuiltIn>([](std::vector<std::shared_ptr<object::Object>> input)
                              -> std::shared_ptr<object::Object> {
       if (input.size() < 2)
         return evaluator::NewError(
             "`send` requires at least two args - destination fx and sources");

       auto at_obj = std::find_if(input.begin(), input.end(),
                                  [](std::shared_ptr<object::Object> o) {
                                    return o->Type() == object::AT_OBJ;
                                  });
       int delayed_by{0};
       if (at_obj != input.end()) {
         auto at = std::dynamic_pointer_cast<object::At>(*at_obj);
         if (at) {
           delayed_by = at->value_;
         }
       }

       int destination_fx = -1;
       auto fx_number_obj = std::dynamic_pointer_cast<object::Number>(input[0]);
       if (fx_number_obj) {
         destination_fx = fx_number_obj->value_;
       } else {
         auto fx_string_obj =
             std::dynamic_pointer_cast<object::String>(input[0]);
         if (fx_string_obj) {
           std::string dest = fx_string_obj->value_;
           if (dest == "delay")
             destination_fx = 0;
           else if (dest == "reverb")
             destination_fx = 1;
           else if (dest == "distort")
             destination_fx = 2;
         }
       }
       if (destination_fx < 0 || destination_fx > 2) {
         std::cerr << "FX NUM:" << destination_fx
                   << " isnae valid - whit's going on?" << std::endl;
         return evaluator::NULLL;
       }

       auto action =
           std::make_unique<AudioActionItem>(AudioAction::MIXER_FX_UPDATE);
       action->mixer_fx_id = destination_fx;
       action->delayed_by = delayed_by;

       auto sg_array = std::dynamic_pointer_cast<object::Array>(input[1]);
       if (sg_array) {
         for (auto const &e : sg_array->elements_) {
           auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(e);
           if (sg) {
             action->group_of_soundgens.push_back(sg->soundgen_id_);
           }
         }
       } else {
         auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(input[1]);
         if (sg) {
           action->group_of_soundgens.push_back(sg->soundgen_id_);
         }
       }
       if (action->group_of_soundgens.size() == 0) {
         std::cout << "NO SOUNDGENS FOUND, RETURNING..\n";
         return evaluator::NULLL;
       }

       double intensity_val = 0.4;
       if (input.size() == 3) {
         auto intensity_obj =
             std::dynamic_pointer_cast<object::Number>(input[2]);
         if (intensity_obj) {
           intensity_val = intensity_obj->value_;
         }
       }
       action->fx_intensity = intensity_val;
       audio_queue.push(std::move(action));
       return evaluator::NULLL;
     })},
    {"xassign",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() < 2)
             return evaluator::NewError(
                 "`send` requires at least two args - channel side and source");

           auto at_obj = std::find_if(input.begin(), input.end(),
                                      [](std::shared_ptr<object::Object> o) {
                                        return o->Type() == object::AT_OBJ;
                                      });
           int delayed_by{0};
           if (at_obj != input.end()) {
             auto at = std::dynamic_pointer_cast<object::At>(*at_obj);
             if (at) {
               delayed_by = at->value_;
             }
           }

           auto xfade_left_or_right =
               std::dynamic_pointer_cast<object::Number>(input[0]);
           if (xfade_left_or_right) {
             auto action = std::make_unique<AudioActionItem>(
                 AudioAction::MIXER_XFADE_ASSIGN);
             action->delayed_by = delayed_by;
             action->xfade_channel =
                 static_cast<unsigned int>(xfade_left_or_right->value_);

             auto sg_array = std::dynamic_pointer_cast<object::Array>(input[1]);
             if (sg_array) {
               for (auto const &e : sg_array->elements_) {
                 auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(e);
                 if (sg) {
                   action->group_of_soundgens.push_back(sg->soundgen_id_);
                 }
               }
             } else {
               auto sg =
                   std::dynamic_pointer_cast<object::SoundGenerator>(input[1]);
               if (sg) {
                 action->group_of_soundgens.push_back(sg->soundgen_id_);
               }
             }
             if (action->group_of_soundgens.size() == 0) {
               std::cout << "NO SOUNDGENS FOUND, RETURNING..\n";
               return evaluator::NULLL;
             }
             audio_queue.push(std::move(action));
           }
           return evaluator::NULLL;
         })},
    {"xclear", std::make_shared<object::BuiltIn>(
                   [](std::vector<std::shared_ptr<object::Object>> input)
                       -> std::shared_ptr<object::Object> {
                     auto at_obj =
                         std::find_if(input.begin(), input.end(),
                                      [](std::shared_ptr<object::Object> o) {
                                        return o->Type() == object::AT_OBJ;
                                      });
                     int delayed_by{0};
                     if (at_obj != input.end()) {
                       auto at = std::dynamic_pointer_cast<object::At>(*at_obj);
                       if (at) {
                         delayed_by = at->value_;
                       }
                     }

                     auto action = std::make_unique<AudioActionItem>(
                         AudioAction::MIXER_XFADE_CLEAR);
                     action->delayed_by = delayed_by;

                     audio_queue.push(std::move(action));
                     return evaluator::NULLL;
                   })},
    {"xfade",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() == 0)
             return evaluator::NewError(
                 "`xfade` requires at least one arg - a direction LEFT(0) or "
                 "RIGHT(1)");

           auto at_obj = std::find_if(input.begin(), input.end(),
                                      [](std::shared_ptr<object::Object> o) {
                                        return o->Type() == object::AT_OBJ;
                                      });
           int delayed_by{0};
           if (at_obj != input.end()) {
             auto at = std::dynamic_pointer_cast<object::At>(*at_obj);
             if (at) {
               delayed_by = at->value_;
             }
           }

           auto num_obj = std::dynamic_pointer_cast<object::Number>(input[0]);
           if (num_obj) {
             auto action = std::make_unique<AudioActionItem>(
                 AudioAction::MIXER_XFADE_ACTION);
             action->delayed_by = delayed_by;
             action->xfade_direction =
                 static_cast<unsigned int>(num_obj->value_);
             audio_queue.push(std::move(action));
           }
           return evaluator::NULLL;
         })},
    {"is_array", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> input)
                         -> std::shared_ptr<object::Object> {
                       if (input.size() != 1)
                         return evaluator::NewError(
                             "`is_array` requires a single arg - an array");

                       std::shared_ptr<object::Array> array_obj =
                           std::dynamic_pointer_cast<object::Array>(input[0]);
                       if (array_obj) {
                         return evaluator::NativeBoolToBooleanObject(true);
                       }
                       return evaluator::NativeBoolToBooleanObject(false);
                     })},
    {"invert",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
           if (input.size() != 1)
             return evaluator::NewError(
                 "`invert` requires a single args - an array");

           std::shared_ptr<object::Array> array_obj =
               std::dynamic_pointer_cast<object::Array>(input[0]);
           if (array_obj) {
             auto return_array = std::make_shared<object::Array>(
                 std::vector<std::shared_ptr<object::Object>>());

             for (auto e : array_obj->elements_) {
               std::shared_ptr<object::Number> num_obj =
                   std::dynamic_pointer_cast<object::Number>(e);
               if (num_obj) {
                 auto new_num_obj =
                     std::make_shared<object::Number>(!num_obj->value_);
                 return_array->elements_.push_back(new_num_obj);
               } else if (e->Type() == "NULL") {
                 auto new_num_obj = std::make_shared<object::Number>(1);
                 return_array->elements_.push_back(new_num_obj);
               } else if (e->Type() == "ARRAY") {
                 std::shared_ptr<object::Array> inner_array_obj =
                     std::dynamic_pointer_cast<object::Array>(e);
                 if (inner_array_obj) {
                   if (inner_array_obj->elements_.size() > 0) {
                     auto new_num_obj = std::make_shared<object::Number>(1);
                     return_array->elements_.push_back(new_num_obj);
                   } else {
                     auto new_num_obj = std::make_shared<object::Number>(0);
                     return_array->elements_.push_back(new_num_obj);
                   }
                 } else {
                   auto new_num_obj = std::make_shared<object::Number>(0);
                   return_array->elements_.push_back(new_num_obj);
                 }
               } else {
                 auto new_num_obj = std::make_shared<object::Number>(0);
                 return_array->elements_.push_back(new_num_obj);
               }
             }

             return return_array;
           }
           auto pattern_obj =
               std::dynamic_pointer_cast<object::Pattern>(input[0]);
           if (pattern_obj) {
             auto evaluated_pat = pattern_obj->Print();
             auto intz = ExtractIntsFromEvalPattern(evaluated_pat);
             auto stepseq = ShrinkPatternToStepSequence(intz);

             auto return_array = std::make_shared<object::Array>(
                 std::vector<std::shared_ptr<object::Object>>());

             for (int i = 0; i < (int)stepseq.size(); ++i) {
               auto new_num_obj = std::make_shared<object::Number>(!stepseq[i]);
               return_array->elements_.push_back(new_num_obj);
             }
             return return_array;
           }

           return evaluator::NULLL;
         })},
    {"solo",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           auto at_obj = std::find_if(args.begin(), args.end(),
                                      [](std::shared_ptr<object::Object> o) {
                                        return o->Type() == object::AT_OBJ;
                                      });
           int delayed_by{0};
           if (at_obj != args.end()) {
             auto at = std::dynamic_pointer_cast<object::At>(*at_obj);
             if (at) {
               delayed_by = at->value_;
               int dif_til_next_loop = (3840 - delayed_by) % 3840;
               auto action =
                   std::make_unique<AudioActionItem>(AudioAction::UNSOLO);
               action->delayed_by = delayed_by + dif_til_next_loop;
               audio_queue.push(std::move(action));
             }
           }
           for (size_t i = 0; i < args.size(); i++) {
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[i]);
             if (soundgen) {
               auto action =
                   std::make_unique<AudioActionItem>(AudioAction::SOLO);
               action->soundgen_num = soundgen->soundgen_id_;
               action->delayed_by = delayed_by;
               audio_queue.push(std::move(action));
             }
           }
           return evaluator::NULLL;
         })},
    {"unsolo", std::make_shared<object::BuiltIn>(
                   [](std::vector<std::shared_ptr<object::Object>> args)
                       -> std::shared_ptr<object::Object> {
                     auto at_obj =
                         std::find_if(args.begin(), args.end(),
                                      [](std::shared_ptr<object::Object> o) {
                                        return o->Type() == object::AT_OBJ;
                                      });
                     int delayed_by{0};
                     if (at_obj != args.end()) {
                       auto at = std::dynamic_pointer_cast<object::At>(*at_obj);
                       if (at) delayed_by = at->value_;
                     }
                     auto action =
                         std::make_unique<AudioActionItem>(AudioAction::UNSOLO);
                     action->delayed_by = delayed_by;
                     audio_queue.push(std::move(action));
                     return evaluator::NULLL;
                   })},
    {"mvol", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> args)
                     -> std::shared_ptr<object::Object> {
                   if (args.size() != 1)
                     return evaluator::NewError(
                         "`Master Volume` requires a volume value.");

                   auto num_obj =
                       std::dynamic_pointer_cast<object::Number>(args[0]);
                   if (!num_obj) return evaluator::NULLL;

                   auto action =
                       std::make_unique<AudioActionItem>(AudioAction::VOLUME);
                   action->new_volume = num_obj->value_;
                   audio_queue.push(std::move(action));
                   return evaluator::NULLL;
                 })},
    {"stop",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() != 1)
             return evaluator::NewError(
                 "`stop` requires a sound_generator target");

           auto soundgen =
               std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (soundgen) {
             auto action = std::make_unique<AudioActionItem>(AudioAction::STOP);
             action->soundgen_num = soundgen->soundgen_id_;
             audio_queue.push(std::move(action));
           }
           return evaluator::NULLL;
         })},
    {"note_on",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() < 2)
             return evaluator::NewError(
                 "`note_on` requires at least two "
                 "args - a sound_generator target "
                 "and a midi_note to play.");

           auto soundgen =
               std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (soundgen) {
             int sgid = soundgen->soundgen_id_;
             std::vector<int> midi_nums = GetMidiNotes(args[1]);
             int vel = 128;
             int dur = 240;

             for (auto a : args) {
               if (a->Type() == object::DURATION_OBJ) {
                 auto dur_obj = std::dynamic_pointer_cast<object::Duration>(a);
                 dur = dur_obj->value_;
               } else if (a->Type() == object::VELOCITY_OBJ) {
                 auto vel_obj = std::dynamic_pointer_cast<object::Velocity>(a);
                 vel = vel_obj->value_;
               }
             }

             note_on(sgid, midi_nums, vel, dur);
           }

           return evaluator::NULLL;
         })},
    {"note_on_at",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() < 3)
             return evaluator::NewError(
                 "`note_on_at` requires at least three args - a "
                 "sound_generator target, a midi_note to play and a time "
                 "in the future specified in midi ticks.");

           auto soundgen =
               std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (soundgen) {
             int sgid = soundgen->soundgen_id_;
             std::vector<int> midi_nums = GetMidiNotes(args[1]);

             auto num_obj = std::dynamic_pointer_cast<object::Number>(args[2]);

             if (!num_obj) return evaluator::NULLL;

             int vel = 128;
             int dur = 240;
             int note_start_time = num_obj->value_;

             for (auto a : args) {
               if (a->Type() == object::DURATION_OBJ) {
                 auto dur_obj = std::dynamic_pointer_cast<object::Duration>(a);
                 dur = dur_obj->value_;
               } else if (a->Type() == object::VELOCITY_OBJ) {
                 auto vel_obj = std::dynamic_pointer_cast<object::Velocity>(a);
                 vel = vel_obj->value_;
               }
             }

             note_on_at(sgid, midi_nums, note_start_time, vel, dur);
           }

           return evaluator::NULLL;
         })},
    {"note_off", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> args)
                         -> std::shared_ptr<object::Object> {
                       if (args.size() == 0)
                         return evaluator::NewError(
                             "`note_off` requires at least one arg - "
                             "a sound_generator target "
                             "to stop.");

                       auto soundgen =
                           std::dynamic_pointer_cast<object::SoundGenerator>(
                               args[0]);
                       if (soundgen) {
                         int sgid = soundgen->soundgen_id_;
                         std::vector<int> midi_nums;
                         if (args.size() > 1) {
                           midi_nums = GetMidiNotes(args[1]);
                         }
                         note_off(sgid, midi_nums);
                       }
                       return evaluator::NULLL;
                     })},
    {"note_off_at",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() < 3)
             return evaluator::NewError(
                 "`note_off_at` requires at least three args - a "
                 "sound_generator target, a midi_note to stop and a time "
                 "in the future specified in midi ticks.");

           auto soundgen =
               std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (soundgen) {
             int sgid = soundgen->soundgen_id_;
             std::vector<int> midi_nums = GetMidiNotes(args[1]);

             auto num_obj = std::dynamic_pointer_cast<object::Number>(args[2]);

             if (!num_obj) return evaluator::NULLL;

             int note_stop_time = num_obj->value_;

             note_off_at(sgid, midi_nums, note_stop_time);
           }
           return evaluator::NULLL;
         })},
    {"set_pitch",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size >= 2) {
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen) {
               auto number = std::dynamic_pointer_cast<object::Number>(args[1]);
               if (number) {
                 int delayed_by = 0;
                 if (args_size == 3) {
                   auto delayed_time =
                       std::dynamic_pointer_cast<object::Number>(args[2]);
                   if (delayed_time) delayed_by = delayed_time->value_;
                 }
                 auto action =
                     std::make_unique<AudioActionItem>(AudioAction::UPDATE);
                 action->mixer_soundgen_idx = soundgen->soundgen_id_;
                 action->delayed_by = delayed_by;
                 action->fx_id = -1;
                 action->param_name = "pitch";
                 action->param_val = std::to_string(number->value_);
                 audio_queue.push(std::move(action));
               }
             }
           }
           return evaluator::NULLL;
         })},
    {"speed", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
                    std::cout << "INBUILT SPEED CALLED!" << std::endl;
                    (void)args;
                    return evaluator::NULLL;
                  })},
    {"add_fx",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size >= 2) {
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen && mixr->IsValidSoundgenNum(soundgen->soundgen_id_)) {
               auto fx = interpreter_sound_cmds::ParseFXCmd(args);
               if (fx.size() > 0) {
                 auto action =
                     std::make_unique<AudioActionItem>(AudioAction::ADD_FX);
                 action->soundgen_num = soundgen->soundgen_id_;
                 action->fx = fx;
                 audio_queue.push(std::move(action));
               }
             }
           }
           return evaluator::NULLL;
         })},
    {"add_buf",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size >= 2) {
             std::cout << "YO ADD BUFFER!\n";
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen && mixr->IsValidSoundgenNum(soundgen->soundgen_id_)) {
               if (args[1]->Type() == "STRING") {
                 std::cout << args[1]->Inspect() << " " << args[1]->Type()
                           << std::endl;
                 auto fb =
                     std::make_unique<SBAudio::FileBuffer>(args[1]->Inspect());
                 auto action =
                     std::make_unique<AudioActionItem>(AudioAction::ADD_BUFFER);
                 action->soundgen_num = soundgen->soundgen_id_;
                 action->fb = std::move(fb);
                 audio_queue.push(std::move(action));
               }
             }
           }
           return evaluator::NULLL;
         })},
    {"monitor", std::make_shared<object::BuiltIn>(
                    [](std::vector<std::shared_ptr<object::Object>> args)
                        -> std::shared_ptr<object::Object> {
                      int args_size = args.size();
                      if (args_size == 1) {
                        std::shared_ptr<object::String> filename =
                            std::dynamic_pointer_cast<object::String>(args[0]);
                        auto cwd = fs::current_path();
                        std::string filepath =
                            cwd.generic_string() + "/" + filename->value_;

                        auto action = std::make_unique<AudioActionItem>(
                            AudioAction::MONITOR);
                        action->filepath = filepath;
                        audio_queue.push(std::move(action));
                        repl_queue.push("Monitoring " + filepath);
                      } else
                        std::cerr << "BARF! ARG SIZE SHOULD BE 1 -  SIZE IS "
                                  << args_size << std::endl;
                      return evaluator::NULLL;
                    })},
    {"list_presets",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();

           if (args_size == 1) {
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             auto sg_type = soundgen->soundgenerator_type;
             if (HasPresets(sg_type)) {
               auto preset_names = GetSynthPresets(sg_type);

               for (const auto &p : preset_names) std::cout << p << std::endl;
             }
           }
           return evaluator::NULLL;
         })},
    {"load_preset",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 2) {
             // find out what kind of synth we have
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             auto preset_name =
                 std::dynamic_pointer_cast<object::String>(args[1]);
             if (soundgen && preset_name) {
               auto action =
                   std::make_unique<AudioActionItem>(AudioAction::LOAD_PRESET);
               action->args = args;
               // read its preset file
               action->preset = GetPreset(soundgen->soundgenerator_type,
                                          preset_name->value_);
               action->preset_name = preset_name->value_;
               audio_queue.push(std::move(action));
             }
           }
           return evaluator::NULLL;
         })},
    {"rand_preset",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();

           std::cout << "YO TAND\n";
           if (args_size >= 1) {
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             auto sg_type = soundgen->soundgenerator_type;
             std::cout << "SG YTYPE:" << sg_type << std::endl;
             if (HasPresets(sg_type)) {
               std::cout << "YO SG TYPE IS GOOGD\n";
               auto preset_names = GetSynthPresets(sg_type);
               int randy_int = rand() % preset_names.size();
               auto preset = preset_names[randy_int];
               auto preset_name =
                   std::make_shared<object::String>(preset_names[randy_int]);
               args.push_back(preset_name);
               std::cout << "RANDILY GOING FOR :" << preset_names[randy_int]
                         << std::endl;
               auto cmd_name = std::make_shared<object::String>("load");
               args.push_back(cmd_name);
               auto action =
                   std::make_unique<AudioActionItem>(AudioAction::LOAD_PRESET);
               action->args = args;
               // read its preset file
               action->preset_name = preset_name->value_;
               action->preset = GetPreset(soundgen->soundgenerator_type,
                                          preset_name->value_);
               audio_queue.push(std::move(action));
             }
           }
           return evaluator::NULLL;
         })},
    {"save_preset", std::make_shared<object::BuiltIn>(
                        [](std::vector<std::shared_ptr<object::Object>> args)
                            -> std::shared_ptr<object::Object> {
                          int args_size = args.size();
                          if (args_size >= 2) {
                            auto cmd_name =
                                std::make_shared<object::String>("save");
                            args.push_back(cmd_name);
                            auto action = std::make_unique<AudioActionItem>(
                                AudioAction::SAVE_PRESET);
                            action->args = args;
                            audio_queue.push(std::move(action));
                          }
                          return evaluator::NULLL;
                        })},
    {"rand",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() == 0) {
             auto rand_number =
                 static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
             return std::make_shared<object::Number>(rand_number);
           }

           auto soundgen =
               std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (soundgen) {
             auto action = std::make_unique<AudioActionItem>(AudioAction::RAND);
             action->mixer_soundgen_idx = soundgen->soundgen_id_;
             audio_queue.push(std::move(action));
             return evaluator::NULLL;
           }

           auto array_obj = std::dynamic_pointer_cast<object::Array>(args[0]);
           if (array_obj) {
             int len_elems = array_obj->elements_.size();
             if (len_elems > 0) {
               int idx = rand() % len_elems;
               return array_obj->elements_[idx];
             }
           }

           auto number = std::dynamic_pointer_cast<object::Number>(args[0]);
           if (number) {
             double rand_number = 0;
             if (number->value_ == 1) {
               rand_number = ((double)rand()) / RAND_MAX;
             } else if (number->value_ > 0) {
               rand_number = rand() % (int)number->value_;
             }
             return std::make_shared<object::Number>(rand_number);
           }

           return evaluator::NULLL;
         })},
    {"rand_sixteenthz",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size > 0) {
             auto nom = std::dynamic_pointer_cast<object::Number>(args[0]);
             if (nom) {
               std::vector<int> noms_added;
               auto return_array = std::make_shared<object::Array>(
                   std::vector<std::shared_ptr<object::Object>>());
               while (noms_added.size() < nom->value_) {
                 int sixt_num = std::rand() % 16;
                 if (std::find(noms_added.begin(), noms_added.end(),
                               sixt_num) == noms_added.end()) {
                   noms_added.push_back(sixt_num);
                   auto num_obj = std::make_shared<object::Number>(sixt_num);
                   return_array->elements_.push_back(num_obj);
                 }
               }
               return return_array;
             }
           } else {
             int sixt_num = std::rand() % 16;
             auto number_obj = std::make_shared<object::Number>(sixt_num);
             return number_obj;
           }
           return evaluator::NULLL;
         })},
    {"perlin", std::make_shared<object::BuiltIn>(
                   [](std::vector<std::shared_ptr<object::Object>> args)
                       -> std::shared_ptr<object::Object> {
                     int args_size = args.size();
                     if (args_size == 1) {
                       auto x =
                           std::dynamic_pointer_cast<object::Number>(args[0]);
                       if (x) {
                         auto val = perlinGenerator.noise1D(float(x->value_));
                         auto number_obj =
                             std::make_shared<object::Number>(val);
                         return number_obj;
                       }
                     }
                     return evaluator::NULLL;
                   })},
    {"sort",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           // only implemented for array of Numbers
           if (args.size() == 1) {
             auto array_obj = std::dynamic_pointer_cast<object::Array>(args[0]);

             if (array_obj) {
               std::vector<double> temp_nums;
               for (const auto &e : array_obj->elements_) {
                 auto number = std::dynamic_pointer_cast<object::Number>(e);
                 if (number) {
                   temp_nums.push_back(number->value_);
                 }
               }

               std::sort(temp_nums.begin(), temp_nums.end());
               for (size_t i = 0; i < temp_nums.size(); i++) {
                 auto number = std::dynamic_pointer_cast<object::Number>(
                     array_obj->elements_[i]);
                 if (number) {
                   number->value_ = temp_nums[i];
                 }
               }
             }

             return array_obj;
           }

           return evaluator::NULLL;
         })},
    {"shuffle",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           // only implemented for array of Numbers
           if (args.size() == 1) {
             auto array_obj = std::dynamic_pointer_cast<object::Array>(args[0]);

             if (array_obj) {
               std::vector<double> temp_nums;
               for (const auto &e : array_obj->elements_) {
                 auto number = std::dynamic_pointer_cast<object::Number>(e);
                 if (number) {
                   temp_nums.push_back(number->value_);
                 }
               }
               std::random_device rd;
               std::mt19937 g(rd());

               std::shuffle(temp_nums.begin(), temp_nums.end(), g);
               for (size_t i = 0; i < temp_nums.size(); i++) {
                 auto number = std::dynamic_pointer_cast<object::Number>(
                     array_obj->elements_[i]);
                 if (number) {
                   number->value_ = temp_nums[i];
                 }
               }
             }

             return array_obj;
           }

           return evaluator::NULLL;
         })},
    {"sin", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object> {
                  int args_size = args.size();
                  if (args_size == 1) {
                    auto x = std::dynamic_pointer_cast<object::Number>(args[0]);
                    if (x) {
                      auto val = sin(x->value_);
                      auto number_obj = std::make_shared<object::Number>(val);
                      return number_obj;
                    }
                  }
                  return evaluator::NULLL;
                })},
    {"cos", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object> {
                  int args_size = args.size();
                  if (args_size == 1) {
                    auto x = std::dynamic_pointer_cast<object::Number>(args[0]);
                    if (x) {
                      auto val = cos(x->value_);
                      auto number_obj = std::make_shared<object::Number>(val);
                      return number_obj;
                    }
                  }
                  return evaluator::NULLL;
                })},
    {"abs", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object> {
                  int args_size = args.size();
                  if (args_size == 1) {
                    auto x = std::dynamic_pointer_cast<object::Number>(args[0]);
                    if (x) {
                      auto val = abs(x->value_);
                      auto number_obj = std::make_shared<object::Number>(val);
                      return number_obj;
                    }
                  }
                  return evaluator::NULLL;
                })},
    {"synchz", std::make_shared<object::BuiltIn>(
                   [](std::vector<std::shared_ptr<object::Object>> args)
                       -> std::shared_ptr<object::Object> {
                     int args_size = args.size();
                     if (args_size == 1) {
                       auto x =
                           std::dynamic_pointer_cast<object::Number>(args[0]);
                       if (x) {
                         double val = 0;
                         switch ((int)x->value_) {
                           case 2:
                             val = mixr->GetHzPerTimingUnit(Quantize::Q2);
                             break;
                           case 4:
                             val = mixr->GetHzPerTimingUnit(Quantize::Q4);
                             break;
                           case 8:
                             val = mixr->GetHzPerTimingUnit(Quantize::Q8);
                             break;
                           case 16:
                             val = mixr->GetHzPerTimingUnit(Quantize::Q16);
                             break;
                           case 32:
                             val = mixr->GetHzPerTimingUnit(Quantize::Q32);
                             break;
                         }
                         auto number_obj =
                             std::make_shared<object::Number>(val);
                         return number_obj;
                       }
                     }
                     return evaluator::NULLL;
                   })},
    {"scale",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 5) {
             auto x = std::dynamic_pointer_cast<object::Number>(args[0]);
             auto min_in = std::dynamic_pointer_cast<object::Number>(args[1]);
             auto max_in = std::dynamic_pointer_cast<object::Number>(args[2]);
             auto min_out = std::dynamic_pointer_cast<object::Number>(args[3]);
             auto max_out = std::dynamic_pointer_cast<object::Number>(args[4]);
             if (x && min_in && max_in && min_out && max_out) {
               auto ret_val =
                   scaleybum(min_in->value_, max_in->value_, min_out->value_,
                             max_out->value_, x->value_);
               auto number_obj = std::make_shared<object::Number>(ret_val);

               return number_obj;
             }
           }
           return evaluator::NULLL;
         })},
    {"rand_array",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 3) {
             auto length = std::dynamic_pointer_cast<object::Number>(args[0]);
             auto lower_val =
                 std::dynamic_pointer_cast<object::Number>(args[1]);
             auto upper_val =
                 std::dynamic_pointer_cast<object::Number>(args[2]);
             if (length && lower_val && upper_val) {
               int low = lower_val->value_;
               int hi = upper_val->value_;
               if (low > hi) {
                 int tmp = low;
                 low = hi;
                 hi = tmp;
               }

               auto return_array = std::make_shared<object::Array>(
                   std::vector<std::shared_ptr<object::Object>>());
               for (int i = 0; i < length->value_; i++) {
                 auto rand_number = rand() % (int)(hi - low + 1) + low;
                 auto num_obj = std::make_shared<object::Number>(rand_number);
                 return_array->elements_.push_back(num_obj);
               }
               return return_array;
             }
           }
           return evaluator::NULLL;
         })},
    {"bjork", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
                    int args_size = args.size();
                    if (args_size == 2) {
                      auto num_pulses_num =
                          std::dynamic_pointer_cast<object::Number>(args[0]);
                      auto seq_length_num =
                          std::dynamic_pointer_cast<object::Number>(args[1]);
                      if (num_pulses_num && seq_length_num) {
                        auto return_array = std::make_shared<object::Array>(
                            std::vector<std::shared_ptr<object::Object>>());

                        int num_pulses = num_pulses_num->value_;
                        int seq_length = seq_length_num->value_;

                        // not dealing with error, just return empty
                        if (num_pulses > seq_length) return return_array;
                        if (seq_length < 1) return return_array;

                        std::vector<int> bjork_num =
                            GenerateBjork(num_pulses, seq_length);

                        if (static_cast<int>(bjork_num.size()) < seq_length)
                          return return_array;

                        for (int i = 0; i < seq_length; i++) {
                          return_array->elements_.push_back(
                              std::make_shared<object::Number>(bjork_num[i]));
                        }
                        return return_array;
                      }
                    }
                    return evaluator::NULLL;
                  })},
    {"gen_perc",
     std::make_shared<
         object::BuiltIn>([](std::vector<std::shared_ptr<object::Object>> args)
                              -> std::shared_ptr<object::Object> {
       (void)args;
       std::string cmd =
           "let prc0 = sample(" + GetRandomSampleNameFromDir("perc") + ")";
       eval_command_queue.push(cmd);
       cmd = "pan prc0 " + std::to_string(GetRandomBetweenNegativeOneAndOne());
       eval_command_queue.push(cmd);

       cmd = "let prc1 = sample(" + GetRandomSampleNameFromDir("perc") + ")";
       eval_command_queue.push(cmd);
       cmd = "pan prc1 " + std::to_string(GetRandomBetweenNegativeOneAndOne());
       eval_command_queue.push(cmd);

       cmd = "let prc2 = sample(" + GetRandomSampleNameFromDir("perc") + ")";
       eval_command_queue.push(cmd);
       cmd = "pan prc2 " + std::to_string(GetRandomBetweenNegativeOneAndOne());
       eval_command_queue.push(cmd);

       cmd = "let prc3 = sample(" + GetRandomSampleNameFromDir("perc") + ")";
       eval_command_queue.push(cmd);
       cmd = "pan prc3 " + std::to_string(GetRandomBetweenNegativeOneAndOne());
       eval_command_queue.push(cmd);

       cmd = "let prc4 = sample(" + GetRandomSampleNameFromDir("perc") + ")";
       eval_command_queue.push(cmd);
       cmd = "pan prc4 " + std::to_string(GetRandomBetweenNegativeOneAndOne());
       eval_command_queue.push(cmd);

       cmd = "let prc5 = sample(" + GetRandomSampleNameFromDir("perc") + ")";
       eval_command_queue.push(cmd);
       cmd = "pan prc5 " + std::to_string(GetRandomBetweenNegativeOneAndOne());
       eval_command_queue.push(cmd);

       cmd = "let prc6 = sample(" + GetRandomSampleNameFromDir("perc") + ")";
       eval_command_queue.push(cmd);
       cmd = "pan prc6 " + std::to_string(GetRandomBetweenNegativeOneAndOne());
       eval_command_queue.push(cmd);

       cmd = "let prc7 = sample(" + GetRandomSampleNameFromDir("perc") + ")";
       eval_command_queue.push(cmd);
       cmd = "pan prc7 " + std::to_string(GetRandomBetweenNegativeOneAndOne());
       eval_command_queue.push(cmd);

       cmd = "let percz = [prc0, prc1, prc2, prc3, prc4, prc5, prc6, prc7]";
       eval_command_queue.push(cmd);

       return evaluator::NULLL;
     })},
    {"kit",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           (void)args;
           std::string cmd =
               "let sbdrum = drum(); load_preset(sbdrum,\"FMZ\");";
           eval_command_queue.push(cmd);

           cmd = "let sb2 = drum(); load_preset(sb2,\"FMZ\");";
           eval_command_queue.push(cmd);

           cmd = "let dx = fm(); vol dx 0.8; load_preset(dx,\"mo_jazz\");";
           eval_command_queue.push(cmd);

           cmd = "let dx2 = fm(); vol dx2 0.7; load_preset(dx2, \"MAW2\");";
           eval_command_queue.push(cmd);

           cmd = "let dx3 = fm(); vol dx3 0.7; load_preset(dx3, \"SMMTH2\");";
           eval_command_queue.push(cmd);

           cmd = "let mo = moog();";
           eval_command_queue.push(cmd);

           return evaluator::NULLL;
         })},
    {"load_dir", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> args)
                         -> std::shared_ptr<object::Object> {
                       int args_size = args.size();
                       if (args_size == 1) {
                         std::shared_ptr<object::String> str_obj =
                             std::dynamic_pointer_cast<object::String>(args[0]);
                         if (str_obj) {
                           auto dirname = str_obj->value_;
                           auto fulldirname = "wavs/" + dirname;
                           for (auto &p : fs::directory_iterator(fulldirname)) {
                             auto pathname = p.path().string();
                             pathname.erase(0, 5);

                             std::string base_filename = pathname.substr(
                                 pathname.find_last_of("/\\") + 1);

                             if (ShouldIgnore(base_filename)) continue;

                             std::string::size_type const dot(
                                 base_filename.find_last_of('.'));
                             std::string file_without_extension =
                                 base_filename.substr(0, dot);

                             std::string cmd = "let " + file_without_extension +
                                               " = sample(" + pathname + ")";
                             eval_command_queue.push(cmd);
                           }
                         }
                       }
                       return evaluator::NULLL;
                     })},
    {"eval_pattern",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 1) {
             auto pat_obj = std::dynamic_pointer_cast<object::Pattern>(args[0]);
             if (pat_obj) {
               auto evaluated_pat = pat_obj->Eval();

               std::shared_ptr<object::Array> return_array =
                   ExtractArrayFromPattern(evaluated_pat);

               return return_array;
             }
           }
           return evaluator::NULLL;
         })},
    {"print_pattern",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 1) {
             auto pat = std::dynamic_pointer_cast<object::Pattern>(args[0]);

             if (pat) {
               auto evaluated_pat = pat->Print();
               auto spat = ExtractStringsFromEvalPattern(evaluated_pat);
               PrintPattern(spat);
               auto stepseq = ShrinkPatternToStepSequence(spat);

               auto return_array = std::make_shared<object::Array>(
                   std::vector<std::shared_ptr<object::Object>>());

               for (int i = 0; i < (int)stepseq.size(); ++i) {
                 auto new_string_obj =
                     std::make_shared<object::String>(stepseq[i]);
                 return_array->elements_.push_back(new_string_obj);
               }
               return return_array;
             } else {
               auto pat_array =
                   std::dynamic_pointer_cast<object::Array>(args[0]);
               if (pat_array) {
                 if (pat_array->elements_.size() == PPBAR) {
                   auto intz = ExtractIntsFromObjectArray(pat_array);
                   PrintPattern(intz);
                   auto stepseq = ShrinkPatternToStepSequence(intz);

                   auto return_array = std::make_shared<object::Array>(
                       std::vector<std::shared_ptr<object::Object>>());

                   for (int i = 0; i < (int)stepseq.size(); ++i) {
                     auto new_num_obj =
                         std::make_shared<object::Number>(stepseq[i]);
                     return_array->elements_.push_back(new_num_obj);
                   }
                   return return_array;
                 } else {
                   return pat_array;
                 }
               }
             }
           }
           return evaluator::NULLL;
         })},
    {"notes_in_key",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size > 0) {
             auto num_obj = std::dynamic_pointer_cast<object::Number>(args[0]);
             if (num_obj) {
               auto return_array = std::make_shared<object::Array>(
                   std::vector<std::shared_ptr<object::Object>>());

               int scale_type = 0;  // MAJOR
               if (args_size == 2) {
                 auto scale_type_obj =
                     std::dynamic_pointer_cast<object::Number>(args[1]);

                 if (scale_type_obj) {
                   scale_type = scale_type_obj->value_;
                 }
               }
               std::vector<int> notez = interpreter_sound_cmds::GetNotesInKey(
                   num_obj->value_, scale_type);

               for (unsigned long i = 0; i < notez.size(); i++) {
                 return_array->elements_.push_back(
                     std::make_shared<object::Number>(notez[i]));
               }
               return return_array;
             }
           }
           return evaluator::NULLL;
         })},
    {"notes_in_chord",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size < 2) {
             std::cerr << "Need a root note and key\n";
             return evaluator::NULLL;
           }

           auto root_note = std::dynamic_pointer_cast<object::Number>(args[0]);
           if (!root_note) {
             std::cerr << "NAE ROOT NOTE! numpty!\n";
             return evaluator::NULLL;
           }
           int root_note_value = root_note->value_;

           auto key = std::dynamic_pointer_cast<object::Number>(args[1]);
           if (!key) {
             std::cerr << "NAE KEY! numpty!\n";
             return evaluator::NULLL;
           }
           int key_value = key->value_;

           int chord_modifier = 0;
           if (args_size > 2) {
             auto mod = std::dynamic_pointer_cast<object::Number>(args[2]);
             if (mod) chord_modifier = mod->value_;
           }

           int key_modifier = 0;
           if (args_size > 3) {
             auto mod = std::dynamic_pointer_cast<object::Number>(args[3]);
             if (mod) key_modifier = mod->value_;
           }

           auto return_array = std::make_shared<object::Array>(
               std::vector<std::shared_ptr<object::Object>>());

           std::vector<int> notez = interpreter_sound_cmds::GetNotesInChord(
               root_note_value, key_value, chord_modifier, key_modifier);

           for (int i = 0; i < (int)notez.size(); i++) {
             return_array->elements_.push_back(
                 std::make_shared<object::Number>(notez[i]));
           }
           return return_array;
           // return evaluator::NULLL;
         })},
    {"scale_note",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();

           if (args_size >= 2) {
             auto note_to_tune =
                 std::dynamic_pointer_cast<object::Number>(args[0]);

             if (note_to_tune) {
               auto root_num_obj =
                   std::dynamic_pointer_cast<object::Number>(args[1]);
               if (root_num_obj) {
                 int scale_type = 0;  // MAJOR
                 if (args_size == 3) {
                   auto scale_type_obj =
                       std::dynamic_pointer_cast<object::Number>(args[2]);

                   if (scale_type_obj) {
                     scale_type = scale_type_obj->value_;
                   }
                 }

                 std::vector<int> notez =
                     ScaleMelodyToKey({static_cast<int>(note_to_tune->value_)},
                                      root_num_obj->value_, scale_type);

                 if (notez.size() > 0) {
                   auto return_number =
                       std::make_shared<object::Number>(notez[0]);
                   return return_number;
                 }
               }
             }
           }
           return evaluator::NULLL;
         })},
    {"scale_melody",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();

           if (args_size >= 2) {
             auto array_to_tune =
                 std::dynamic_pointer_cast<object::Array>(args[0]);

             if (array_to_tune) {
               auto root_num_obj =
                   std::dynamic_pointer_cast<object::Number>(args[1]);
               if (root_num_obj) {
                 int scale_type = 0;  // MAJOR
                 if (args_size == 3) {
                   auto scale_type_obj =
                       std::dynamic_pointer_cast<object::Number>(args[2]);

                   if (scale_type_obj) {
                     scale_type = scale_type_obj->value_;
                   }
                 }

                 std::vector<int> orig_notes = {};

                 for (const auto &n : array_to_tune->elements_) {
                   auto numbj = std::dynamic_pointer_cast<object::Number>(n);
                   if (numbj) {
                     orig_notes.push_back(numbj->value_);
                   }
                 }

                 std::vector<int> notez = ScaleMelodyToKey(
                     orig_notes, root_num_obj->value_, scale_type);

                 auto return_tuned_array = std::make_shared<object::Array>(
                     std::vector<std::shared_ptr<object::Object>>());

                 for (unsigned long i = 0; i < notez.size(); i++) {
                   return_tuned_array->elements_.push_back(
                       std::make_shared<object::Number>(notez[i]));
                 }
                 return return_tuned_array;
               }
             }
           }
           return evaluator::NULLL;
         })},
    {"fast",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size < 3) {
             std::cerr << "Need a target, an array to play and a speed..\n";
             return evaluator::NULLL;
           }
           int vel = 128;
           int dur = 240;

           for (auto a : args) {
             if (a->Type() == object::DURATION_OBJ) {
               auto dur_obj = std::dynamic_pointer_cast<object::Duration>(a);
               dur = dur_obj->value_;
             } else if (a->Type() == object::VELOCITY_OBJ) {
               auto vel_obj = std::dynamic_pointer_cast<object::Velocity>(a);
               vel = vel_obj->value_;
             }
           }

           auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (sg) {
             auto speed_obj =
                 std::dynamic_pointer_cast<object::Number>(args[2]);
             if (!speed_obj) {
               std::cerr << "NAE SPEED! numpty!\n";
               return evaluator::NULLL;
             }
             play_array_on(args[0], args[1], speed_obj->value_, dur, vel);
           } else {
             auto speed_obj =
                 std::dynamic_pointer_cast<object::Number>(args[1]);
             if (!speed_obj) {
               std::cerr << "NAE SPEED! numpty!\n";
               return evaluator::NULLL;
             }
             play_array(args[0], speed_obj->value_, dur, vel);
           }

           return evaluator::NULLL;
         })},
    {"play_array",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size < 1) {
             std::cerr << "Need an array to play..\n";
             return evaluator::NULLL;
           }

           int vel = 110 + (rand() % 17);
           int dur = 240;

           for (auto a : args) {
             if (a->Type() == object::DURATION_OBJ) {
               auto dur_obj = std::dynamic_pointer_cast<object::Duration>(a);
               dur = dur_obj->value_;
             } else if (a->Type() == object::VELOCITY_OBJ) {
               auto vel_obj = std::dynamic_pointer_cast<object::Velocity>(a);
               vel = vel_obj->value_;
             }
           }
           auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (sg) {
             if (args_size > 1) {
               play_array_on(args[0], args[1], 1, dur, vel);
             } else {
               std::cerr << "OOFT, NEED AN ARRAY TAE PLAY!\n";
             }
           } else {
             play_array(args[0], 1, dur, vel);
           }

           return evaluator::NULLL;
         })},
    {"play_array_over",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size < 2) {
             std::cerr << "Need an array to play and a speed..\n";
             return evaluator::NULLL;
           }
           int vel = 128;
           int dur = 240;

           for (auto a : args) {
             if (a->Type() == object::DURATION_OBJ) {
               auto dur_obj = std::dynamic_pointer_cast<object::Duration>(a);
               dur = dur_obj->value_;
             } else if (a->Type() == object::VELOCITY_OBJ) {
               auto vel_obj = std::dynamic_pointer_cast<object::Velocity>(a);
               vel = vel_obj->value_;
             }
           }
           auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (sg) {
             auto speed_obj =
                 std::dynamic_pointer_cast<object::Number>(args[2]);
             if (speed_obj)
               play_array_on(args[0], args[1], speed_obj->value_, dur, vel);
           } else {
             auto speed_obj =
                 std::dynamic_pointer_cast<object::Number>(args[1]);
             if (speed_obj) play_array(args[0], speed_obj->value_, dur, vel);
           }

           return evaluator::NULLL;
         })},
    {"play_rhythm",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size < 1) {
             std::cerr << "Need an Rhythm Map to play..\n";
             return evaluator::NULLL;
           }

           int vel = 128;
           int dur = 240;

           for (auto a : args) {
             if (a->Type() == object::DURATION_OBJ) {
               auto dur_obj = std::dynamic_pointer_cast<object::Duration>(a);
               dur = dur_obj->value_;
             } else if (a->Type() == object::VELOCITY_OBJ) {
               auto vel_obj = std::dynamic_pointer_cast<object::Velocity>(a);
               vel = vel_obj->value_;
             }
           }
           auto rhythm_map = std::dynamic_pointer_cast<object::Hash>(args[0]);
           if (rhythm_map) {
             for (const auto &[_, value] : rhythm_map->pairs_) {
               play_array_on(value.key_, value.value_, 1, dur, vel);
             }
           }

           return evaluator::NULLL;
         })},
    {"play_map",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size < 1) {
             std::cerr << "Need an map to play..\n";
             return evaluator::NULLL;
           }

           int vel = 110 + (rand() % 17);
           int dur = 240;

           for (auto a : args) {
             if (a->Type() == object::DURATION_OBJ) {
               auto dur_obj = std::dynamic_pointer_cast<object::Duration>(a);
               dur = dur_obj->value_;
             } else if (a->Type() == object::VELOCITY_OBJ) {
               auto vel_obj = std::dynamic_pointer_cast<object::Velocity>(a);
               vel = vel_obj->value_;
             }
           }
           auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
           if (sg) {
             if (args_size > 1) {
               play_map_on(args[0], args[1], dur, vel);
             } else {
               std::cerr << "OOFT, NEED A MAP TAE PLAY!\n";
             }
           }
           return evaluator::NULLL;
         })},
    {"type", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> args)
                     -> std::shared_ptr<object::Object> {
                   int args_size = args.size();
                   if (args_size == 1)
                     // repl_queue.push("Type:" + args[0]->Type());
                     return std::make_shared<object::String>(args[0]->Type());

                   return evaluator::NULLL;
                 })},
    {"scramble",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 1) {
             auto array_to_scramble =
                 std::dynamic_pointer_cast<object::Array>(args[0]);

             if (array_to_scramble &&
                 array_to_scramble->elements_.size() == 16) {
               auto return_array = std::make_shared<object::Array>(
                   std::vector<std::shared_ptr<object::Object>>());

               std::vector<std::shared_ptr<object::Object>> vals = {};
               for (int i = 0; i < 16; i++) {
                 auto num = std::dynamic_pointer_cast<object::Number>(
                     array_to_scramble->elements_[i]);
                 if (num) {
                   if (num->value_ > 0) {
                     vals.push_back(array_to_scramble->elements_[i]);
                   }
                 }
               }

               for (int i = 0; i < 16; i++) {
                 if (random() % 100 > 70) {
                   return_array->elements_.push_back(
                       vals[random() % vals.size()]);
                 } else {
                   return_array->elements_.push_back(
                       std::make_shared<object::Number>(0));
                 }
               }

               return return_array;
             }
           }
           return evaluator::NULLL;
         })},
    {"stutter",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 1) {
             auto array_to_stutter =
                 std::dynamic_pointer_cast<object::Array>(args[0]);

             if (array_to_stutter) {
               auto return_array = std::make_shared<object::Array>(
                   std::vector<std::shared_ptr<object::Object>>());

               int read_idx = 0;
               if (array_to_stutter->elements_.size() == PPBAR) {
                 for (int i = 0; i < 16; i++) {
                   for (int j = 0; j < PPBAR / 16; ++j) {
                     return_array->elements_.push_back(
                         array_to_stutter->elements_[read_idx + j]);
                   }
                   if (array_to_stutter->elements_[read_idx] == 0 ||
                       random() % 100 > 47)
                     read_idx += PPBAR / 16;
                 }
               } else {
                 for (int i = 0; i < (int)array_to_stutter->elements_.size();
                      i++) {
                   return_array->elements_.push_back(
                       array_to_stutter->elements_[read_idx]);
                   if (array_to_stutter->elements_[read_idx] == 0 ||
                       random() % 100 > 47)
                     ++read_idx;
                 }
               }

               return return_array;
             } else {
               auto pattern_obj =
                   std::dynamic_pointer_cast<object::Pattern>(args[0]);
               if (pattern_obj) {
                 array_to_stutter =
                     ExtractArrayFromPattern(pattern_obj->Eval());

                 auto return_array = std::make_shared<object::Array>(
                     std::vector<std::shared_ptr<object::Object>>());
                 int read_idx = 0;
                 for (int i = 0; i < 16; i++) {
                   for (int j = 0; j < PPBAR / 16; ++j) {
                     return_array->elements_.push_back(
                         array_to_stutter->elements_[read_idx + j]);
                   }
                   if (array_to_stutter->elements_[read_idx] == 0 ||
                       random() % 100 > 47)
                     read_idx += PPBAR / 16;
                 }

                 return return_array;
               }
             }
           }

           return evaluator::NULLL;
         })},
    {"midi_init", std::make_shared<object::BuiltIn>(
                      [](std::vector<std::shared_ptr<object::Object>> args)
                          -> std::shared_ptr<object::Object> {
                        (void)args;
                        MidiInit(mixr);
                        return evaluator::NULLL;
                      })},
    {"midi_assign",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 1) {
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen) {
               mixr->AssignSoundGeneratorToMidiController(
                   soundgen->soundgen_id_);
             }
           }
           return evaluator::NULLL;
         })},
    {"midi_rec", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> args)
                         -> std::shared_ptr<object::Object> {
                       (void)args;
                       mixr->RecordMidiToggle();
                       return evaluator::NULLL;
                     })},
    {"midi_print", std::make_shared<object::BuiltIn>(
                       [](std::vector<std::shared_ptr<object::Object>> args)
                           -> std::shared_ptr<object::Object> {
                         (void)args;
                         mixr->PrintMidiToggle();
                         return evaluator::NULLL;
                       })},
    {"midi_reset", std::make_shared<object::BuiltIn>(
                       [](std::vector<std::shared_ptr<object::Object>> args)
                           -> std::shared_ptr<object::Object> {
                         (void)args;
                         mixr->ResetMidiRecording();
                         return evaluator::NULLL;
                       })},
    {"midi_dump", std::make_shared<object::BuiltIn>(
                      [](std::vector<std::shared_ptr<object::Object>> args)
                          -> std::shared_ptr<object::Object> {
                        (void)args;
                        auto return_pattern =
                            std::make_shared<object::MidiArray>(
                                mixr->RecordingBuffer());
                        return return_pattern;
                      })},
    {"midi2array",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 1) {
             auto midi_array =
                 std::dynamic_pointer_cast<object::MidiArray>(args[0]);
             if (midi_array) {
               auto return_array = std::make_shared<object::Array>(
                   std::vector<std::shared_ptr<object::Object>>());

               for (int i = 0; i < 16; i++) {
                 int lower_midi_index = 240 * i;
                 int higher_midi_index = lower_midi_index + 240;  //

                 bool found = false;
                 for (auto &e : midi_array->notes_on_) {
                   if (e.playback_tick > lower_midi_index &&
                       e.playback_tick < higher_midi_index) {
                     return_array->elements_.push_back(
                         std::make_shared<object::Number>(e.data1));
                     found = true;
                     break;
                   }
                 }

                 if (!found) {
                   return_array->elements_.push_back(
                       std::make_shared<object::Number>(0));
                 }
               }

               return return_array;
             }
           }

           return evaluator::NULLL;
         })},
    {"websock", std::make_shared<object::BuiltIn>(
                    [](std::vector<std::shared_ptr<object::Object>> args)
                        -> std::shared_ptr<object::Object> {
                      std::cout << "Websocket enable yo!\n";
                      int args_size = args.size();
                      if (args_size == 1) {
                        auto val = evaluator::ObjectToNativeBool(args[0]);
                        std::cout << "YO VAL IS:" << val << std::endl;
                        if (val) {
                          SendMixerActionGeneral(AudioAction::ENABLE_WEBSOCKET,
                                                 val);
                        }
                      }
                      return evaluator::NULLL;
                    })},
    {"midi_at",
     std::make_shared<object::BuiltIn>(  // TODO - better name!
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           int args_size = args.size();
           if (args_size == 2) {
             auto midi_array =
                 std::dynamic_pointer_cast<object::MidiArray>(args[0]);
             auto index_at = std::dynamic_pointer_cast<object::Number>(args[1]);
             if (midi_array && index_at) {
               int index_at_val = index_at->value_;
               int lower_midi_index = 240 * index_at_val;
               int higher_midi_index = lower_midi_index + 240;

               for (auto &e : midi_array->notes_on_) {
                 if (e.playback_tick > lower_midi_index &&
                     e.playback_tick < higher_midi_index) {
                   return std::make_shared<object::Number>(e.data1);
                 }
               }
               return std::make_shared<object::Number>(0);
             }
           }
           return evaluator::NULLL;
         })},
    {"midi_fix", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> args)
                         -> std::shared_ptr<object::Object> {
                       int args_size = args.size();
                       if (args_size == 1) {
                         auto midi_array =
                             std::dynamic_pointer_cast<object::MidiArray>(
                                 args[0]);
                         if (midi_array) {
                           for (auto &e : midi_array->notes_on_) {
                             int new_pos_div = e.playback_tick / 120;
                             e.playback_tick = new_pos_div * 120;
                           }
                           for (auto &e : midi_array->control_messages_) {
                             int new_pos_div = e.playback_tick / 120;
                             e.playback_tick = new_pos_div * 120;
                           }
                         }
                       }
                       return evaluator::NULLL;
                     })},
    {"midi_map", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> args)
                         -> std::shared_ptr<object::Object> {
                       int args_size = args.size();
                       if (args_size == 2) {
                         auto id =
                             std::dynamic_pointer_cast<object::Number>(args[0]);
                         auto param =
                             std::dynamic_pointer_cast<object::String>(args[1]);
                         if (id && param) {
                           std::cout << "GOT ID AND PARAM:" << id->value_ << " "
                                     << param->value_ << std::endl;
                           SendMidiMapping(id->value_, param->value_);
                         }
                       } else {
                         std::cout << "DUMP MIXER MAP\n";
                         SendMidiMappingShow();
                       }
                       return evaluator::NULLL;
                     })},
    {"midi2note",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() == 1) {
             auto midi_val = std::dynamic_pointer_cast<object::Number>(args[0]);
             if (midi_val) {
               std::string note_string = GetNoteFromMidiNum(midi_val->value_);
               return std::make_shared<object::String>(note_string);
             }
           }
           return evaluator::NULLL;
         })},
    {"midi2freq",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
           if (args.size() == 1) {
             auto midi_val = std::dynamic_pointer_cast<object::Number>(args[0]);
             if (midi_val) {
               float freq = Midi2Freq(midi_val->value_);
               return std::make_shared<object::Number>(freq);
             }
           }
           return evaluator::NULLL;
         })},
    {"freq2midi", std::make_shared<object::BuiltIn>(
                      [](std::vector<std::shared_ptr<object::Object>> args)
                          -> std::shared_ptr<object::Object> {
                        if (args.size() == 1) {
                          auto freq = std::dynamic_pointer_cast<object::Number>(
                              args[0]);
                          if (freq) {
                            int midi = Freq2Midi(freq->value_);
                            return std::make_shared<object::Number>(midi);
                          }
                        }
                        return evaluator::NULLL;
                      })},
    {"signal_from", std::make_shared<object::BuiltIn>(
                        [](std::vector<std::shared_ptr<object::Object>> args)
                            -> std::shared_ptr<object::Object> {
                          int args_size = args.size();
                          if (args_size == 1) {
                            auto signal_generator =
                                std::dynamic_pointer_cast<object::Phasor>(
                                    args[0]);
                            if (signal_generator) {
                              double sig_val = signal_generator->Generate();
                              return std::make_shared<object::Number>(sig_val);
                            }
                          }
                          return evaluator::NULLL;
                        })},
};

}  // namespace builtin
