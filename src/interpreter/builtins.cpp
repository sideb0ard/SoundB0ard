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

extern mixer *mixr;
extern Tsqueue<audio_action_queue_item> audio_queue;
extern Tsqueue<std::string> eval_command_queue;
extern Tsqueue<std::string> repl_queue;
extern siv::PerlinNoise perlinGenerator;

const std::vector<std::string> FILES_TO_IGNORE = {".DS_Store"};

namespace
{
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

                     auto return_array = std::make_shared<object::Array>(
                         std::vector<std::shared_ptr<object::Object>>());

                     int len_elems = array_obj->elements_.size();
                     for (int i = 0; i < len_elems; i++)
                         return_array->elements_.push_back(
                             array_obj->elements_[i]);

                     return_array->elements_.push_back(input[1]);

                     return return_array;
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
                            mixer_print_timing_info(mixr);
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

             std::shared_ptr<object::Array> array_obj =
                 std::dynamic_pointer_cast<object::Array>(input[0]);
             if (array_obj && number)
             {
                 auto return_array =
                     std::make_shared<object::Array>(array_obj->elements_);

                 std::rotate(return_array->elements_.begin(),
                             return_array->elements_.begin() + number->value_,
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
                         // std::cout << "VAL:" << num_obj->value_ << std::endl;
                         // std::cout << "NOTVAL:" << !num_obj->value_
                         //          << std::endl;
                         auto new_num_obj =
                             std::make_shared<object::Number>(!num_obj->value_);
                         return_array->elements_.push_back(new_num_obj);
                     }
                     else
                     {
                         auto new_num_obj = std::make_shared<object::Number>(0);
                         return_array->elements_.push_back(new_num_obj);
                     }
                 }

                 return return_array;
             }

             return evaluator::NULLL;
         })},
    // TODO - fix for many synth types
    //{"keys",
    // std::make_shared<object::BuiltIn>(
    //     [](std::vector<std::shared_ptr<object::Object>> args)
    //         -> std::shared_ptr<object::Object> {
    //         if (args.size() == 1)
    //         {
    //             auto synth =
    //             std::dynamic_pointer_cast<object::Synth>(args[0]);
    //             if (synth)
    //             {
    //                 if (mixer_is_valid_soundgen_num(mixr,
    //                 synth->soundgen_id_))
    //                 {
    //                     midi_set_destination(mixr,
    //                     synth->soundgen_id_);
    //                 }
    //             }
    //         }
    //         return evaluator::NULLL;
    //     })},
    {"midi_init", std::make_shared<object::BuiltIn>(
                      [](std::vector<std::shared_ptr<object::Object>> args)
                          -> std::shared_ptr<object::Object>
                      {
                          (void)args;
                          midi_launch_init(mixr);
                          return evaluator::NULLL;
                      })},
    {"note_on", std::make_shared<object::BuiltIn>(
                    [](std::vector<std::shared_ptr<object::Object>> args)
                        -> std::shared_ptr<object::Object>
                    {
                        audio_action_queue_item action_req{
                            .type = AudioAction::MIDI_EVENT_ADD, .args = args};
                        audio_queue.push(action_req);
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
    {"note_on_at", std::make_shared<object::BuiltIn>(
                       [](std::vector<std::shared_ptr<object::Object>> args)
                           -> std::shared_ptr<object::Object>
                       {
                           audio_action_queue_item action_req{
                               .type = AudioAction::MIDI_EVENT_ADD_DELAYED,
                               .args = args};
                           audio_queue.push(action_req);
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
    {"compose",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             std::cout << "COMPOSE! with num args:" << args_size << std::endl;
             if (args_size >= 1)
             {
                 std::shared_ptr<object::String> str_obj =
                     std::dynamic_pointer_cast<object::String>(args[0]);
                 if (str_obj)
                 {
                     if (str_obj->value_ == "melody")
                     {
                         auto melody_obj = std::make_shared<object::Array>(
                             std::vector<std::shared_ptr<object::Object>>());
                         auto melody = interpreter_sound_cmds::GenerateMelody();
                         for (size_t i = 0; i < melody.size(); i++)
                         {
                             auto chord_obj = std::make_shared<object::Array>(
                                 std::vector<
                                     std::shared_ptr<object::Object>>());
                             for (size_t j = 0; j < melody[i].size(); j++)
                             {
                                 chord_obj->elements_.push_back(
                                     std::make_shared<object::String>(
                                         melody[i][j]));
                             }
                             melody_obj->elements_.push_back(chord_obj);
                         }
                         return melody_obj;
                     }
                     else if (str_obj->value_ == "bass")
                     {
                         std::cout << "YO GEN A BASS YO\n";
                     }
                 }
                 // auto cmd_name =
                 // std::make_shared<object::String>("save");
                 // args.push_back(cmd_name);
                 // audio_action_queue_item action_req{
                 //    .type = AudioAction::SAVE_PRESET, .args =
                 //    args};
                 // audio_queue.push(action_req);
             }
             return evaluator::NULLL;
         })},
    {"gimme_notes",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             auto melody_obj = std::make_shared<object::Array>(
                 std::vector<std::shared_ptr<object::Object>>());
             auto notes = interpreter_sound_cmds::GetNotesInCurrentChord();
             for (size_t i = 0; i < notes.size(); i++)
             {
                 melody_obj->elements_.push_back(
                     std::make_shared<object::Number>(notes[i]));
             }
             return melody_obj;
         })},
    {"set_key",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             std::cout << "SET KEY! with num args:" << args_size << std::endl;
             if (args_size >= 1)
             {
                 std::shared_ptr<object::String> str_obj =
                     std::dynamic_pointer_cast<object::String>(args[0]);
                 if (str_obj)
                 {
                     mixer_set_key(mixr, str_obj->value_);
                 }
             }
             return evaluator::NULLL;
         })},
    {"set_prog",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object>
         {
             int args_size = args.size();
             std::cout << "SET PROG! with num args:" << args_size << std::endl;
             if (args_size >= 1)
             {
                 auto number =
                     std::dynamic_pointer_cast<object::Number>(args[0]);
                 if (number)
                 {
                     mixer_set_chord_progression(mixr, number->value_);
                 }
             }
             return evaluator::NULLL;
         })},
    {"prog_chord", std::make_shared<object::BuiltIn>(
                       [](std::vector<std::shared_ptr<object::Object>> args)
                           -> std::shared_ptr<object::Object>
                       {
                           mixer_next_chord(mixr);
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
                 auto rand_number = rand() % (int)number->value_;
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
    {"map", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object>
                {
                    int args_size = args.size();
                    if (args_size == 5)
                    {
                        auto x =
                            std::dynamic_pointer_cast<object::Number>(args[0]);
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
                            auto ret_val = scaleybum(
                                min_in->value_, max_in->value_, min_out->value_,
                                max_out->value_, x->value_);
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
    {"notes", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object>
                  {
                      auto return_array = std::make_shared<object::Array>(
                          std::vector<std::shared_ptr<object::Object>>());
                      std::vector<int> notez =
                          interpreter_sound_cmds::GetNotesInCurrentKey();

                      for (int i = 0; i < notez.size(); i++)
                      {

                          return_array->elements_.push_back(
                              std::make_shared<object::Number>(notez[i]));
                      }
                      return return_array;
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

                     std::vector<int> bjork_num =
                         GenerateBjork(num_pulses, seq_length);

                     if (static_cast<int>(bjork_num.size()) < seq_length)
                         return return_array;

                     for (int i = 0; i < seq_length; i++)
                     {
                         // std::shared_ptr<object::Number> num_obj;
                         // if (bjork_num[i] == 1)
                         //    num_obj = std::make_shared<object::Number>(1);
                         // else
                         //    num_obj = std::make_shared<object::Number>(0);

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
};

} // namespace builtin
