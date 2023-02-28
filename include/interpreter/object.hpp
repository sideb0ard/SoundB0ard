#pragma once

#include <array>
#include <functional>
#include <interpreter/ast.hpp>
#include <map>
#include <memory>
#include <pattern_parser/ast.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "defjams.h"

namespace object {

constexpr char NULL_OBJ[] = "NULL";
constexpr char ERROR_OBJ[] = "ERROR";

constexpr char NUMBER_OBJ[] = "NUMBER";
constexpr char BOOLEAN_OBJ[] = "BOOLEAN";

constexpr char BITOP_OBJ[] = "BITOP";

constexpr char RETURN_VALUE_OBJ[] = "RETURN_VALUE";

constexpr char BREAK_OBJ[] = "BREAK";
constexpr char FORLOOP_OBJ[] = "FOR LOOP";
constexpr char FUNCTION_OBJ[] = "FUNCTION";
constexpr char GENERATOR_OBJ[] = "GENERATOR";
constexpr char PHASOR_OBJ[] = "PHASOR";

constexpr char STRING_OBJ[] = "STRING";

constexpr char BUILTIN_OBJ[] = "BUILTIN";

constexpr char ARRAY_OBJ[] = "ARRAY";

constexpr char HASH_OBJ[] = "HASH";

constexpr char PROCESS_OBJ[] = "PROCESS";
constexpr char PATTERN_OBJ[] = "PATTERN";

constexpr char SYNTH_OBJ[] = "SYNTH";
constexpr char SAMPLE_OBJ[] = "SAMPLE";
constexpr char GRANULAR_OBJ[] = "GRANULAR";

constexpr char MIDI_ARRAY[] = "MIDI_ARRAY";

constexpr char AT_OBJ[] = "AT";
constexpr char DURATION_OBJ[] = "DURATION";
constexpr char VELOCITY_OBJ[] = "VELOCITY";

using ObjectType = std::string;

class HashKey {
 public:
  HashKey() = default;
  HashKey(ObjectType type, uint64_t value) : type_{type}, value_{value} {};
  uint64_t Value() const { return value_; }
  std::string Type() const { return type_; }
  bool operator==(const HashKey &hk) const {
    return hk.type_ == type_ && hk.value_ == value_;
  }
  bool operator!=(const HashKey &hk) const {
    return hk.type_ != type_ || hk.value_ != value_;
  }

 private:
  ObjectType type_;
  uint64_t value_;
};

bool operator<(HashKey const &lhs, HashKey const &rhs);

class Object {
 public:
  virtual ~Object() = default;
  virtual ObjectType Type() = 0;
  virtual std::string Inspect() = 0;
};

class Break : public Object {
 public:
  Break() = default;
  ObjectType Type() override;
  std::string Inspect() override;
};

class BitOp : public Object {
 public:
  BitOp(std::string bitop) : bitop_{bitop} {};
  ObjectType Type() override;
  std::string Inspect() override;

 public:
  std::string bitop_;
};

class Number : public Object {
 public:
  explicit Number(double val) : value_{val} {};
  ObjectType Type() override;
  std::string Inspect() override;
  HashKey HashKey();

 public:
  double value_;
};

class At : public Object {
 public:
  explicit At(double val) : value_{val} {};
  ObjectType Type() override;
  std::string Inspect() override;

 public:
  double value_;
};

class Duration : public Object {
 public:
  explicit Duration(double val) : value_{val} {};
  ObjectType Type() override;
  std::string Inspect() override;

 public:
  double value_;
};

class Velocity : public Object {
 public:
  explicit Velocity(double val) : value_{val} {};
  ObjectType Type() override;
  std::string Inspect() override;

 public:
  double value_;
};

class Array : public Object {
 public:
  explicit Array(std::vector<std::shared_ptr<Object>> elements)
      : elements_{elements} {};
  ObjectType Type() override { return ARRAY_OBJ; }
  std::string Inspect() override;

 public:
  std::vector<std::shared_ptr<Object>> elements_;
};

class Boolean : public Object {
 public:
  explicit Boolean(bool val) : value_{val} {};
  ObjectType Type() override;
  std::string Inspect() override;
  HashKey HashKey();

 public:
  bool value_;
};

class String : public Object {
 public:
  explicit String(std::string val) : value_{val} {};
  ObjectType Type() override { return STRING_OBJ; }
  std::string Inspect() override { return value_; }
  HashKey HashKey();

 public:
  std::string value_;
};

class ReturnValue : public Object {
 public:
  explicit ReturnValue(std::shared_ptr<Object> val) : value_{val} {};
  ObjectType Type() override;
  std::string Inspect() override;

 public:
  std::shared_ptr<Object> value_;
};

class Null : public Object {
 public:
  ObjectType Type() override;
  std::string Inspect() override;
};

class Error : public Object {
 public:
  std::string message_;

 public:
  Error() = default;
  explicit Error(std::string err_msg);
  ObjectType Type() override;
  std::string Inspect() override;
};

class Environment {
 public:
  Environment() = default;
  explicit Environment(std::shared_ptr<Environment> outer_env)
      : outer_env_{outer_env} {};
  ~Environment() = default;
  std::shared_ptr<Object> Get(std::string key);
  std::shared_ptr<Object> Set(std::string key, std::shared_ptr<Object> val,
                              bool create = true);
  std::string Debug();
  std::string ListFuncsAndGen();
  std::map<std::string, int> GetSoundGenerators();

 private:
  std::unordered_map<std::string, std::shared_ptr<Object>> store_;
  std::shared_ptr<Environment> outer_env_{nullptr};
};

/////////////////////////////////////////////////

class Phasor : public Object {
 public:
  Phasor(int frequency) : frequency_{frequency} {};
  ~Phasor() = default;
  ObjectType Type() override;
  std::string Inspect() override;

 private:
  int frequency_{0};  // how often this will be called
  double signal_{0};  // return value
  int counter_{0};    // internal counter

 public:
  double Generate();
};

class Function : public Object {
 public:
  Function(std::vector<std::shared_ptr<ast::Identifier>> parameters,
           std::shared_ptr<Environment> env,
           std::shared_ptr<ast::BlockStatement> body)
      : parameters_{parameters}, env_{env}, body_{body} {};
  ~Function() = default;
  ObjectType Type() override;
  std::string Inspect() override;

 public:
  std::vector<std::shared_ptr<ast::Identifier>> parameters_;
  std::shared_ptr<Environment> env_;
  std::shared_ptr<ast::BlockStatement> body_;
};

class MidiArray : public Object {
 public:
  MidiArray();
  MidiArray(MultiEventMidiPattern events);
  MidiArray(std::vector<midi_event> notes_on) : notes_on_{notes_on} {};
  ~MidiArray() = default;
  ObjectType Type() override;
  std::string Inspect() override;

  MultiEventMidiPattern events_;
  std::vector<midi_event> notes_on_;
  std::vector<midi_event> control_messages_;
};

class Pattern : public Object {
 public:
  Pattern(std::string string_pattern);
  ~Pattern() = default;
  ObjectType Type() override;
  std::string Inspect() override;

 private:
  int eval_counter{0};
  std::string string_pattern{};
  std::shared_ptr<pattern_parser::PatternNode> pattern_root{nullptr};
  std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> pattern_events;

 public:
  std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> Eval();
  std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> Print();
  // void RunPattern();

 private:
  void ParseStringPattern();

  void EvalPattern(std::shared_ptr<pattern_parser::PatternNode> const &pattern,
                   int target_start, int target_end);
};

// TODO - rename to ProcessGenerator
class Generator : public Object {
 public:
  Generator(std::vector<std::shared_ptr<ast::Identifier>> parameters,
            std::shared_ptr<Environment> env,
            std::shared_ptr<ast::BlockStatement> setup,
            std::shared_ptr<ast::BlockStatement> run,
            std::shared_ptr<ast::BlockStatement> signal_generator = nullptr)
      : parameters_{parameters},
        env_{env},
        setup_{setup},
        run_{run},
        signal_generator_{signal_generator} {};
  ~Generator() = default;
  ObjectType Type() override;
  std::string Inspect() override;

 public:
  std::vector<std::shared_ptr<ast::Identifier>> parameters_;
  std::shared_ptr<Environment> env_;
  std::shared_ptr<ast::BlockStatement> setup_;
  std::shared_ptr<ast::BlockStatement> run_;
  std::shared_ptr<ast::BlockStatement> signal_generator_;
};

class ForLoop : public Object {
 public:
  ForLoop(std::shared_ptr<Environment> env, std::shared_ptr<ast::Identifier> it,
          std::shared_ptr<ast::Expression> iterator_value,
          std::shared_ptr<ast::Expression> termination_condition,
          std::shared_ptr<ast::Expression> increment,
          std::shared_ptr<ast::BlockStatement> body)
      : env_{env},
        iterator_{it},
        iterator_value_{iterator_value},
        termination_condition_{termination_condition},
        increment_{increment},
        body_{body} {};
  ~ForLoop() = default;
  ObjectType Type() override;
  std::string Inspect() override;

 public:
  std::shared_ptr<Environment> env_;

  std::shared_ptr<ast::Identifier> iterator_{nullptr};
  std::shared_ptr<ast::Expression> iterator_value_{nullptr};

  std::shared_ptr<ast::Expression> termination_condition_{nullptr};

  std::shared_ptr<ast::Expression> increment_{nullptr};

  std::shared_ptr<ast::BlockStatement> body_;
};

using BuiltInFunc = std::function<std::shared_ptr<object::Object>(
    std::vector<std::shared_ptr<object::Object>>)>;

class BuiltIn : public Object {
 public:
  explicit BuiltIn(BuiltInFunc fn) : func_{fn} {}
  ~BuiltIn() = default;
  ObjectType Type() override { return BUILTIN_OBJ; }
  std::string Inspect() override { return "builtin function"; }

 public:
  BuiltInFunc func_;
};

/////////////////////////////////////////////////
class SoundGenerator : public Object {
 public:
  SoundGenerator() = default;
  ~SoundGenerator() = default;
  virtual ObjectType Type() = 0;
  virtual std::string Inspect() = 0;
  HashKey HashKey();

 public:
  int soundgen_id_{-1};
  int soundgenerator_type{};
};

class FMSynth : public SoundGenerator {
 public:
  FMSynth();
  ~FMSynth() = default;
  ObjectType Type() override;
  std::string Inspect() override;
};

class DrumSynth : public SoundGenerator {
 public:
  DrumSynth();
  ~DrumSynth() = default;
  ObjectType Type() override;
  std::string Inspect() override;
};

class MoogSynth : public SoundGenerator {
 public:
  MoogSynth();
  ~MoogSynth() = default;
  ObjectType Type() override;
  std::string Inspect() override;
};

class Sample : public SoundGenerator {
 public:
  Sample(std::string sample_path);
  ~Sample() = default;
  ObjectType Type() override;
  std::string Inspect() override;
  std::string sample_path_{};
};

class Granular : public SoundGenerator {
 public:
  Granular(std::string Granular_path, int loop_mode);
  ~Granular() = default;
  ObjectType Type() override;
  std::string Inspect() override;
};

class SBSynth : public SoundGenerator {
 public:
  SBSynth();
  ~SBSynth() = default;
  ObjectType Type() override;
  std::string Inspect() override;
};

/////////////////////////////////////////////////

class HashPair {
 public:
  HashPair(std::shared_ptr<Object> key, std::shared_ptr<Object> value)
      : key_{key}, value_{value} {}

 public:
  std::shared_ptr<Object> key_;
  std::shared_ptr<Object> value_;
};

class Hash : public Object {
 public:
  Hash() = default;
  explicit Hash(std::map<HashKey, HashPair> pairs) : pairs_{pairs} {}
  ~Hash() = default;
  ObjectType Type() override { return HASH_OBJ; }
  std::string Inspect() override;

 public:
  std::map<HashKey, HashPair> pairs_;
};

bool IsSoundGenerator(object::ObjectType type);

}  // namespace object
