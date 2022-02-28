#include <audio_action_queue.h>
#include <defjams.h>
#include <drum_synth.h>
#include <drumsampler.h>
#include <dxsynth.h>
#include <looper.h>
#include <minisynth.h>
#include <mixer.h>
#include <sbsynth.h>

#include <interpreter/object.hpp>
#include <iostream>
#include <memory>
#include <process.hpp>
#include <sstream>
#include <string>
#include <tsqueue.hpp>
#include <vector>

extern Tsqueue<audio_action_queue_item> audio_queue;
extern Tsqueue<int> audio_reply_queue;

namespace {
int AddSoundGenerator(unsigned int type, std::string filepath = "",
                      bool loop_mode = false) {
  std::shared_ptr<SBAudio::SoundGenerator> sg;
  switch (type) {
    case (MINISYNTH_TYPE):
      sg = std::make_shared<SBAudio::MiniSynth>();
      break;
    case (DXSYNTH_TYPE):
      sg = std::make_shared<SBAudio::DXSynth>();
      break;
    case (DRUMSYNTH_TYPE):
      sg = std::make_shared<SBAudio::DrumSynth>();
      break;
    case (LOOPER_TYPE):
      sg = std::make_shared<SBAudio::Looper>(filepath, loop_mode);
      break;
    case (DRUMSAMPLER_TYPE):
      sg = std::make_shared<SBAudio::DrumSampler>(filepath);
      break;
    case (SBSYNTH_TYPE):
      sg = std::make_shared<SBAudio::SBSynth>();
      break;
  }
  audio_action_queue_item action_req{.type = AudioAction::ADD,
                                     .soundgenerator_type = type,
                                     .sg = sg,
                                     .filepath = filepath,
                                     .loop_mode = loop_mode};
  audio_queue.push(action_req);
  auto sg_index = audio_reply_queue.pop();
  if (sg_index)
    return sg_index.value();
  else
    return -1;
}

}  // namespace

namespace object {

bool IsSoundGenerator(object::ObjectType type) {
  if (type == object::SYNTH_OBJ || type == object::SAMPLE_OBJ ||
      type == object::GRANULAR_OBJ)
    return true;
  return false;
}

std::string Number::Inspect() {
  std::stringstream val;
  val << value_;
  return val.str();
}

bool operator<(HashKey const &lhs, HashKey const &rhs) {
  if (lhs.Type() < rhs.Type())
    return true;
  else if (lhs.Type() > rhs.Type())
    return false;
  else  // same type - compare values
  {
    if (lhs.Value() < rhs.Value())
      return true;
    else if (lhs.Value() > rhs.Value())
      return false;
    else  // value_ is equal
      return false;
  }
}

ObjectType Number::Type() { return NUMBER_OBJ; }
object::HashKey Number::HashKey() {
  return object::HashKey(Type(), (uint64_t)value_);
}

std::string Boolean::Inspect() {
  std::stringstream val;
  val << (value_ ? "true" : "false");
  return val.str();
}
ObjectType Boolean::Type() { return BOOLEAN_OBJ; }
object::HashKey Boolean::HashKey() {
  uint64_t val = 0;
  if (value_) val = 1;

  return object::HashKey(Type(), (uint64_t)val);
}

object::HashKey String::HashKey() {
  std::hash<std::string> hasher;
  return object::HashKey(Type(), (uint64_t)hasher(value_));
}

std::string Break::Inspect() { return "break"; }
ObjectType Break::Type() { return BREAK_OBJ; }

std::string Null::Inspect() { return "null"; }
ObjectType Null::Type() { return NULL_OBJ; }

FMSynth::FMSynth() {
  soundgen_id_ = AddSoundGenerator(DXSYNTH_TYPE);
  soundgenerator_type = DXSYNTH_TYPE;
}
std::string FMSynth::Inspect() { return "FM synth."; }
ObjectType FMSynth::Type() { return SYNTH_OBJ; }

object::HashKey SoundGenerator::HashKey() {
  return object::HashKey(Type(), (uint64_t)soundgen_id_);
}

SBSynth::SBSynth() {
  soundgen_id_ = AddSoundGenerator(SBSYNTH_TYPE);
  soundgenerator_type = SBSYNTH_TYPE;
}
std::string SBSynth::Inspect() { return "SB synth."; }
ObjectType SBSynth::Type() { return SYNTH_OBJ; }

MoogSynth::MoogSynth() {
  soundgen_id_ = AddSoundGenerator(MINISYNTH_TYPE);
  soundgenerator_type = MINISYNTH_TYPE;
}
std::string MoogSynth::Inspect() { return "Moog synth."; }
ObjectType MoogSynth::Type() { return SYNTH_OBJ; }

DrumSynth::DrumSynth() {
  soundgen_id_ = AddSoundGenerator(DRUMSYNTH_TYPE);
  soundgenerator_type = DRUMSYNTH_TYPE;
}
std::string DrumSynth::Inspect() { return "Drum synth."; }
ObjectType DrumSynth::Type() { return SYNTH_OBJ; }

Sample::Sample(std::string sample_path) {
  sample_path_ = sample_path;
  soundgen_id_ = AddSoundGenerator(DRUMSAMPLER_TYPE, sample_path);
  soundgenerator_type = DRUMSAMPLER_TYPE;
}
std::string Sample::Inspect() { return "sample."; }
ObjectType Sample::Type() { return SAMPLE_OBJ; }

Granular::Granular(std::string sample_path, bool loop_mode) {
  std::cout << "OBJECT! LOOPM ODE IS " << loop_mode << std::endl;
  soundgen_id_ = AddSoundGenerator(LOOPER_TYPE, sample_path, loop_mode);
  soundgenerator_type = LOOPER_TYPE;
}
std::string Granular::Inspect() { return "Granular."; }
ObjectType Granular::Type() { return GRANULAR_OBJ; }

std::string ReturnValue::Inspect() { return value_->Inspect(); }
ObjectType ReturnValue::Type() { return RETURN_VALUE_OBJ; }

Error::Error(std::string err_msg) : message_{err_msg} {}
std::string Error::Inspect() { return "ERROR: " + message_; }
ObjectType Error::Type() { return ERROR_OBJ; }

ObjectType Function::Type() { return FUNCTION_OBJ; }
std::string Function::Inspect() {
  std::stringstream params;
  int len = parameters_.size();
  int i = 0;
  for (auto &p : parameters_) {
    params << p->String();
    if (i < len - 1) params << ", ";
    i++;
  }
  std::stringstream return_val;
  return_val << "fn(" << params.str() << ") {\n";
  return_val << body_->String() << "\n)";
  return_val << "}\n";

  return return_val.str();
}

ObjectType Generator::Type() { return GENERATOR_OBJ; }
std::string Generator::Inspect() {
  std::stringstream params;
  int len = parameters_.size();
  int i = 0;
  for (auto &p : parameters_) {
    params << p->String();
    if (i < len - 1) params << ", ";
    i++;
  }
  std::stringstream return_val;
  return_val << "gn(" << params.str() << ") \n";
  return_val << "setup() {\n" << setup_->String() << "}\n";
  return_val << "run() {\n" << run_->String() << "}\n";

  return return_val.str();
}

std::string At::Inspect() {
  std::stringstream val;
  val << value_;
  return val.str();
}
ObjectType At::Type() { return AT_OBJ; }

std::string Duration::Inspect() {
  std::stringstream val;
  val << value_;
  return val.str();
}
ObjectType Duration::Type() { return DURATION_OBJ; }

std::string Velocity::Inspect() {
  std::stringstream val;
  val << value_;
  return val.str();
}
ObjectType Velocity::Type() { return VELOCITY_OBJ; }

ObjectType ForLoop::Type() { return FORLOOP_OBJ; }
std::string ForLoop::Inspect() { return "FOR LOOP"; }

std::string Array::Inspect() {
  std::stringstream elems;
  int len = elements_.size();
  int i = 0;
  for (auto &e : elements_) {
    elems << e->Inspect();
    if (i < len - 1) elems << ", ";
    i++;
  }
  std::stringstream return_val;
  return_val << "[" << elems.str() << "]";

  return return_val.str();
}

std::string Environment::ListFuncsAndGen() {
  std::stringstream ss;
  for (const auto &it : store_) {
    if (it.second->Type() == "FUNCTION") {
      ss << ANSI_COLOR_WHITE << it.first << COOL_COLOR_PINK << " = fn()"
         << std::endl;
    } else if (it.second->Type() == "GENERATOR") {
      ss << ANSI_COLOR_WHITE << it.first << COOL_COLOR_PINK << " = gen()"
         << std::endl;
    }
  }
  ss << ANSI_COLOR_RESET;
  return ss.str();
}
std::string Environment::Debug() {
  std::stringstream ss;
  for (const auto &it : store_) {
    if (IsSoundGenerator(it.second->Type())) {
      // no-op - sg ps happens in mixer.
    } else if (it.second->Type() == "FUNCTION" ||
               it.second->Type() == "GENERATOR") {
      // no-op
    } else {
      if (it.first == "rhythms_int") {
        // ignore! too messy!
      } else {
        ss << ANSI_COLOR_WHITE << it.first << COOL_COLOR_GREEN << " = "
           << it.second->Inspect() << std::endl;
      }
    }
  }
  ss << ANSI_COLOR_RESET;
  return ss.str();
}

std::map<std::string, int> Environment::GetSoundGenerators() {
  std::map<std::string, int> soundgens;
  for (const auto &it : store_) {
    if (IsSoundGenerator(it.second->Type())) {
      auto gen = std::dynamic_pointer_cast<object::SoundGenerator>(it.second);
      if (gen) {
        soundgens.insert({it.first, gen->soundgen_id_});
      }
    }
  }
  return soundgens;
}

std::shared_ptr<Object> Environment::Get(std::string name) {
  auto entry = store_.find(name);
  if (entry == store_.end()) {
    if (outer_env_) return outer_env_->Get(name);
    return nullptr;
  }
  return entry->second;
}

std::shared_ptr<Object> Environment::Set(std::string key,
                                         std::shared_ptr<Object> val,
                                         bool create) {
  if (val->Type() == "NUMBER") {
    // make copy of value
    auto num_obj = std::dynamic_pointer_cast<object::Number>(val);
    if (!num_obj) {
      std::cerr << "OOFt, SOMETHING UP\n";
      return nullptr;
    }
    // make a copy - don't want to change original val
    auto num_copy = std::make_shared<object::Number>(num_obj->value_);

    val = num_copy;
  }

  if (create) {
    store_[key] = val;
  } else {
    auto entry = store_.find(key);
    if (entry != store_.end()) {
      store_[key] = val;
    } else {
      if (outer_env_) return outer_env_->Set(key, val, create);
      std::cerr << key << " not found.\n";
      return nullptr;
    }
  }
  return val;
}

std::string Hash::Inspect() {
  std::stringstream out;
  std::vector<std::string> pairs;
  for (auto const &it : pairs_) {
    pairs.push_back(it.second.key_->Inspect() + ": " +
                    it.second.value_->Inspect());
  }

  int pairs_size = pairs.size();
  out << "{";
  for (int i = 0; i < pairs_size; i++) {
    out << pairs[i];
    if (i != pairs_size - 1) out << ", ";
  }
  out << "}";

  return out.str();
}

}  // namespace object
