#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "evaluator.hpp"
#include "object.hpp"

namespace builtin
{

extern std::unordered_map<std::string, std::shared_ptr<object::BuiltIn>>
    built_ins;

} // namespace builtin
