#include <interpreter/builtins.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <interpreter/evaluator.hpp>
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
                   mixer_ps(mixr, false);
                   return evaluator::NULLL;
               })},
    {"noteOn",
     std::make_shared<object::BuiltIn>(
         [](std::vector<std::shared_ptr<object::Object>> args)
             -> std::shared_ptr<object::Object> {
             if (args.size() == 2)
             {
                 std::cout << "NOTE ON MOFO!\n";
                 auto synth = std::dynamic_pointer_cast<object::Synth>(args[0]);
                 if (synth)
                 {
                     auto midinum =
                         std::dynamic_pointer_cast<object::Integer>(args[1]);
                     std::cout << "JAMMY! Synth Num:" << synth->synth_num_
                               << " MidiNum:" << midinum->value_ << "\n";

                     // create midi packet
                     // cast synth_num to synth
                     // noteOn()
                     midi_event _event =
                         new_midi_event(MIDI_ON, midinum->value_, 128);
                     _event.source = EXTERNAL_OSC;
                     if (mixer_is_valid_soundgen_num(mixr, synth->synth_num_))
                     {
                         SoundGenerator *sg =
                             mixr->SoundGenerators[synth->synth_num_];
                         sg->parseMidiEvent(_event, mixr->timing_info);
                     }
                 }
             }
             return evaluator::NULLL;
         })},
};

} // namespace builtin
