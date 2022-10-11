#include <audioutils.h>
#include <fx/dynamics_processor.h>
#include <mixer.h>

#include <interpreter/sound_cmds.hpp>
#include <iostream>

extern Mixer *mixr;

namespace interpreter_sound_cmds {

void ParseFXCmd(std::vector<std::shared_ptr<object::Object>> &args) {
  auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
  if (soundgen) {
    if (mixr->IsValidSoundgenNum(soundgen->soundgen_id_)) {
      auto sg = mixr->sound_generators_[soundgen->soundgen_id_];

      std::shared_ptr<object::String> str_obj =
          std::dynamic_pointer_cast<object::String>(args[1]);
      if (str_obj) {
        if (str_obj->value_ == "bitcrush")
          sg->AddBitcrush();
        else if (str_obj->value_ == "compressor")
          sg->AddCompressor();
        else if (str_obj->value_ == "delay")
          sg->AddDelay();
        else if (str_obj->value_ == "distort")
          sg->AddDistortion();
        else if (str_obj->value_ == "decimate")
          sg->AddDecimate();
        else if (str_obj->value_ == "filter")
          sg->AddBasicfilter();
        else if (str_obj->value_ == "reverb")
          sg->AddReverb();
        else if (str_obj->value_ == "sidechain") {
          if (args.size() > 2) {
            std::cout << "Got a source!\n";
            auto soundgen_sidechain_src =
                std::dynamic_pointer_cast<object::SoundGenerator>(args[2]);
            if (soundgen_sidechain_src &&
                mixr->IsValidSoundgenNum(
                    soundgen_sidechain_src->soundgen_id_)) {
              int fx_num = sg->AddCompressor();
              DynamicsProcessor *dp = (DynamicsProcessor *)sg->effects[fx_num];
              dp->SetExternalSource(soundgen_sidechain_src->soundgen_id_);
              dp->SetDefaultSidechainParams();
            }
          }
        } else if (str_obj->value_ == "moddelay")
          sg->AddModdelay();
        else if (str_obj->value_ == "modfilter")
          sg->AddModfilter();
        else if (str_obj->value_ == "waveshape")
          sg->AddWaveshape();
      }
    }
  }
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
