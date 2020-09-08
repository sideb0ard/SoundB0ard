#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "evaluator.hpp"
#include "object.hpp"

namespace interpreter_sound_cmds
{

void ParseFXCmd(std::vector<std::shared_ptr<object::Object>> &args);
void ParseSynthCmd(std::vector<std::shared_ptr<object::Object>> &args);

std::vector<std::vector<std::string>> GenerateMelody();

} // namespace interpreter_sound_cmds
