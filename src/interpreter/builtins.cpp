#include <interpreter/builtins.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "PerlinNoise.hpp"

#include <audio_action_queue.h>
#include <filesystem>
#include <interpreter/evaluator.hpp>
#include <interpreter/sound_cmds.hpp>
#include <keys.h>
#include <midi_cmds.h>
#include <mixer.h>
#include <tsqueue.hpp>
#include <utils.h>

namespace fs = std::filesystem;

extern Mixer *mixr;
extern Tsqueue<audio_action_queue_item> audio_queue;
extern Tsqueue<std::string> eval_command_queue;
extern Tsqueue<std::string> repl_queue;
extern siv::PerlinNoise perlinGenerator;

const std::vector<std::string> FILES_TO_IGNORE = {".DS_Store"};

namespace
{

std::shared_ptr<object::Array> ExtractArrayFromPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> evaluated_pat)
{
    auto return_array = std::make_shared<object::Array>(
        std::vector<std::shared_ptr<object::Object>>());

    for (auto ep : evaluated_pat)
    {
        auto midi_tick_array = std::make_shared<object::Array>(
            std::vector<std::shared_ptr<object::Object>>());
        if (!ep.empty())
        {
            for (auto p : ep)
            {
                midi_tick_array->elements_.push_back(
                    std::make_shared<object::String>(p->value_));
            }
        }

        return_array->elements_.push_back(midi_tick_array);
    }

    return return_array;
}
std::array<std::vector<int>, PPBAR> ExtractIntsFromEvalPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> evaluated_pat)
{
    std::array<std::vector<int>, PPBAR> return_array;

    for (int i = 0; i < PPBAR; ++i)
    {
        auto ep = evaluated_pat[i];
        if (!ep.empty())
        {
            for (auto p : ep)
            {
                int p_val = 1;
                try
                {
                    p_val = std::stoi(p->value_);
                }
                catch (...)
                {
                    // expected sometimes.
                }

                return_array[i].push_back(p_val);
            }
        }
    }

    return return_array;
}

std::array<std::vector<std::string>, PPBAR> ExtractStringsFromEvalPattern(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> evaluated_pat)
{
    std::array<std::vector<std::string>, PPBAR> return_array;

    for (int i = 0; i < PPBAR; ++i)
    {
        auto ep = evaluated_pat[i];
        if (!ep.empty())
        {
            for (auto p : ep)
                return_array[i].push_back(p->value_);
        }
    }

    return return_array;
}

bool ShouldIgnore(std::string filename)
{
    auto result =
        std::find(begin(FILES_TO_IGNORE), end(FILES_TO_IGNORE), filename);
    if (result == std::end(FILES_TO_IGNORE))
    {
        return false;
    }
    return true;
}

std::vector<int> GetMidiNotes(std::shared_ptr<object::Object> &numz)
{
    std::vector<int> midi_nums{};
    auto int_object = std::dynamic_pointer_cast<object::Number>(numz);
    if (int_object)
    {
        midi_nums.push_back(int_object->value_);
    }

    auto array_object = std::dynamic_pointer_cast<object::Array>(numz);
    if (array_object)
    {

        for (auto e : array_object->elements_)
        {
            auto int_object = std::dynamic_pointer_cast<object::Number>(e);
            if (int_object)
            {
                midi_nums.push_back(int_object->value_);
            }
        }
    }
    return midi_nums;
}

void chord_on_at(int sgid, std::vector<int> midi_nums, int chord_start_time,
                 int vel, int dur)
{
    audio_action_queue_item action_req{
        .type = AudioAction::MIDI_CHORD_EVENT_ADD_DELAYED,
        .soundgen_num = sgid,
        .notes = midi_nums,
        .velocity = vel,
        .duration = dur,
        .note_start_time = chord_start_time};
    audio_queue.push(action_req);
}

void chord_on(int sgid, std::vector<int> midi_nums, int vel, int dur)
{
    audio_action_queue_item action_req{.type =
                                           AudioAction::MIDI_CHORD_EVENT_ADD,
                                       .soundgen_num = sgid,
                                       .notes = midi_nums,
                                       .velocity = vel,
                                       .duration = dur};
    audio_queue.push(action_req);
}

void note_on_at(int sgid, std::vector<int> midi_nums, int note_start_time,
                int vel, int dur)
{
    audio_action_queue_item action_req{.type =
                                           AudioAction::MIDI_EVENT_ADD_DELAYED,
                                       .soundgen_num = sgid,
                                       .notes = midi_nums,
                                       .velocity = vel,
                                       .duration = dur,
                                       .note_start_time = note_start_time};
    audio_queue.push(action_req);
}

void note_on(int sgid, std::vector<int> midi_nums, int vel, int dur)
{
    audio_action_queue_item action_req{.type = AudioAction::MIDI_EVENT_ADD,
                                       .soundgen_num = sgid,
                                       .notes = midi_nums,
                                       .velocity = vel,
                                       .duration = dur};
    audio_queue.push(action_req);
}

template <typename T>
std::array<T, 16>
ShrinkPatternToStepSequence(std::array<std::vector<T>, PPBAR> &pattern)
{
    std::array<T, 16> shrunk{};
    for (int i = 0; i < 16; ++i)
    {
        for (int j = 0; j < 240; ++j)
        {
            if (pattern[i * 240 + j].size() > 0)
                shrunk[i] = pattern[i * 240 + j][0];
        }
    }
    return shrunk;
}

std::array<std::vector<int>, PPBAR>
ExtractIntsFromObjectArray(std::shared_ptr<object::Array> play_pattern)
{
    std::array<std::vector<int>, PPBAR> return_pattern;
    for (int i = 0; i < PPBAR; ++i)
    {
        auto ppattern = std::dynamic_pointer_cast<object::Array>(
            play_pattern->elements_[i]);
        if (ppattern)
        {
            for (auto item : ppattern->elements_)
            {
                auto sg_string =
                    std::dynamic_pointer_cast<object::String>(item);
                if (sg_string)
                {
                    int sg_val = 1;
                    try
                    {
                        sg_val = std::stoi(sg_string->value_);
                    }
                    catch (...)
                    {
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
                     int offset)
{
    int nlen = play_pattern->elements_.size();
    int pulses_per = midi_len / nlen;

    for (int i = 0; i < nlen; ++i)
    {
        auto item = play_pattern->elements_[i];
        auto val = std::dynamic_pointer_cast<object::Number>(item);
        if (val)
            scaled_pattern[offset + i * pulses_per].push_back(val->value_);

        auto aval = std::dynamic_pointer_cast<object::Array>(item);
        if (aval)
            RecursiveScaler(scaled_pattern, aval, pulses_per, i * pulses_per);
    }
}

template <typename T>
void PrintPattern(std::array<std::vector<T>, PPBAR> &patternn)
{
    for (int i = 0; i < PPBAR; ++i)
    {
        if (patternn[i].size() > 0)
        {
            for (auto item : patternn[i])
                std::cout << "[" << i << "] " << item << std::endl;
        }
    }
}

std::array<std::vector<int>, PPBAR>
ScalePattern(std::shared_ptr<object::Array> play_pattern)
{
    std::array<std::vector<int>, PPBAR> scaled_pattern;
    int midi_len = PPBAR;
    RecursiveScaler(scaled_pattern, play_pattern, midi_len, 0);

    return scaled_pattern;
}

void play_array_on(std::shared_ptr<object::Object> soundgen,
                   std::shared_ptr<object::Object> pattern, float speed,
                   int dur, int vel)
{
    auto sg = std::dynamic_pointer_cast<object::SoundGenerator>(soundgen);
    if (!sg)
        return;

    std::shared_ptr<object::Array> play_pattern;
    auto pat_obj = std::dynamic_pointer_cast<object::Pattern>(pattern);
    if (pat_obj)
        play_pattern = ExtractArrayFromPattern(pat_obj->Eval());
    else
        play_pattern = std::dynamic_pointer_cast<object::Array>(pattern);

    if (play_pattern)
    {
        std::array<std::vector<int>, PPBAR> live_pattern;

        if (play_pattern->elements_.size() >= PPBAR &&
            play_pattern->elements_[0]->Type() == "ARRAY")
            live_pattern = ExtractIntsFromObjectArray(play_pattern);
        else if (play_pattern->elements_.size() > 0 &&
                 play_pattern->elements_[0]->Type() == "NUMBER")
            live_pattern = ScalePattern(play_pattern);

        for (int j = 0; j < speed; ++j)
        {
            float j_offset = (int)live_pattern.size() / speed * j;

            // PrintPattern(live_pattern);
            for (int i = 0; i < (int)live_pattern.size(); ++i)
            {
                float i_offset = i / speed;

                auto ppattern = live_pattern[i];

                if (ppattern.size() > 0)
                {
                    std::vector<int> midi_nums;
                    for (auto item : ppattern)
                    {
                        if (item > 0)
                            midi_nums.push_back(item);
                    }

                    if (midi_nums.size() > 0)
                        note_on_at(sg->soundgen_id_, midi_nums,
                                   j_offset + i_offset, vel, dur);
                }
            }
        }
    }
}

void play_pattern_array(std::shared_ptr<object::Array> play_pattern,
                        float speed, int dur, int vel)
{
    if (play_pattern->elements_.size() != PPBAR)
        return;

    for (int j = 0; j < speed; ++j)
    {
        float j_offset = PPBAR / speed * j;
        for (int i = 0; i < PPBAR; ++i)
        {
            float i_offset = i / speed;
            auto ppattern = std::dynamic_pointer_cast<object::Array>(
                play_pattern->elements_[i]);
            if (ppattern)
            {
                for (auto item : ppattern->elements_)
                {
                    auto sg_string =
                        std::dynamic_pointer_cast<object::String>(item);
                    if (sg_string)
                    {
                        auto sg_name = sg_string->value_;
                        std::stringstream ss;
                        ss << "note_on_at(" << sg_name << ", 1, "
                           << j_offset + i_offset << ", dur=" << dur
                           << ", vel=" << vel << ")";
                        eval_command_queue.push(ss.str());
                    }
                }
            }
        }
    }
}

// Assuming this is a Tidal Notation Pattern
void play_array(std::shared_ptr<object::Object> pattern, float speed, int dur,
                int vel)
{
    std::shared_ptr<object::Array> play_pattern;
    auto pat_obj = std::dynamic_pointer_cast<object::Pattern>(pattern);
    if (pat_obj)
        play_pattern = ExtractArrayFromPattern(pat_obj->Eval());
    else // Already Eval()'d
        play_pattern = std::dynamic_pointer_cast<object::Array>(pattern);
    if (play_pattern)
    {
        if (play_pattern->elements_.size() == PPBAR)
        {
            play_pattern_array(play_pattern, speed, dur, vel);
            return;
        }
    }
}

} // namespace

namespace builtin
{

std::unordered_map<std::string, std::shared_ptr<object::BuiltIn>> built_ins = {
    {"len", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> input)
                    -> std::shared_ptr<object::Object>
                {
                    if (input.size() != 1)
                        return evaluator::NewError(
                            "Too many arguments for len - can only accept "
                            "one");

                    std::shared_ptr<object::String> str_obj =
                        std::dynamic_pointer_cast<object::String>(input[0]);
                    if (str_obj)
                    {
                        return std::make_shared<object::Number>(
                            str_obj->value_.size());
                    }

                    std::shared_ptr<object::Array> array_obj =
                        std::dynamic_pointer_cast<object::Array>(input[0]);
                    if (array_obj)
                    {
                        return std::make_shared<object::Number>(
                            array_obj->elements_.size());
                    }

                    std::shared_ptr<object::Hash> map_obj =
                        std::dynamic_pointer_cast<object::Hash>(input[0]);
                    if (map_obj)
                    {
                        return std::make_shared<object::Number>(
                            map_obj->pairs_.size());
                    }

                    return evaluator::NewError(
                        "argument to `len` not supported, got %s",
                        input[0]->Type());
                })},
    {"head", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object>
                 {
                     if (input.size() != 1)
                         return evaluator::NewError(
                             "Too many arguments for len - can only accept "
                             "one");

                     std::shared_ptr<object::Array> array_obj =
                         std::dynamic_pointer_cast<object::Array>(input[0]);
                     if (!array_obj)
                     {
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
                      -> std::shared_ptr<object::Object>
                  {
                      if (args.size() != 1)
                          return evaluator::NewError("Need WAN arg for floor!");
                      auto number =
                          std::dynamic_pointer_cast<object::Number>(args[0]);
                      if (number)
                      {
                          int floor_num = floor(number->value_);
                          return std::make_shared<object::Number>(floor_num);
                      }
                      return evaluator::NULLL;
                  })},
    {"incr",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             if (args.size() != 3)
                 return evaluator::NewError(
                     "Too many arguments for incr - need three - "
                     "number to "
                     "incr, min and max");
             auto number = std::dynamic_pointer_cast<object::Number>(args[0]);
             auto min = std::dynamic_pointer_cast<object::Number>(args[1]);
             auto max = std::dynamic_pointer_cast<object::Number>(args[2]);
             if (number && min && max)
             {
                 int incr_num = number->value_;
                 incr_num++;
                 if (incr_num >= max->value_)
                 {
                     incr_num = 0 + min->value_;
                 }
                 return std::make_shared<object::Number>(incr_num);
             }
             return evaluator::NULLL;
         })},
    {"tail", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object>
                 {
                     if (input.size() != 1)
                         return evaluator::NewError(
                             "Too many arguments for `tail` - can only "
                             "accept one");

                     std::shared_ptr<object::Array> array_obj =
                         std::dynamic_pointer_cast<object::Array>(input[0]);
                     if (!array_obj)
                     {
                         return evaluator::NewError(
                             "argument to `tail` must be an array - got %s",
                             input[0]->Type());
                     }

                     int len_elems = array_obj->elements_.size();
                     if (len_elems > 0)
                     {
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
                     -> std::shared_ptr<object::Object>
                 {
                     if (input.size() != 1)
                         return evaluator::NewError(
                             "Too many arguments for `last` - can only "
                             "accept one");

                     std::shared_ptr<object::Array> array_obj =
                         std::dynamic_pointer_cast<object::Array>(input[0]);
                     if (!array_obj)
                     {
                         return evaluator::NewError(
                             "argument to `last` must be an array - got %s",
                             input[0]->Type());
                     }

                     int len_elems = array_obj->elements_.size();
                     if (len_elems > 0)
                     {
                         return array_obj->elements_[len_elems - 1];
                     }

                     return evaluator::NULLL;
                 })},
    {"midi_info", std::make_shared<object::BuiltIn>(
                      [](std::vector<std::shared_ptr<object::Object>> args)
                          -> std::shared_ptr<object::Object>
                      {
                          mixr->PrintMidiInfo();
                          return evaluator::NULLL;
                      })},
    {"push", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> input)
                     -> std::shared_ptr<object::Object>
                 {
                     if (input.size() != 2)
                         return evaluator::NewError(
                             "`push` requires two arguments - array and "
                             "object");

                     std::shared_ptr<object::Array> array_obj =
                         std::dynamic_pointer_cast<object::Array>(input[0]);
                     if (!array_obj)
                     {
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
                      -> std::shared_ptr<object::Object>
                  {
                      std::stringstream out;
                      for (auto &o : args)
                      {
                          out << o->Inspect();
                      }

                      std::cout << out.str() << std::endl;

                      return evaluator::NULLL;
                  })},
    {"timing_info", std::make_shared<object::BuiltIn>(
                        [](std::vector<std::shared_ptr<object::Object>> args)
                            -> std::shared_ptr<object::Object>
                        {
                            mixr->PrintTimingInfo();
                            return evaluator::NULLL;
                        })},
    {"reverse",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object>
         {
             if (input.size() != 1)
                 return evaluator::NewError(
                     "`reverse` requires a single array or string "
                     "argument.");

             std::shared_ptr<object::Array> array_obj =
                 std::dynamic_pointer_cast<object::Array>(input[0]);
             if (array_obj)
             {
                 auto return_array =
                     std::make_shared<object::Array>(array_obj->elements_);

                 std::reverse(return_array->elements_.begin(),
                              return_array->elements_.end());
                 return return_array;
             }

             std::shared_ptr<object::String> string_obj =
                 std::dynamic_pointer_cast<object::String>(input[0]);
             if (string_obj)
             {
                 std::string reversed_string = string_obj->value_;
                 reverse(reversed_string.begin(), reversed_string.end());
                 return std::make_shared<object::String>(reversed_string);
             }

             return evaluator::NULLL;
         })},
    {"rotate",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object>
         {
             if (input.size() != 2)
                 return evaluator::NewError(
                     "`rotate` requires two args - an array or "
                     "string plus a num of positions to rotate.");

             auto number = std::dynamic_pointer_cast<object::Number>(input[1]);

             auto pat_obj =
                 std::dynamic_pointer_cast<object::Pattern>(input[0]);
             if (pat_obj && number)
             {
                 auto evaluated_pat = pat_obj->Eval();
                 auto return_array = ExtractArrayFromPattern(evaluated_pat);

                 auto rotate_by =
                     (int)number->value_ % return_array->elements_.size();

                 if (return_array->elements_.size() == 3840 && rotate_by < 16)
                 {
                     rotate_by *= 240;
                 }

                 std::rotate(return_array->elements_.begin(),
                             return_array->elements_.begin() + rotate_by,
                             return_array->elements_.end());

                 return return_array;
             }

             std::shared_ptr<object::Array> array_obj =
                 std::dynamic_pointer_cast<object::Array>(input[0]);
             if (array_obj && number)
             {
                 auto return_array =
                     std::make_shared<object::Array>(array_obj->elements_);

                 auto rotate_by =
                     (int)number->value_ % return_array->elements_.size();

                 if (array_obj->elements_.size() == 3840 && rotate_by < 16)
                 {
                     rotate_by *= 240;
                 }

                 std::rotate(return_array->elements_.begin(),
                             return_array->elements_.begin() + rotate_by,
                             return_array->elements_.end());

                 return return_array;
             }

             std::shared_ptr<object::String> string_obj =
                 std::dynamic_pointer_cast<object::String>(input[0]);
             if (string_obj && number)
             {
                 std::string rotated_string = string_obj->value_;
                 std::rotate(rotated_string.begin(),
                             rotated_string.begin() + number->value_,
                             rotated_string.end());
                 return std::make_shared<object::String>(rotated_string);
             }

             return evaluator::NULLL;
         })},
    {"is_array",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object>
         {
             if (input.size() != 1)
                 return evaluator::NewError(
                     "`is_array` requires a single args - an array");

             std::shared_ptr<object::Array> array_obj =
                 std::dynamic_pointer_cast<object::Array>(input[0]);
             if (array_obj)
             {
                 return evaluator::NativeBoolToBooleanObject(true);
             }
             return evaluator::NativeBoolToBooleanObject(false);
         })},
    {"invert",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object>
         {
             if (input.size() != 1)
                 return evaluator::NewError(
                     "`invert` requires a single args - an array");

             std::shared_ptr<object::Array> array_obj =
                 std::dynamic_pointer_cast<object::Array>(input[0]);
             if (array_obj)
             {

                 auto return_array = std::make_shared<object::Array>(
                     std::vector<std::shared_ptr<object::Object>>());

                 for (auto e : array_obj->elements_)
                 {
                     std::shared_ptr<object::Number> num_obj =
                         std::dynamic_pointer_cast<object::Number>(e);
                     if (num_obj)
                     {
                         auto new_num_obj =
                             std::make_shared<object::Number>(!num_obj->value_);
                         return_array->elements_.push_back(new_num_obj);
                     }
                     else if (e->Type() == "NULL")
                     {
                         auto new_num_obj = std::make_shared<object::Number>(1);
                         return_array->elements_.push_back(new_num_obj);
                     }
                     else if (e->Type() == "ARRAY")
                     {
                         std::shared_ptr<object::Array> inner_array_obj =
                             std::dynamic_pointer_cast<object::Array>(e);
                         if (inner_array_obj)
                         {
                             if (inner_array_obj->elements_.size() > 0)
                             {
                                 auto new_num_obj =
                                     std::make_shared<object::Number>(1);
                                 return_array->elements_.push_back(new_num_obj);
                             }
                             else
                             {
                                 auto new_num_obj =
                                     std::make_shared<object::Number>(0);
                                 return_array->elements_.push_back(new_num_obj);
                             }
                         }
                         else
                         {
                             auto new_num_obj =
                                 std::make_shared<object::Number>(0);
                             return_array->elements_.push_back(new_num_obj);
                         }
                     }
                     else
                     {
                         auto new_num_obj = std::make_shared<object::Number>(0);
                         return_array->elements_.push_back(new_num_obj);
                     }
                 }

                 return return_array;
             }
             auto pattern_obj =
                 std::dynamic_pointer_cast<object::Pattern>(input[0]);
             if (pattern_obj)
             {
                 auto evaluated_pat = pattern_obj->Print();
                 auto intz = ExtractIntsFromEvalPattern(evaluated_pat);
                 auto stepseq = ShrinkPatternToStepSequence(intz);

                 auto return_array = std::make_shared<object::Array>(
                     std::vector<std::shared_ptr<object::Object>>());

                 for (int i = 0; i < (int)stepseq.size(); ++i)
                 {
                     auto new_num_obj =
                         std::make_shared<object::Number>(!stepseq[i]);
                     return_array->elements_.push_back(new_num_obj);
                 }
                 return return_array;
             }

             return evaluator::NULLL;
         })},
    {"stop",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             if (args.size() != 1)
                 return evaluator::NewError(
                     "`stop` requires a sound_generator target");

             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen)
             {
                 audio_action_queue_item action_req{.type = AudioAction::STOP,
                                                    .soundgen_num =
                                                        soundgen->soundgen_id_};
                 audio_queue.push(action_req);
             }
             return evaluator::NULLL;
         })},
    {"chord_on",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             if (args.size() < 2)
                 return evaluator::NewError("`chord_on` requires at least two "
                                            "args - a sound_generator target "
                                            "and a midi_note to play.");
             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen)
             {
                 int sgid = soundgen->soundgen_id_;
                 std::vector<int> object_val = GetMidiNotes(args[1]);

                 if (object_val.empty())
                     return evaluator::NULLL;

                 std::vector<int> notez =
                     GetMidiNotesInChord(object_val[0], 0, true);

                 int vel = 128;
                 int dur = 240;

                 for (auto a : args)
                 {
                     if (a->Type() == object::DURATION_OBJ)
                     {
                         auto dur_obj =
                             std::dynamic_pointer_cast<object::Duration>(a);
                         dur = dur_obj->value_;
                     }
                     else if (a->Type() == object::VELOCITY_OBJ)
                     {
                         auto vel_obj =
                             std::dynamic_pointer_cast<object::Velocity>(a);
                         vel = vel_obj->value_;
                     }
                 }

                 chord_on(sgid, notez, vel, dur);
             }
             return evaluator::NULLL;
         })},
    {"chord_on_at",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             if (args.size() < 3)
                 return evaluator::NewError(
                     "`chord_on_at` requires at least three args - a "
                     "sound_generator target, a midi_note to play and a time "
                     "in the future specified in midi ticks.");

             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen)
             {
                 int sgid = soundgen->soundgen_id_;

                 std::vector<int> object_val = GetMidiNotes(args[1]);

                 if (object_val.empty())
                     return evaluator::NULLL;

                 std::vector<int> notez =
                     GetMidiNotesInChord(object_val[0], 0, true);

                 auto num_obj =
                     std::dynamic_pointer_cast<object::Number>(args[2]);

                 if (!num_obj)
                     return evaluator::NULLL;

                 int vel = 128;
                 int dur = 240;
                 int note_start_time = num_obj->value_;

                 for (auto a : args)
                 {
                     if (a->Type() == object::DURATION_OBJ)
                     {
                         auto dur_obj =
                             std::dynamic_pointer_cast<object::Duration>(a);
                         dur = dur_obj->value_;
                     }
                     else if (a->Type() == object::VELOCITY_OBJ)
                     {
                         auto vel_obj =
                             std::dynamic_pointer_cast<object::Velocity>(a);
                         vel = vel_obj->value_;
                     }
                 }

                 chord_on_at(sgid, notez, note_start_time, vel, dur);
             }

             return evaluator::NULLL;
         })},
    {"note_on",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             if (args.size() < 2)
                 return evaluator::NewError("`note_on` requires at least two "
                                            "args - a sound_generator target "
                                            "and a midi_note to play.");

             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen)
             {
                 int sgid = soundgen->soundgen_id_;
                 std::vector<int> midi_nums = GetMidiNotes(args[1]);
                 int vel = 128;
                 int dur = 240;

                 for (auto a : args)
                 {
                     if (a->Type() == object::DURATION_OBJ)
                     {
                         auto dur_obj =
                             std::dynamic_pointer_cast<object::Duration>(a);
                         dur = dur_obj->value_;
                     }
                     else if (a->Type() == object::VELOCITY_OBJ)
                     {
                         auto vel_obj =
                             std::dynamic_pointer_cast<object::Velocity>(a);
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
             -> std::shared_ptr<object::Object>
         {
             if (args.size() < 3)
                 return evaluator::NewError(
                     "`note_on_at` requires at least three args - a "
                     "sound_generator target, a midi_note to play and a time "
                     "in the future specified in midi ticks.");

             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen)
             {
                 int sgid = soundgen->soundgen_id_;
                 std::vector<int> midi_nums = GetMidiNotes(args[1]);

                 auto num_obj =
                     std::dynamic_pointer_cast<object::Number>(args[2]);

                 if (!num_obj)
                     return evaluator::NULLL;

                 int vel = 128;
                 int dur = 240;
                 int note_start_time = num_obj->value_;

                 for (auto a : args)
                 {
                     if (a->Type() == object::DURATION_OBJ)
                     {
                         auto dur_obj =
                             std::dynamic_pointer_cast<object::Duration>(a);
                         dur = dur_obj->value_;
                     }
                     else if (a->Type() == object::VELOCITY_OBJ)
                     {
                         auto vel_obj =
                             std::dynamic_pointer_cast<object::Velocity>(a);
                         vel = vel_obj->value_;
                     }
                 }

                 note_on_at(sgid, midi_nums, note_start_time, vel, dur);
             }

             return evaluator::NULLL;
         })},
    {"set_pitch",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size >= 2)
             {
                 auto soundgen =
                     std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
                 if (soundgen)
                 {
                     auto number =
                         std::dynamic_pointer_cast<object::Number>(args[1]);
                     if (number)
                     {
                         int delayed_by = 0;
                         if (args_size == 3)
                         {
                             auto delayed_time =
                                 std::dynamic_pointer_cast<object::Number>(
                                     args[2]);
                             if (delayed_time)
                                 delayed_by = delayed_time->value_;
                         }
                         audio_action_queue_item action{
                             .type = AudioAction::UPDATE,
                             .mixer_soundgen_idx = soundgen->soundgen_id_,
                             .delayed_by = delayed_by,
                             .fx_id = -1,
                             .param_name = "pitch",
                             .param_val = std::to_string(number->value_)};
                         audio_queue.push(action);
                     }
                 }
             }
             return evaluator::NULLL;
         })},
    {"speed", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object>
                  {
                      std::cout << "INBUILT SPEED CALLED!" << std::endl;
                      return evaluator::NULLL;
                  })},
    {"add_fx", std::make_shared<object::BuiltIn>(
                   [](std::vector<std::shared_ptr<object::Object>> args)
                       -> std::shared_ptr<object::Object>
                   {
                       int args_size = args.size();
                       if (args_size >= 2)
                       {
                           audio_action_queue_item action_req{
                               .type = AudioAction::ADD_FX, .args = args};
                           audio_queue.push(action_req);
                       }
                       return evaluator::NULLL;
                   })},
    {"monitor",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 std::shared_ptr<object::String> filename =
                     std::dynamic_pointer_cast<object::String>(args[0]);
                 auto cwd = fs::current_path();
                 std::string filepath =
                     cwd.generic_string() + "/" + filename->value_;

                 // mixr_add_file_to_monitor(mixr, filepath);
                 audio_action_queue_item action_req{
                     .type = AudioAction::MONITOR, .filepath = filepath};
                 audio_queue.push(action_req);
                 repl_queue.push("Monitoring " + filepath);
             }
             else
                 std::cerr << "BARF! ARG SIZE SHOULD BE 1 -  SIZE IS "
                           << args_size << std::endl;
             return evaluator::NULLL;
         })},
    {"list_presets",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 auto cmd_name = std::make_shared<object::String>("list");
                 args.push_back(cmd_name);
                 audio_action_queue_item action_req{
                     .type = AudioAction::LIST_PRESETS, .args = args};
                 audio_queue.push(action_req);
             }
             return evaluator::NULLL;
         })},
    {"load_preset",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size >= 2)
             {
                 auto cmd_name = std::make_shared<object::String>("load");
                 args.push_back(cmd_name);
                 audio_action_queue_item action_req{
                     .type = AudioAction::LOAD_PRESET, .args = args};
                 audio_queue.push(action_req);
             }
             return evaluator::NULLL;
         })},
    {"save_preset",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size >= 2)
             {
                 auto cmd_name = std::make_shared<object::String>("save");
                 args.push_back(cmd_name);
                 audio_action_queue_item action_req{
                     .type = AudioAction::SAVE_PRESET, .args = args};
                 audio_queue.push(action_req);
             }
             return evaluator::NULLL;
         })},
    {"rand",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             if (args.size() == 0)
             {
                 auto rand_number =
                     static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                 return std::make_shared<object::Number>(rand_number);
             }

             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen)
             {

                 audio_action_queue_item action_req{.mixer_soundgen_idx =
                                                        soundgen->soundgen_id_,
                                                    .type = AudioAction::RAND};
                 audio_queue.push(action_req);
                 return evaluator::NULLL;
             }

             auto array_obj = std::dynamic_pointer_cast<object::Array>(args[0]);
             if (array_obj)
             {

                 int len_elems = array_obj->elements_.size();
                 if (len_elems > 0)
                 {
                     int idx = rand() % len_elems;
                     return array_obj->elements_[idx];
                 }
             }

             auto number = std::dynamic_pointer_cast<object::Number>(args[0]);
             if (number)
             {
                 int rand_number = 0;
                 if (number->value_ > 0)
                     rand_number = rand() % (int)number->value_;
                 return std::make_shared<object::Number>(rand_number);
             }

             return evaluator::NULLL;
         })},
    {"perlin",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 auto x = std::dynamic_pointer_cast<object::Number>(args[0]);
                 if (x)
                 {
                     auto val = perlinGenerator.noise1D(float(x->value_));
                     auto number_obj = std::make_shared<object::Number>(val);
                     return number_obj;
                 }
             }
             return evaluator::NULLL;
         })},
    {"sin", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object>
                {
                    int args_size = args.size();
                    if (args_size == 1)
                    {
                        auto x =
                            std::dynamic_pointer_cast<object::Number>(args[0]);
                        if (x)
                        {
                            auto val = sin(x->value_);
                            auto number_obj =
                                std::make_shared<object::Number>(val);
                            return number_obj;
                        }
                    }
                    return evaluator::NULLL;
                })},
    {"cos", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object>
                {
                    int args_size = args.size();
                    if (args_size == 1)
                    {
                        auto x =
                            std::dynamic_pointer_cast<object::Number>(args[0]);
                        if (x)
                        {
                            auto val = cos(x->value_);
                            auto number_obj =
                                std::make_shared<object::Number>(val);
                            return number_obj;
                        }
                    }
                    return evaluator::NULLL;
                })},
    {"abs", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object>
                {
                    int args_size = args.size();
                    if (args_size == 1)
                    {
                        auto x =
                            std::dynamic_pointer_cast<object::Number>(args[0]);
                        if (x)
                        {
                            auto val = abs(x->value_);
                            auto number_obj =
                                std::make_shared<object::Number>(val);
                            return number_obj;
                        }
                    }
                    return evaluator::NULLL;
                })},
    {"synchz",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 auto x = std::dynamic_pointer_cast<object::Number>(args[0]);
                 if (x)
                 {
                     double val = 0;
                     switch ((int)x->value_)
                     {
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
                     auto number_obj = std::make_shared<object::Number>(val);
                     return number_obj;
                 }
             }
             return evaluator::NULLL;
         })},
    {"scale",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 5)
             {
                 auto x = std::dynamic_pointer_cast<object::Number>(args[0]);
                 auto min_in =
                     std::dynamic_pointer_cast<object::Number>(args[1]);
                 auto max_in =
                     std::dynamic_pointer_cast<object::Number>(args[2]);
                 auto min_out =
                     std::dynamic_pointer_cast<object::Number>(args[3]);
                 auto max_out =
                     std::dynamic_pointer_cast<object::Number>(args[4]);
                 if (x && min_in && max_in && min_out && max_out)
                 {
                     auto ret_val =
                         scaleybum(min_in->value_, max_in->value_,
                                   min_out->value_, max_out->value_, x->value_);
                     auto number_obj =
                         std::make_shared<object::Number>(ret_val);

                     return number_obj;
                 }
             }
             return evaluator::NULLL;
         })},
    {"rand_array",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 3)
             {
                 auto length =
                     std::dynamic_pointer_cast<object::Number>(args[0]);
                 auto lower_val =
                     std::dynamic_pointer_cast<object::Number>(args[1]);
                 auto upper_val =
                     std::dynamic_pointer_cast<object::Number>(args[2]);
                 if (length && lower_val && upper_val)
                 {
                     // not dealing with error, just return
                     if (lower_val->value_ >= upper_val->value_)
                         return evaluator::NULLL;

                     auto return_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());
                     for (int i = 0; i < length->value_; i++)
                     {
                         auto rand_number =
                             rand() % (int)(upper_val->value_ -
                                            lower_val->value_ + 1) +
                             lower_val->value_;
                         auto num_obj =
                             std::make_shared<object::Number>(rand_number);
                         return_array->elements_.push_back(num_obj);
                     }
                     return return_array;
                 }
             }
             return evaluator::NULLL;
         })},
    {"bjork",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 2)
             {
                 auto num_pulses_num =
                     std::dynamic_pointer_cast<object::Number>(args[0]);
                 auto seq_length_num =
                     std::dynamic_pointer_cast<object::Number>(args[1]);
                 if (num_pulses_num && seq_length_num)
                 {
                     auto return_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());

                     int num_pulses = num_pulses_num->value_;
                     int seq_length = seq_length_num->value_;

                     // not dealing with error, just return empty
                     if (num_pulses >= seq_length)
                         return return_array;
                     if (seq_length < 1)
                         return return_array;

                     std::vector<int> bjork_num =
                         GenerateBjork(num_pulses, seq_length);

                     if (static_cast<int>(bjork_num.size()) < seq_length)
                         return return_array;

                     for (int i = 0; i < seq_length; i++)
                     {
                         return_array->elements_.push_back(
                             std::make_shared<object::Number>(bjork_num[i]));
                     }
                     return return_array;
                 }
             }
             return evaluator::NULLL;
         })},
    {"load_dir",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 std::shared_ptr<object::String> str_obj =
                     std::dynamic_pointer_cast<object::String>(args[0]);
                 if (str_obj)
                 {
                     auto dirname = str_obj->value_;
                     auto fulldirname = "wavs/" + dirname;
                     for (auto &p : fs::directory_iterator(fulldirname))
                     {
                         auto pathname = p.path().string();
                         pathname.erase(0, 5);

                         std::string base_filename =
                             pathname.substr(pathname.find_last_of("/\\") + 1);

                         if (ShouldIgnore(base_filename))
                             continue;

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
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 auto pat_obj =
                     std::dynamic_pointer_cast<object::Pattern>(args[0]);
                 if (pat_obj)
                 {
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
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 auto pat = std::dynamic_pointer_cast<object::Pattern>(args[0]);

                 if (pat)
                 {
                     auto evaluated_pat = pat->Print();
                     auto spat = ExtractStringsFromEvalPattern(evaluated_pat);
                     PrintPattern(spat);
                     auto stepseq = ShrinkPatternToStepSequence(spat);

                     auto return_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());

                     for (int i = 0; i < (int)stepseq.size(); ++i)
                     {
                         auto new_string_obj =
                             std::make_shared<object::String>(stepseq[i]);
                         return_array->elements_.push_back(new_string_obj);
                     }
                     return return_array;
                 }
                 else
                 {
                     auto pat_array =
                         std::dynamic_pointer_cast<object::Array>(args[0]);
                     if (pat_array)
                     {
                         if (pat_array->elements_.size() == PPBAR)
                         {
                             auto intz = ExtractIntsFromObjectArray(pat_array);
                             PrintPattern(intz);
                             auto stepseq = ShrinkPatternToStepSequence(intz);

                             auto return_array =
                                 std::make_shared<object::Array>(
                                     std::vector<
                                         std::shared_ptr<object::Object>>());

                             for (int i = 0; i < (int)stepseq.size(); ++i)
                             {
                                 auto new_num_obj =
                                     std::make_shared<object::Number>(
                                         stepseq[i]);
                                 return_array->elements_.push_back(new_num_obj);
                             }
                             return return_array;
                         }
                         else
                         {
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
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 auto num_obj =
                     std::dynamic_pointer_cast<object::Number>(args[0]);
                 if (num_obj)
                 {
                     auto return_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());

                     std::vector<int> notez =
                         interpreter_sound_cmds::GetNotesInKey(num_obj->value_);

                     for (unsigned long i = 0; i < notez.size(); i++)
                     {

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
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size >= 2)
             {
                 float root_note_value = 0;
                 auto root_note =
                     std::dynamic_pointer_cast<object::Number>(args[0]);
                 if (!root_note)
                 {
                     auto root_note_string =
                         std::dynamic_pointer_cast<object::String>(args[0]);
                     if (!root_note_string)
                     {
                         std::cerr << "NAE ROOT NOTE! numpty!\n";
                         return evaluator::NULLL;
                     }
                     root_note_value = std::stoi(root_note_string->value_);
                 }
                 else
                 {
                     root_note_value = root_note->value_;
                 }

                 // can be MAJOR (0), MINOR (1), DIMINISHED (2)
                 auto chord_type =
                     std::dynamic_pointer_cast<object::Number>(args[1]);
                 if (!chord_type)
                 {
                     std::cerr << "NAE CHORD TYPE! numpty!\n";
                     return evaluator::NULLL;
                 }

                 auto return_array = std::make_shared<object::Array>(
                     std::vector<std::shared_ptr<object::Object>>());

                 bool seventh = false;
                 if (args_size == 3)
                     seventh = true;

                 std::vector<int> notez = GetMidiNotesInChord(
                     root_note_value, chord_type->value_, seventh);

                 for (int i = 0; i < (int)notez.size(); i++)
                 {

                     return_array->elements_.push_back(
                         std::make_shared<object::Number>(notez[i]));
                 }
                 return return_array;
             }
             return evaluator::NULLL;
         })},
    {"fast",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size < 3)
             {
                 std::cerr << "Need a target, an array to play and a speed..\n";
                 return evaluator::NULLL;
             }
             int vel = 128;
             int dur = 240;

             for (auto a : args)
             {
                 if (a->Type() == object::DURATION_OBJ)
                 {
                     auto dur_obj =
                         std::dynamic_pointer_cast<object::Duration>(a);
                     dur = dur_obj->value_;
                 }
                 else if (a->Type() == object::VELOCITY_OBJ)
                 {
                     auto vel_obj =
                         std::dynamic_pointer_cast<object::Velocity>(a);
                     vel = vel_obj->value_;
                 }
             }

             auto sg =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (sg)
             {
                 auto speed_obj =
                     std::dynamic_pointer_cast<object::Number>(args[2]);
                 if (!speed_obj)
                 {
                     std::cerr << "NAE SPEED! numpty!\n";
                     return evaluator::NULLL;
                 }
                 play_array_on(args[0], args[1], speed_obj->value_, dur, vel);
             }
             else
             {
                 auto speed_obj =
                     std::dynamic_pointer_cast<object::Number>(args[1]);
                 if (!speed_obj)
                 {
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
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size < 1)
             {
                 std::cerr << "Need an array to play..\n";
                 return evaluator::NULLL;
             }

             int vel = 128;
             int dur = 240;

             for (auto a : args)
             {
                 if (a->Type() == object::DURATION_OBJ)
                 {
                     auto dur_obj =
                         std::dynamic_pointer_cast<object::Duration>(a);
                     dur = dur_obj->value_;
                 }
                 else if (a->Type() == object::VELOCITY_OBJ)
                 {
                     auto vel_obj =
                         std::dynamic_pointer_cast<object::Velocity>(a);
                     vel = vel_obj->value_;
                 }
             }
             auto sg =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (sg)
                 play_array_on(args[0], args[1], 1, dur, vel);
             else
                 play_array(args[0], 1, dur, vel);

             return evaluator::NULLL;
         })},
    {"play_array_over",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size < 2)
             {
                 std::cerr << "Need an array to play and a speed..\n";
                 return evaluator::NULLL;
             }
             int vel = 128;
             int dur = 240;

             for (auto a : args)
             {
                 if (a->Type() == object::DURATION_OBJ)
                 {
                     auto dur_obj =
                         std::dynamic_pointer_cast<object::Duration>(a);
                     dur = dur_obj->value_;
                 }
                 else if (a->Type() == object::VELOCITY_OBJ)
                 {
                     auto vel_obj =
                         std::dynamic_pointer_cast<object::Velocity>(a);
                     vel = vel_obj->value_;
                 }
             }
             auto sg =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (sg)
             {
                 auto speed_obj =
                     std::dynamic_pointer_cast<object::Number>(args[2]);
                 if (speed_obj)
                     play_array_on(args[0], args[1], speed_obj->value_, dur,
                                   vel);
             }
             else
             {
                 auto speed_obj =
                     std::dynamic_pointer_cast<object::Number>(args[1]);
                 if (speed_obj)
                     play_array(args[0], speed_obj->value_, dur, vel);
             }

             return evaluator::NULLL;
         })},
    {"type", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> args)
                     -> std::shared_ptr<object::Object>
                 {
                     int args_size = args.size();
                     if (args_size == 1)
                         // repl_queue.push("Type:" + args[0]->Type());
                         return std::make_shared<object::String>(
                             args[0]->Type());

                     return evaluator::NULLL;
                 })},
    {"stutter",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             if (args_size == 1)
             {
                 auto array_to_stutter =
                     std::dynamic_pointer_cast<object::Array>(args[0]);

                 if (array_to_stutter)
                 {
                     auto return_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());

                     int read_idx = 0;
                     if (array_to_stutter->elements_.size() == PPBAR)
                     {
                         for (int i = 0; i < 16; i++)
                         {
                             for (int j = 0; j < PPBAR / 16; ++j)
                             {
                                 return_array->elements_.push_back(
                                     array_to_stutter->elements_[read_idx + j]);
                             }
                             if (array_to_stutter->elements_[read_idx] == 0 ||
                                 random() % 100 > 47)
                                 read_idx += PPBAR / 16;
                         }
                     }
                     else
                     {
                         for (int i = 0;
                              i < (int)array_to_stutter->elements_.size(); i++)
                         {
                             return_array->elements_.push_back(
                                 array_to_stutter->elements_[read_idx]);
                             if (array_to_stutter->elements_[read_idx] == 0 ||
                                 random() % 100 > 47)
                                 ++read_idx;
                         }
                     }

                     return return_array;
                 }
                 else
                 {
                     auto pattern_obj =
                         std::dynamic_pointer_cast<object::Pattern>(args[0]);
                     if (pattern_obj)
                     {
                         array_to_stutter =
                             ExtractArrayFromPattern(pattern_obj->Eval());

                         auto return_array = std::make_shared<object::Array>(
                             std::vector<std::shared_ptr<object::Object>>());
                         int read_idx = 0;
                         for (int i = 0; i < 16; i++)
                         {
                             for (int j = 0; j < PPBAR / 16; ++j)
                             {
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
};

} // namespace builtin
