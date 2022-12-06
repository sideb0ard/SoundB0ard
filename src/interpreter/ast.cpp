#include <interpreter/ast.hpp>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

extern char *s_event_type[];

namespace ast {

std::string Program::TokenLiteral() const {
  if (!statements_.empty())
    return statements_[0]->TokenLiteral();
  else
    return "";
}

std::string Program::String() const {
  std::stringstream ss;

  for (auto s : statements_) ss << s->String();

  return ss.str();
}

std::string BreakStatement::String() const {
  std::stringstream ss;

  return "BREAK";
}

std::string VelocityExpression::String() const {
  std::stringstream ss;

  if (velocity_val) ss << velocity_val->String();
  ss << ";";

  return ss.str();
}

std::string AtExpression::String() const {
  std::stringstream ss;

  if (midi_ticks_from_now) ss << midi_ticks_from_now->String();
  ss << ";";

  return ss.str();
}

std::string DurationExpression::String() const {
  std::stringstream ss;

  if (duration_val) ss << duration_val->String();
  ss << ";";

  return ss.str();
}

std::string LetStatement::String() const {
  std::stringstream ss;
  if (is_new_item) ss << TokenLiteral() << " ";
  if (name_) ss << name_->String() << " = ";
  if (value_) ss << value_->String();
  ss << ";";

  return ss.str();
}

std::string InfoStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << soundgen_identifier_->String();
  ss << ";";

  return ss.str();
}

std::string BpmStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << bpm_val_->String();
  ss << ";";

  return ss.str();
}

std::string LsStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << ";";

  return ss.str();
}
std::string StrategyStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << ";";

  return ss.str();
}

std::string PlayStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << ";";

  return ss.str();
}

std::string PsStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << ";";

  return ss.str();
}

std::string HelpStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << ";";

  return ss.str();
}

std::string SetStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << target_ << ":" << param_ << " - " << value_;

  return ss.str();
}

std::string VolumeStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << ";";

  return ss.str();
}

std::string PanStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  ss << ";";

  return ss.str();
}

std::string ForStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral() << "(";
  if (iterator_) ss << iterator_->String() << " = ";
  if (iterator_value_) ss << iterator_value_->String();
  ss << ";";
  if (termination_condition_) ss << termination_condition_->String();
  ss << ";";
  if (increment_) ss << increment_->String();
  ss << ")";
  if (body_) ss << body_->String();

  return ss.str();
}

std::string ReturnStatement::String() const {
  std::stringstream ss;
  ss << TokenLiteral();
  if (return_value_) ss << " " << return_value_->String();
  ss << ";";

  return ss.str();
}

std::string ExpressionStatement::String() const {
  std::stringstream ss;
  if (expression_) {
    ss << expression_->String();
  }

  return ss.str();
}

std::string PrefixExpression::String() const {
  std::stringstream ss;
  ss << "(";
  if (!operator_.empty()) ss << operator_;
  if (right_) ss << right_->String();
  ss << ")";

  return ss.str();
}

std::string BlockStatement::String() const {
  std::stringstream ss;
  for (auto &s : statements_) ss << s->String() << '\n';

  return ss.str();
}

std::string IfExpression::String() const {
  std::stringstream ss;
  ss << "if ";
  if (condition_) ss << condition_->String();
  ss << " ";
  if (consequence_) ss << consequence_->String();
  if (alternative_) {
    ss << " else ";
    ss << alternative_->String();
  }

  return ss.str();
}

std::string EveryExpression::String() const {
  std::stringstream ss;
  ss << "every " << frequency_ << " " << (int)event_type_ << body_->String();

  return ss.str();
}

std::string PhasorLiteral::String() const {
  std::stringstream ss;

  ss << frequency_;
  return ss.str();
}

std::string FunctionLiteral::String() const {
  std::stringstream ss;

  std::vector<std::string> params;
  for (auto p : parameters_) params.push_back(p->String());

  ss << TokenLiteral() << "("
     << std::accumulate(params.begin(), params.end(), std::string(),
                        [](const std::string &lhs, const std::string &rhs) {
                          std::string ret{lhs};
                          if (!lhs.empty() && !rhs.empty()) ret += ", ";
                          ret += rhs;
                          return ret;
                        })
     << ")" << body_->String();

  return ss.str();
}

std::string GeneratorLiteral::String() const {
  std::stringstream ss;

  std::vector<std::string> params;
  for (auto p : parameters_) params.push_back(p->String());

  ss << TokenLiteral() << "("
     << std::accumulate(params.begin(), params.end(), std::string(),
                        [](const std::string &lhs, const std::string &rhs) {
                          std::string ret{lhs};
                          if (!lhs.empty() && !rhs.empty()) ret += ", ";
                          ret += rhs;
                          return ret;
                        })
     << ")" << setup_->String() << run_->String();

  return ss.str();
}

std::string MidiArrayExpression::String() const {
  std::stringstream ss;
  ss << "MIDI ARRAY!:" << token_.literal_ << ":";
  ss << elements_->String();
  return ss.str();
}

std::string SynthExpression::String() const {
  std::stringstream ss;
  ss << "SYNTH:" << token_.literal_;

  return ss.str();
}

std::string SynthPresetExpression::String() const {
  std::stringstream ss;
  ss << "SYNTH PRESET:" << token_.literal_;

  return ss.str();
}

std::string SynthLoadExpression::String() const {
  std::stringstream ss;
  ss << "SYNTH LOAD:" << token_.literal_ << ":" << preset_name_;

  return ss.str();
}

std::string SynthSaveExpression::String() const {
  std::stringstream ss;
  ss << "SYNTH SAVE:" << token_.literal_ << ":" << preset_name_;

  return ss.str();
}
std::string PatternExpression::String() const {
  std::stringstream ss;
  ss << "PATTERN:" << string_pattern;
  return ss.str();
}

std::string SampleExpression::String() const { return "SAMPLE"; }

std::string GranularExpression::String() const { return "GRANULAR"; }

std::string PatternFunctionExpression::String() const {
  std::stringstream ss;
  ss << "PATTERN FUNCTION EXPRESSION!:" << token_.literal_;

  return ss.str();
}

ProcessSetStatement::ProcessSetStatement(Token token) : Statement(token) {
  mixer_process_id_ = std::stoi(token.literal_);
}

std::string ProcessSetStatement::String() const {
  return "process set statement yo";
}

ProcessStatement::ProcessStatement(Token token) : Statement(token) {
  mixer_process_id_ = std::stoi(token.literal_);
}

std::string ProcessStatement::String() const {
  std::stringstream ss;
  ss << "p" << token_.literal_;
  if (process_type_ == PATTERN_PROCESS) {
    if (target_type_ == ENV)
      ss << " $ ";
    else
      ss << " # ";
  }
  ss << command_ << " ";
  for (auto &t : targets_) ss << t << ",";
  ss << " ";
  for (auto &f : functions_) ss << f->String();

  return ss.str();
}

std::string CallExpression::String() const {
  std::stringstream ss;

  std::vector<std::string> arguments;
  for (auto a : arguments_) {
    if (a) arguments.push_back(a->String());
  }

  ss << function_->String() << "("
     << std::accumulate(arguments.begin(), arguments.end(), std::string(),
                        [](const std::string &lhs, const std::string &rhs) {
                          std::string ret{lhs};
                          if (!lhs.empty() && !rhs.empty()) ret += ", ";
                          ret += rhs;
                          return ret;
                        })
     << ")";

  return ss.str();
}

std::string ArrayLiteral::String() const {
  std::stringstream ss;

  std::vector<std::string> elements;
  for (auto const &e : elements_) elements.push_back(e->String());

  int elements_size = elements.size();
  ss << "[";
  for (int i = 0; i < elements_size; i++) {
    ss << elements[i];
    if (i != elements_size - 1) ss << ", ";
  }
  ss << "]";

  return ss.str();
}

std::string HashLiteral::String() const {
  std::stringstream ss;

  std::vector<std::string> pairs;
  for (auto const &it : pairs_)
    pairs.push_back(it.first->String() + ":" + it.second->String());

  int pairs_size = pairs.size();
  ss << "{";
  for (int i = 0; i < pairs_size; i++) {
    ss << pairs[i];
    if (i != pairs_size - 1) ss << ", ";
  }
  ss << "}";

  return ss.str();
}

std::string IndexExpression::String() const {
  std::stringstream ss;
  ss << "(";
  if (left_) ss << left_->String();
  ss << "[";
  if (index_) ss << index_->String();
  ss << "])";
  return ss.str();
}

std::string InfixExpression::String() const {
  std::stringstream ss;
  ss << "(";
  if (left_) ss << left_->String();
  if (!operator_.empty()) ss << operator_;
  if (right_) ss << right_->String();
  ss << ")";

  return ss.str();
}

std::string Identifier::String() const { return value_; }
std::string NumberLiteral::String() const { return token_.literal_; }
std::string BooleanExpression::String() const { return token_.literal_; }

}  // namespace ast
