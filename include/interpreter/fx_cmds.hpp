#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "evaluator.hpp"
#include "object.hpp"

namespace fxcmds
{

void ParseFXCmd(std::vector<std::shared_ptr<object::Object>> &args);

} // namespace fxcmds
