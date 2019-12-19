#include <interpreter/builtins.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <interpreter/evaluator.hpp>
#include <keys.h>
#include <midi_cmds.h>
#include <mixer.h>
#include <mixer_cmds.h>
#include <utils.h>

extern mixer *mixr;

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
                        return std::make_shared<object::Integer>(
                            str_obj->value_.size());
                    }

                    std::shared_ptr<object::Array> array_obj =
                        std::dynamic_pointer_cast<object::Array>(input[0]);
                    if (array_obj)
                    {
                        return std::make_shared<object::Integer>(
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
    {"ls", std::make_shared<object::BuiltIn>(
               [](std::vector<std::shared_ptr<object::Object>> args)
                   -> std::shared_ptr<object::Object> {
                   if (args.size() == 1)
                   {
                       std::shared_ptr<object::String> dirname =
                           std::dynamic_pointer_cast<object::String>(args[0]);
                       if (dirname)
                           list_sample_dir(dirname->value_);
                   }
                   else
                   {
                       list_sample_dir("");
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
    {"ps", std::make_shared<object::BuiltIn>(
               [](std::vector<std::shared_ptr<object::Object>> args)
                   -> std::shared_ptr<object::Object> {
                   (void)args;
                   mixer_ps(mixr, false);
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
    {"keys",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
             if (args.size() == 1)
             {
                 auto synth = std::dynamic_pointer_cast<object::Synth>(args[0]);
                 if (synth)
                 {
                     if (mixer_is_valid_soundgen_num(mixr, synth->soundgen_id_))
                     {
                         midi_set_destination(mixr, synth->soundgen_id_);
                     }
                 }
             }
             return evaluator::NULLL;
         })},
    {"midiInit", std::make_shared<object::BuiltIn>(
                     [](std::vector<std::shared_ptr<object::Object>> args)
                         -> std::shared_ptr<object::Object> {
                         (void)args;
                         midi_launch_init(mixr);
                         return evaluator::NULLL;
                     })},
    {"noteOn",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
             int args_size = args.size();
             if (args_size >= 2)
             {
                 auto int_object =
                     std::dynamic_pointer_cast<object::Integer>(args[1]);

                 if (!int_object)
                     return evaluator::NULLL;

                 auto midinum = int_object->value_;

                 int velocity = 127;
                 if (args_size >= 3)
                 {
                     auto int_obj =
                         std::dynamic_pointer_cast<object::Integer>(args[2]);
                     if (!int_obj)
                         return evaluator::NULLL;
                     int passed_velocity = int_obj->value_;
                     if (passed_velocity < 128)
                         velocity = passed_velocity;
                 }
                 midi_event event_on =
                     new_midi_event(MIDI_ON, midinum, velocity);
                 event_on.source = EXTERNAL_OSC;

                 auto synth = std::dynamic_pointer_cast<object::Synth>(args[0]);
                 if (synth)
                 {
                     if (mixer_is_valid_soundgen_num(mixr, synth->soundgen_id_))
                     {
                         SoundGenerator *sg =
                             mixr->SoundGenerators[synth->soundgen_id_];
                         // sg->parseMidiEvent(event_on, mixr->timing_info);

                         int note_duration_ms = sg->note_duration_ms_;
                         if (args_size >= 4)
                         {
                             auto intr_obj =
                                 std::dynamic_pointer_cast<object::Integer>(
                                     args[3]);
                             if (!intr_obj)
                                 return evaluator::NULLL;

                             note_duration_ms = intr_obj->value_;
                         }

                         // call noteOn after ensuring we got duration for
                         // noteOff, otherwise we could have a stuck note.
                         sg->noteOn(event_on);

                         int duration_in_midi_ticks =
                             note_duration_ms /
                             mixr->timing_info.ms_per_midi_tick;
                         int midi_off_tick = (mixr->timing_info.midi_tick +
                                              duration_in_midi_ticks) %
                                             PPBAR;

                         midi_event event_off =
                             new_midi_event(MIDI_OFF, midinum, velocity);
                         event_off.delete_after_use = true;
                         sg->noteOffDelayed(event_off, midi_off_tick);
                     }
                 }
                 // else
                 auto sample =
                     std::dynamic_pointer_cast<object::Sample>(args[0]);
                 if (sample)
                 {
                     if (mixer_is_valid_soundgen_num(mixr,
                                                     sample->soundgen_id_))
                     {
                         SoundGenerator *sg =
                             mixr->SoundGenerators[sample->soundgen_id_];
                         sg->noteOn(event_on);
                     }
                 }
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

             auto synth = std::dynamic_pointer_cast<object::Synth>(args[0]);
             if (synth)
             {

                 if (mixer_is_valid_soundgen_num(mixr, synth->soundgen_id_))
                 {
                     SoundGenerator *sg =
                         mixr->SoundGenerators[synth->soundgen_id_];
                     sg->randomize();
                 }
             }
             return evaluator::NULLL;
         })},
};

} // namespace builtin
