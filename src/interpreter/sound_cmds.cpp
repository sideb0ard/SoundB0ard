#include <audioutils.h>
#include <fx/basicfilterpass.h>
#include <fx/bitcrush.h>
#include <fx/decimate.h>
#include <fx/distortion.h>
#include <fx/dynamics_processor.h>
#include <fx/fx.h>
#include <fx/genz.h>
#include <fx/modfilter.h>
#include <fx/modular_delay.h>
#include <fx/reverb.h>
#include <fx/stereodelay.h>
#include <fx/waveshaper.h>
#include <mixer.h>

#include <interpreter/sound_cmds.hpp>
#include <iostream>

extern Mixer *mixr;

namespace interpreter_sound_cmds {

std::vector<std::shared_ptr<Fx>> ParseFXCmd(
    std::vector<std::shared_ptr<object::Object>> &args) {
  int args_size = args.size();
  std::vector<std::shared_ptr<Fx>> fx;
  if (args_size >= 2) {
    std::cout << "PARSEFX\n";

    std::shared_ptr<object::String> str_obj;
    for (int i = 1; i < args.size(); i++) {
      str_obj = std::dynamic_pointer_cast<object::String>(args[i]);
      if (str_obj) {
        if (str_obj->value_ == "bitcrush") {
          fx.push_back(std::make_unique<BitCrush>());
        } else if (str_obj->value_ == "compressor") {
          fx.push_back(std::make_unique<DynamicsProcessor>());
        } else if (str_obj->value_ == "delay") {
          std::cout << "CREATING DELAY>>>\n";
          fx.push_back(std::make_shared<StereoDelay>());
        } else if (str_obj->value_ == "distort") {
          std::cout << "BOOYA! Distortion all up in this kittycat!\n";
          fx.push_back(std::make_shared<Distortion>());
        } else if (str_obj->value_ == "decimate") {
          fx.push_back(std::make_shared<Decimate>());
        } else if (str_obj->value_ == "filter") {
          fx.push_back(std::make_shared<FilterPass>());
        } else if (str_obj->value_ == "genz") {
          fx.push_back(std::make_shared<GenZ>());
        } else if (str_obj->value_ == "reverb") {
          fx.push_back(std::make_shared<Reverb>());
        } else if (str_obj->value_ == "sidechain") {
          if (args.size() > 2) {
            std::cout << "Got a source!\n";
            auto soundgen_sidechain_src =
                std::dynamic_pointer_cast<object::SoundGenerator>(args[2]);
            if (soundgen_sidechain_src &&
                mixr->IsValidSoundgenNum(
                    soundgen_sidechain_src->soundgen_id_)) {
              auto dp = std::make_shared<DynamicsProcessor>();
              dp->SetExternalSource(soundgen_sidechain_src->soundgen_id_);
              dp->SetDefaultSidechainParams();
              fx.push_back(dp);
            }
          }
        } else if (str_obj->value_ == "moddelay") {
          fx.push_back(std::make_shared<ModDelay>());
        } else if (str_obj->value_ == "modfilter") {
          fx.push_back(std::make_shared<ModFilter>());
        } else if (str_obj->value_ == "waveshape") {
          fx.push_back(std::make_shared<WaveShaper>());
        }
      }
    }
  }
  return fx;
}

void ParseSynthCmd(std::vector<std::shared_ptr<object::Object>> &args) {
  if (args.size() < 2) return;

  auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
  if (soundgen) {
    if (mixr->IsValidSoundgenNum(soundgen->soundgen_id_)) {
      auto sg = mixr->sound_generators_[soundgen->soundgen_id_];
      std::shared_ptr<object::String> str_obj =
          std::dynamic_pointer_cast<object::String>(args[1]);
      if (str_obj) {
        if (str_obj->value_ == "list") {
          sg->ListPresets();
        } else if (args.size() == 3) {
          std::shared_ptr<object::String> str_cmd =
              std::dynamic_pointer_cast<object::String>(args[2]);
          // if (str_cmd->value_ == "load")
          //   sg->Load(str_obj->value_);
          //   TODO - OFFLOAD THIS FROM AUDIO THREAD
          if (str_cmd->value_ == "save") sg->Save(str_obj->value_);
        }
      }
    }
  }
}

void SynthLoadPreset(std::shared_ptr<object::Object> &obj,
                     const std::string &preset_name,
                     const std::map<std::string, double> &preset) {
  auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(obj);
  if (soundgen) {
    if (mixr->IsValidSoundgenNum(soundgen->soundgen_id_)) {
      auto sg = mixr->sound_generators_[soundgen->soundgen_id_];

      sg->LoadPreset(preset_name, preset);
    }
  }
}

std::vector<int> GetNotesInKey(int root, int scale_type) {
  std::vector<int> notes;

  switch (scale_type) {
    case (0):  // MAJOR
      notes.push_back(root + 0);
      notes.push_back(root + 2);
      notes.push_back(root + 4);
      notes.push_back(root + 5);
      notes.push_back(root + 7);
      notes.push_back(root + 9);
      notes.push_back(root + 11);
      break;
    case (1):  // MINOR
      notes.push_back(root + 0);
      notes.push_back(root + 2);
      notes.push_back(root + 3);
      notes.push_back(root + 5);
      notes.push_back(root + 7);
      notes.push_back(root + 8);
      notes.push_back(root + 10);
      break;
    case (2):  // PHRYGIAN
      notes.push_back(root + 0);
      notes.push_back(root + 1);
      notes.push_back(root + 3);
      notes.push_back(root + 5);
      notes.push_back(root + 7);
      notes.push_back(root + 8);
      notes.push_back(root + 10);
      break;
  }

  return notes;
}

}  // namespace interpreter_sound_cmds
