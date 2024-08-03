#include <drum_synth.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "midi_freq_table.h"
#include "utils.h"

namespace {
std::array<std::string, 8> osc_types{"SINE", "SAW1",   "SAW2",  "SAW3",
                                     "TRI",  "SQUARE", "NOISE", "PSNOISE"};
std::string GetOscType(int type) {
  std::string the_type = "dunno";
  if (type < osc_types.size()) return osc_types[type];
  return the_type;
}

}  // namespace

namespace SBAudio {

DrumSynth::DrumSynth() {
  bd_ = std::make_unique<BassDrum>();

  LoadSettings(DrumSettings());
  active = true;
}

StereoVal DrumSynth::GenNext(mixer_timing_info tinfo) {
  StereoVal out = {.left = 0, .right = 0};
  if (!active) return out;

  auto bd_out = bd_->Generate();
  out.left += bd_out.left;
  out.right += bd_out.right;

  return out;
}

void DrumSynth::SetParam(std::string name, double val) {
  if (name == "volume")
    settings_.volume = val;
  else if (name == "bd_vol")
    settings_.bd_vol = val;
  else if (name == "bd_pan")
    settings_.bd_pan = val;
  else if (name == "bd_tone")
    settings_.bd_tone = val;
  else if (name == "bd_q")
    settings_.bd_q = val;
  else if (name == "bd_ntone")
    settings_.bd_tone = val;
  else if (name == "bd_nq")
    settings_.bd_q = val;
  else if (name == "bd_decay")
    settings_.bd_decay = val;
  else if (name == "bd_octave")
    settings_.bd_octave = val;
  else if (name == "bd_key")
    settings_.bd_key = val;
  else if (name == "bd_detune")
    settings_.bd_detune_cents = val;
  else if (name == "bd_hard_sync")
    settings_.bd_hard_sync = val;
  else if (name == "bd_dist")
    settings_.bd_distortion_threshold = val;
  Update();
}

std::string DrumSynth::Info() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << COOL_COLOR_PINK2;
  ss << "DrumZynth - " << settings_.name << " - vol:" << volume
     << " pan:" << pan << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW "     bd(0): bd_vol:" << settings_.bd_vol
     << " bd_pan:" << settings_.bd_pan << " bd_nvol:" << settings_.bd_noise_vol
     << " bd_octave:" << settings_.bd_octave << " bd_key:" << settings_.bd_key
     << " bd_detune:" << settings_.bd_detune_cents
     << " bd_hard_sync:" << settings_.bd_hard_sync << std::endl;
  ss << "     bd_tone:" << settings_.bd_tone << " bd_q:" << settings_.bd_q
     << " bd_ntone:" << settings_.bd_ntone << " bd_nq:" << settings_.bd_nq
     << " bd_decay:" << settings_.bd_decay
     << " bd_dist:" << settings_.bd_distortion_threshold << std::endl;

  return ss.str();
}

std::string DrumSynth::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "DrumZynth - " << COOL_COLOR_PINK2 << " - vol:" << volume;

  return ss.str();
}

void DrumSynth::Start() {
  if (active) return;  // no-op
  active = true;
}

void DrumSynth::Stop() {
  if (active) return;  // no-op
  active = false;
}

// no-op
void DrumSynth::NoteOff(midi_event ev) {}

void DrumSynth::NoteOn(midi_event ev) {
  unsigned int drum_module_num = ev.data1;
  unsigned int velocity = ev.data2;

  double amplitude = scaleybum(0, 127, 0, 1, velocity);

  switch (drum_module_num) {
    case (0):
      // Bass Drum
      bd_->NoteOn(amplitude);
      break;
    case (1):
      // Snare Drum
      break;
    default:
      std::cerr << "DrumSynth - num not implemented:" << drum_module_num
                << std::endl;
  }
}

void DrumSynth::Save(std::string new_preset_name) {
  std::cout << "DRUMSYNTH -- saving new preset '" << new_preset_name << "'"
            << std::endl;
  if (new_preset_name.empty()) {
    std::cerr << "Play tha game, pal, need a name to save yer synth settings "
              << std::endl;
    return;
  }
  settings_.name = new_preset_name;

  std::ofstream presetzzz;
  const std::string kSEP = "::";
  presetzzz.open(DRUM_PRESET_FILENAME, std::ios::app);
  presetzzz << "name=" << settings_.name << kSEP;
  presetzzz << "volume=" << settings_.volume << kSEP;
  presetzzz << "bd_vol=" << settings_.bd_vol << kSEP;
  presetzzz << "bd_pan=" << settings_.bd_pan << kSEP;
  presetzzz << "bd_nvol=" << settings_.bd_noise_vol << kSEP;
  presetzzz << "bd_octave=" << settings_.bd_octave << kSEP;
  presetzzz << "bd_key=" << settings_.bd_key << kSEP;
  presetzzz << "bd_detune=" << settings_.bd_detune_cents << kSEP;
  presetzzz << "bd_hard_sync=" << settings_.bd_hard_sync << kSEP;
  presetzzz << "bd_tone=" << settings_.bd_tone << kSEP;
  presetzzz << "bd_q=" << settings_.bd_q << kSEP;
  presetzzz << "bd_ntone=" << settings_.bd_ntone << kSEP;
  presetzzz << "bd_nq=" << settings_.bd_nq << kSEP;
  presetzzz << "bd_decay=" << settings_.bd_decay << kSEP;
  presetzzz << "bd_dist=" << settings_.bd_distortion_threshold << kSEP;
  presetzzz << std::endl;
  presetzzz.close();

  std::cout << "DRUMSYNTH -- saving -- DONE" << std::endl;
}

void DrumSynth::Update() {
  volume = settings_.volume;
  bd_->dca_.m_amplitude_control = settings_.bd_vol;
  bd_->dca_.m_pan_control = settings_.bd_pan;
  bd_->noise_->m_amplitude = settings_.bd_noise_vol;
  bd_->out_filter_->SetFcControl(settings_.bd_tone);
  bd_->out_filter_->SetQControl(settings_.bd_q);
  bd_->noise_filter_->SetFcControl(settings_.bd_ntone);
  bd_->noise_filter_->SetQControl(settings_.bd_nq);
  bd_->eg_.SetDecayTimeMsec(settings_.bd_decay);
  bd_->frequency_ =
      Midi2Freq((settings_.bd_octave + 1) * 12 + (settings_.bd_key % 12));
  bd_->osc1_->m_cents = settings_.bd_detune_cents;
  bd_->osc2_->m_cents = -(settings_.bd_detune_cents);
  bd_->hard_sync_ = settings_.bd_hard_sync;
  bd_->distortion_.SetParam("threshold", settings_.bd_distortion_threshold);
}

void DrumSynth::LoadSettings(DrumSettings settings) {
  settings_ = settings;
  Update();
}

void DrumSynth::PrintSettings(DrumSettings settingz) {
  std::cout << "AAIIGht, settings for" << settingz.name << std::endl;
}

void DrumSynth::Randomize() {}

void DrumSynth::LoadPreset(std::string name,
                           std::map<std::string, double> preset_vals) {
  DrumSettings settings = Map2DrumSettings(name, preset_vals);
  LoadSettings(settings);
}

DrumSettings Map2DrumSettings(std::string name,
                              std::map<std::string, double> &preset_vals) {
  DrumSettings preset;
  preset.name = name;

  for (auto &[key, dval] : preset_vals) {
    // std::cout << "KEY:" << key << " VAL:" << dval << std::endl;
    if (key == "volume")
      preset.volume = dval;
    else if (key == "bd_vol")
      preset.bd_vol = dval;
    else if (key == "bd_pan")
      preset.bd_pan = dval;
    else if (key == "bd_nvol")
      preset.bd_noise_vol = dval;
    else if (key == "bd_octave")
      preset.bd_octave = dval;
    else if (key == "bd_key")
      preset.bd_key = dval;
    else if (key == "bd_detune")
      preset.bd_detune_cents = dval;
    else if (key == "bd_hard_sync")
      preset.bd_hard_sync = dval;
    else if (key == "bd_tone")
      preset.bd_tone = dval;
    else if (key == "bd_q")
      preset.bd_q = dval;
    else if (key == "bd_ntone")
      preset.bd_ntone = dval;
    else if (key == "bd_nq")
      preset.bd_nq = dval;
    else if (key == "bd_decay")
      preset.bd_decay = dval;
    else if (key == "bd_dist")
      preset.bd_distortion_threshold = dval;
  }

  return preset;
}

}  // namespace SBAudio
