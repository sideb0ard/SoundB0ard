#include <interpreter/builtins.hpp>

#include <algorithm>
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
extern Tsqueue<std::string> interpret_command_queue;
extern Tsqueue<std::string> repl_queue;
extern siv::PerlinNoise perlinGenerator;

namespace builtin
{

std::unordered_map<std::string, std::shared_ptr<object::BuiltIn>> built_ins = {
    {"len", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> input)
                    -> std::shared_ptr<object::Object> {
                    if (input.size() != 1)
                        return evaluator::NewError(
                            "Too many arguments for len - can only accept one");

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
    {"head",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
             if (input.size() != 1)
                 return evaluator::NewError(
                     "Too many arguments for len - can only accept one");

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
    {"incr",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
             if (args.size() != 3)
                 return evaluator::NewError(
                     "Too many arguments for incr - need three - number to "
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
    {"tail",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
             if (input.size() != 1)
                 return evaluator::NewError(
                     "Too many arguments for `tail` - can only accept one");

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
                     return_array->elements_.push_back(array_obj->elements_[i]);
                 return return_array;
             }
             return evaluator::NULLL;
         })},
    {"last",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
             if (input.size() != 1)
                 return evaluator::NewError(
                     "Too many arguments for `last` - can only accept one");

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
    {"push",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
             if (input.size() != 2)
                 return evaluator::NewError(
                     "`push` requires two arguments - array and object");

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
                 return_array->elements_.push_back(array_obj->elements_[i]);

             return_array->elements_.push_back(input[1]);

             return return_array;
         })},
    {"puts", std::make_shared<object::BuiltIn>(
                 [](std::vector<std::shared_ptr<object::Object>> args)
                     -> std::shared_ptr<object::Object> {
                     std::stringstream out;
                     for (auto &o : args)
                     {
                         out << o->Inspect();
                     }

                     std::cout << out.str() << std::endl;

                     return evaluator::NULLL;
                 })},
    {"reverse",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> input)
             -> std::shared_ptr<object::Object> {
             if (input.size() != 1)
                 return evaluator::NewError(
                     "`reverse` requires a single array argument.");

             std::shared_ptr<object::Array> array_obj =
                 std::dynamic_pointer_cast<object::Array>(input[0]);
             if (!array_obj)
             {
                 return evaluator::NewError(
                     "argument to `reverse` must be an array - got %s",
                     input[0]->Type());
             }

             auto return_array =
                 std::make_shared<object::Array>(array_obj->elements_);

             std::reverse(return_array->elements_.begin(),
                          return_array->elements_.end());

             return return_array;
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
    // TODO - fix for many synth types
    //{"keys",
    // std::make_shared<object::BuiltIn>(
    //     [](std::vector<std::shared_ptr<object::Object>> args)
    //         -> std::shared_ptr<object::Object> {
    //         if (args.size() == 1)
    //         {
    //             auto synth =
    //             std::dynamic_pointer_cast<object::Synth>(args[0]); if (synth)
    //             {
    //                 if (mixer_is_valid_soundgen_num(mixr,
    //                 synth->soundgen_id_))
    //                 {
    //                     midi_set_destination(mixr, synth->soundgen_id_);
    //                 }
    //             }
    //         }
    //         return evaluator::NULLL;
    //     })},
    {"midiInit", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> args)
                         -> std::shared_ptr<object::Object> {
                         (void)args;
                         midi_launch_init(mixr);
                         return evaluator::NULLL;
                     })},
    {"noteOn", std::make_shared<object::BuiltIn>(
                   [](std::vector<std::shared_ptr<object::Object>> args)
                       -> std::shared_ptr<object::Object> {
                       audio_action_queue_item action_req{
                           .type = AudioAction::NOTE_ON, .args = args};
                       audio_queue.push(action_req);
                       return evaluator::NULLL;
                   })},
    {"speed", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
                      std::cout << "INBUILT SPEED CALLED!" << std::endl;
                      return evaluator::NULLL;
                  })},
    {"addFx", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
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
     std::make_shared<
         object::BuiltIn>([](std::vector<std::shared_ptr<object::Object>> args)
                              -> std::shared_ptr<object::Object> {
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
                             std::vector<std::shared_ptr<object::Object>>());
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
             //    .type = AudioAction::SAVE_PRESET, .args = args};
             // audio_queue.push(action_req);
         }
         return evaluator::NULLL;
     })},
    {"gimmeNotes", std::make_shared<object::BuiltIn>(
                       [](std::vector<std::shared_ptr<object::Object>> args)
                           -> std::shared_ptr<object::Object> {
                           auto melody_obj = std::make_shared<object::Array>(
                               std::vector<std::shared_ptr<object::Object>>());
                           auto notes =
                               interpreter_sound_cmds::GetNotesInCurrentChord();
                           for (size_t i = 0; i < notes.size(); i++)
                           {
                               melody_obj->elements_.push_back(
                                   std::make_shared<object::String>(notes[i]));
                           }
                           return melody_obj;
                       })},
    {"setKey",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
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
    {"setProg",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
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
    {"progChord", std::make_shared<object::BuiltIn>(
                      [](std::vector<std::shared_ptr<object::Object>> args)
                          -> std::shared_ptr<object::Object> {
                          mixer_next_chord(mixr);
                          return evaluator::NULLL;
                      })},
    {"import", std::make_shared<object::BuiltIn>(
                   [](std::vector<std::shared_ptr<object::Object>> args)
                       -> std::shared_ptr<object::Object> {
                       int args_size = args.size();
                       if (args_size == 1)
                       {
                           std::cout << "YOIMPORT\n";
                           std::shared_ptr<object::String> filename =
                               std::dynamic_pointer_cast<object::String>(
                                   args[0]);
                           auto cwd = fs::current_path();
                           std::string filepath =
                               cwd.generic_string() + "/" + filename->value_;

                           mixr_add_file_to_monitor(mixr, filepath);
                           repl_queue.push("Monitoring " + filepath);
                       }
                       else
                           std::cerr << "BARF! ARG SIZE SHOULD BE 1 -  SIZE IS "
                                     << args_size << std::endl;
                       return evaluator::NULLL;
                   })},
    {"loadPreset",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
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
    {"savePreset",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
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
             -> std::shared_ptr<object::Object> {
             if (args.size() != 1)
                 return evaluator::NewError("`rand` requires a single argument "
                                            "- either a synth, array or num.");

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
             -> std::shared_ptr<object::Object> {
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
    {"map", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object> {
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
};

} // namespace builtin
