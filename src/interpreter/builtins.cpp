#include <interpreter/builtins.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <audio_action_queue.h>
#include <interpreter/evaluator.hpp>
#include <interpreter/sound_cmds.hpp>
#include <keys.h>
#include <midi_cmds.h>
#include <mixer.h>
#include <mixer_cmds.h>
#include <tsqueue.hpp>
#include <utils.h>

extern mixer *mixr;
extern Tsqueue<audio_action_queue_item> g_audio_action_queue;

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
                       g_audio_action_queue.push(action_req);
                       return evaluator::NULLL;
                   })},
    {"speed", std::make_shared<object::BuiltIn>(
                  [](std::vector<std::shared_ptr<object::Object>> args)
                      -> std::shared_ptr<object::Object> {
                      std::cout << "INBUILT SPEED CALLED!" << std::endl;
                      audio_action_queue_item action_req{
                          .type = AudioAction::SPEED, .args = args};
                      g_audio_action_queue.push(action_req);
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
                          g_audio_action_queue.push(action_req);
                      }
                      return evaluator::NULLL;
                  })},
    {"gen", std::make_shared<object::BuiltIn>(
                [](std::vector<std::shared_ptr<object::Object>> args)
                    -> std::shared_ptr<object::Object> {
                    int args_size = args.size();
                    std::cout << "GEN! with num args:" << args_size
                              << std::endl;
                    if (args_size >= 1)
                    {
                        std::shared_ptr<object::String> str_obj =
                            std::dynamic_pointer_cast<object::String>(args[0]);
                        if (str_obj)
                        {
                            if (str_obj->value_ == "melody")
                            {
                                auto melody =
                                    interpreter_sound_cmds::GenerateMelody();
                                return std::make_shared<object::String>(melody);
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
                        // g_audio_action_queue.push(action_req);
                    }
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
                 g_audio_action_queue.push(action_req);
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
                 g_audio_action_queue.push(action_req);
             }
             return evaluator::NULLL;
         })},
    {"rand",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
             if (args.size() != 1)
                 return evaluator::NewError(
                     "`rand` requires a single synth argument.");

             auto soundgen =
                 std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
             if (soundgen)
             {

                 audio_action_queue_item action_req{.mixer_soundgen_idx =
                                                        soundgen->soundgen_id_,
                                                    .type = AudioAction::RAND};
                 g_audio_action_queue.push(action_req);
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

             return evaluator::NULLL;
         })},
};

} // namespace builtin
