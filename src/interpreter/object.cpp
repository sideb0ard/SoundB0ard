#include <audio_action_queue.h>
#include <defjams.h>
#include <drum_synth.h>
#include <drumsampler.h>
#include <dxsynth.h>
#include <looper.h>
#include <minisynth.h>
#include <mixer.h>
#include <sbsynth.h>
#include <utils.h>

#include <interpreter/object.hpp>
#include <iostream>
#include <memory>
#include <process.hpp>
#include <sstream>
#include <string>
#include <tsqueue.hpp>
#include <vector>

extern Tsqueue<std::unique_ptr<AudioActionItem>> audio_queue;
extern Tsqueue<int> audio_reply_queue;

namespace {
int AddSoundGenerator(unsigned int type, std::string filepath = "",
                      int loop_mode = 0) {
  std::unique_ptr<SBAudio::SoundGenerator> sg;
  switch (type) {
    case (MINISYNTH_TYPE):
      sg = std::make_unique<SBAudio::MiniSynth>();
      break;
    case (DXSYNTH_TYPE):
      sg = std::make_unique<SBAudio::DXSynth>();
      break;
    case (DRUMSYNTH_TYPE):
      sg = std::make_unique<SBAudio::DrumSynth>();
      break;
    case (LOOPER_TYPE):
      sg = std::make_unique<SBAudio::Looper>(filepath, loop_mode);
      break;
    case (DRUMSAMPLER_TYPE):
      sg = std::make_unique<SBAudio::DrumSampler>(filepath);
      break;
    case (SBSYNTH_TYPE):
      sg = std::make_unique<SBAudio::SBSynth>();
      break;
  }
  auto action = std::make_unique<AudioActionItem>(AudioAction::ADD);
  action->soundgenerator_type = type;
  action->sg = std::move(sg);
  action->filepath = filepath;

  std::thread::id this_id = std::this_thread::get_id();
  std::cout << "YO OBJECT - CREATE SOUND GENERTAOR!! threadid:" << this_id
            << "\n";
  audio_queue.push(std::move(action));
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

std::string BitOp::Inspect() { return bitop_; }
std::string BitOp::Type() { return BITOP_OBJ; }

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

Granular::Granular(std::string sample_path, int loop_mode) {
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

double Phasor::Generate() {
  signal_ = scale(counter_, 0, frequency_, 0, 1);
  counter_++;
  if (counter_ == frequency_) counter_ = 0;
  return signal_;
}

ObjectType Phasor::Type() { return PHASOR_OBJ; }
std::string Phasor::Inspect() {
  std::stringstream reply;
  reply << "phasor. freq:" << frequency_;
  return reply.str();
}
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

MidiArray::MidiArray(MultiEventMidiPattern events) : events_{events} {
  std::vector<midi_event> notes_off;

  for (int i = 0; i < PPBAR; i++) {
    if (events_[i].size() > 0) {
      for (auto &e : events_[i]) {
        switch (e.event_type) {
          case (MIDI_ON):
            notes_on_.push_back(e);
            break;
          case (MIDI_OFF):
            notes_off.push_back(e);
            break;
          case (MIDI_CONTROL):
            control_messages_.push_back(e);
            break;
        }
      }
    }
  }

  std::sort(notes_on_.begin(), notes_on_.end());
  std::sort(notes_off.begin(), notes_off.end());
  std::sort(control_messages_.begin(), control_messages_.end());

  // TODO - should combine the control and note_ons
  size_t smallest = std::min(notes_off.size(), notes_on_.size());
  for (size_t i = 0; i < smallest; ++i) {
    notes_on_[i].dur = notes_off[i].original_tick - notes_on_[i].original_tick;
    notes_on_[i].playback_tick = notes_on_[i].original_tick % PPBAR;
  }

  for (auto &e : control_messages_) e.playback_tick = e.original_tick % PPBAR;
}

ObjectType MidiArray::Type() { return MIDI_ARRAY; }

std::string MidiArray::Inspect() {
  std::stringstream ss;
  ss << "{";
  for (unsigned long i = 0; i < notes_on_.size(); i++) {
    auto e = notes_on_[i];
    ss << e.playback_tick << ":\"" << e.data1 << ":" << e.data2 << ":" << e.dur
       << "\"";
    if (i != notes_on_.size() - 1) {
      ss << ", ";
    }
  }
  ss << "}";
  return ss.str();
}

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
