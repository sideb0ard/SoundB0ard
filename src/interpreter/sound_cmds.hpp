#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "drum_synth.h"
#include "evaluator.hpp"
#include "object.hpp"

namespace interpreter_sound_cmds {

std::vector<std::shared_ptr<Fx>> ParseFXCmd(
    std::vector<std::shared_ptr<object::Object>> &args);
void ParseSynthCmd(std::vector<std::shared_ptr<object::Object>> &args);
void SynthLoadPreset(std::shared_ptr<object::Object> &obj,
                     const std::string &preset_name,
                     const std::map<std::string, double> &preset);
std::vector<int> GetNotesInKey(int root, int scale_type = 0);
std::vector<int> GetNotesInChord(int root, int key, int chord_modifier = 0,
                                 int key_modifier = 0);

}  // namespace interpreter_sound_cmds
