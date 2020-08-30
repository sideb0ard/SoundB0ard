#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <interpreter/ast.hpp>
#include <mixer.h>

namespace object
{

constexpr char NULL_OBJ[] = "NULL";
constexpr char ERROR_OBJ[] = "ERROR";

constexpr char NUMBER_OBJ[] = "NUMBER";
constexpr char BOOLEAN_OBJ[] = "BOOLEAN";

constexpr char RETURN_VALUE_OBJ[] = "RETURN_VALUE";

constexpr char FUNCTION_OBJ[] = "FUNCTION";
constexpr char GENERATOR_OBJ[] = "GENERATOR";

constexpr char STRING_OBJ[] = "STRING";

constexpr char BUILTIN_OBJ[] = "BUILTIN";

constexpr char ARRAY_OBJ[] = "ARRAY";

constexpr char HASH_OBJ[] = "HASH";

constexpr char PROCESS_OBJ[] = "PROCESS";

constexpr char SYNTH_OBJ[] = "SYNTH";
constexpr char SAMPLE_OBJ[] = "SAMPLE";
constexpr char GRANULAR_OBJ[] = "GRANULAR";

using ObjectType = std::string;

class HashKey
{
  public:
    HashKey() = default;
    HashKey(ObjectType type, uint64_t value) : type_{type}, value_{value} {};
    uint64_t Value() const { return value_; }
    std::string Type() const { return type_; }
    bool operator==(const HashKey &hk) const
    {
        return hk.type_ == type_ && hk.value_ == value_;
    }
    bool operator!=(const HashKey &hk) const
    {
        return hk.type_ != type_ || hk.value_ != value_;
    }

  private:
    ObjectType type_;
    uint64_t value_;
};

bool operator<(HashKey const &lhs, HashKey const &rhs);

class Object
{
  public:
    virtual ~Object() = default;
    virtual ObjectType Type() = 0;
    virtual std::string Inspect() = 0;
};

class Number : public Object
{
  public:
    explicit Number(double val) : value_{val} {};
    ObjectType Type() override;
    std::string Inspect() override;
    HashKey HashKey();

  public:
    double value_;
};

class Array : public Object
{
  public:
    explicit Array(std::vector<std::shared_ptr<Object>> elements)
        : elements_{elements} {};
    ObjectType Type() override { return ARRAY_OBJ; }
    std::string Inspect() override;

  public:
    std::vector<std::shared_ptr<Object>> elements_;
};

class Boolean : public Object
{
  public:
    explicit Boolean(bool val) : value_{val} {};
    ObjectType Type() override;
    std::string Inspect() override;
    HashKey HashKey();

  public:
    bool value_;
};

class String : public Object
{
  public:
    explicit String(std::string val) : value_{val} {};
    ObjectType Type() override { return STRING_OBJ; }
    std::string Inspect() override { return value_; }
    HashKey HashKey();

  public:
    std::string value_;
};

class ReturnValue : public Object
{
  public:
    explicit ReturnValue(std::shared_ptr<Object> val) : value_{val} {};
    ObjectType Type() override;
    std::string Inspect() override;

  public:
    std::shared_ptr<Object> value_;
};

class Null : public Object
{
  public:
    ObjectType Type() override;
    std::string Inspect() override;
};

class Error : public Object
{
  public:
    std::string message_;

  public:
    Error() = default;
    explicit Error(std::string err_msg);
    ObjectType Type() override;
    std::string Inspect() override;
};

class Environment
{
  public:
    Environment() = default;
    explicit Environment(std::shared_ptr<Environment> outer_env)
        : outer_env_{outer_env} {};
    ~Environment() = default;
    std::shared_ptr<Object> Get(std::string key);
    std::shared_ptr<Object> Set(std::string key, std::shared_ptr<Object> val);
    std::string Debug();
    std::unordered_map<std::string, int> GetSoundGenerators();

  private:
    std::unordered_map<std::string, std::shared_ptr<Object>> store_;
    std::shared_ptr<Environment> outer_env_;
};

/////////////////////////////////////////////////

class Function : public Object
{
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

class Generator : public Object
{
  public:
    Generator(std::vector<std::shared_ptr<ast::Identifier>> parameters,
              std::shared_ptr<Environment> env,
              std::shared_ptr<ast::BlockStatement> setup,
              std::shared_ptr<ast::BlockStatement> run)
        : parameters_{parameters}, env_{env}, setup_{setup}, run_{run} {};
    ~Generator() = default;
    ObjectType Type() override;
    std::string Inspect() override;

  public:
    std::vector<std::shared_ptr<ast::Identifier>> parameters_;
    std::shared_ptr<Environment> env_;
    std::shared_ptr<ast::BlockStatement> setup_;
    std::shared_ptr<ast::BlockStatement> run_;
};

using BuiltInFunc = std::function<std::shared_ptr<object::Object>(
    std::vector<std::shared_ptr<object::Object>>)>;

class BuiltIn : public Object
{
  public:
    explicit BuiltIn(BuiltInFunc fn) : func_{fn} {}
    ~BuiltIn() = default;
    ObjectType Type() override { return BUILTIN_OBJ; }
    std::string Inspect() override { return "builtin function"; }

  public:
    BuiltInFunc func_;
};

/////////////////////////////////////////////////
class SoundGenerator : public Object
{
  public:
    SoundGenerator() = default;
    ~SoundGenerator() = default;
    virtual ObjectType Type() = 0;
    virtual std::string Inspect() = 0;

  public:
    int soundgen_id_{-1};
};

class FMSynth : public SoundGenerator
{
  public:
    FMSynth();
    ~FMSynth() = default;
    ObjectType Type() override;
    std::string Inspect() override;
};

class MoogSynth : public SoundGenerator
{
  public:
    MoogSynth();
    ~MoogSynth() = default;
    ObjectType Type() override;
    std::string Inspect() override;
};

class DrumSynth : public SoundGenerator
{
  public:
    DrumSynth();
    ~DrumSynth() = default;
    ObjectType Type() override;
    std::string Inspect() override;
};

class DigiSynth : public SoundGenerator
{
  public:
    DigiSynth(std::string sample_name);
    ~DigiSynth() = default;
    ObjectType Type() override;
    std::string Inspect() override;
};

class Sample : public SoundGenerator
{
  public:
    Sample(std::string sample_path);
    ~Sample() = default;
    ObjectType Type() override;
    std::string Inspect() override;
};

class Granular : public SoundGenerator
{
  public:
    Granular(std::string Granular_path, bool loop_mode);
    ~Granular() = default;
    ObjectType Type() override;
    std::string Inspect() override;
};

/////////////////////////////////////////////////

class HashPair
{
  public:
    HashPair(std::shared_ptr<Object> key, std::shared_ptr<Object> value)
        : key_{key}, value_{value}
    {
    }

  public:
    std::shared_ptr<Object> key_;
    std::shared_ptr<Object> value_;
};

class Hash : public Object
{
  public:
    Hash() = default;
    explicit Hash(std::map<HashKey, HashPair> pairs) : pairs_{pairs} {}
    ~Hash() = default;
    ObjectType Type() override { return HASH_OBJ; }
    std::string Inspect() override;

  public:
    std::map<HashKey, HashPair> pairs_;
};

} // namespace object
