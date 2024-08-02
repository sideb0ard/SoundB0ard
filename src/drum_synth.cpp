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
  // default
  // LoadSettings(DrumSettings());
  bd_ = std::make_unique<BassDrum>();

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
  if (name == "volume") settings_.volume = val;
  Update();
}

std::string DrumSynth::Info() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "DrumZynth - " << COOL_COLOR_PINK2 << settings_.name
     << COOL_COLOR_YELLOW_MELLOW << " - vol:" << volume << " pan:" << pan
     << std::endl;

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
  //  presetzzz << "name=" << settings_.name << kSEP;
  //  presetzzz << "volume=" << settings_.volume << kSEP;
  //  presetzzz << "pan=" << settings_.pan << kSEP;
  //  presetzzz << "distortion_threshold=" << settings_.distortion_threshold
  //            << kSEP;
  //  presetzzz << "hard_sync=" << settings_.hard_sync << kSEP;
  //  presetzzz << "detune_cents=" << settings_.detune_cents << kSEP;
  //  presetzzz << "pulse_width_pct=" << settings_.pulse_width_pct << kSEP;
  //
  //  presetzzz << "noise_amp=" << settings_.noise_amp << kSEP;
  //  presetzzz << "noise_eg_attack_ms=" << settings_.noise_eg_attack_ms <<
  //  kSEP; presetzzz << "noise_eg_decay_ms=" << settings_.noise_eg_decay_ms <<
  //  kSEP; presetzzz << "noise_eg_mode=" << settings_.noise_eg_mode << kSEP;
  //  presetzzz << "noise_filter_type=" << settings_.noise_filter_type << kSEP;
  //  presetzzz << "noise_filter_fc=" << settings_.noise_filter_fc << kSEP;
  //  presetzzz << "noise_filter_q=" << settings_.noise_filter_q << kSEP;
  //
  //  // OSCILLATORS
  //  presetzzz << "osc1_wav=" << settings_.osc1_wav << kSEP;
  //  presetzzz << "osc1_amp=" << settings_.osc1_amp << kSEP;
  //  presetzzz << "osc1_ratio=" << settings_.osc1_ratio << kSEP;
  //  presetzzz << "osc_eg_attack_ms=" << settings_.osc_eg_attack_ms << kSEP;
  //  presetzzz << "osc_eg_decay_ms=" << settings_.osc_eg_decay_ms << kSEP;
  //  presetzzz << "osc_eg_mode=" << settings_.osc_eg_mode << kSEP;
  //
  //  // OUTPUT
  //  presetzzz << "amp_eg_attack_ms=" << settings_.amp_eg_attack_ms << kSEP;
  //  presetzzz << "amp_eg_decay_ms=" << settings_.amp_eg_decay_ms << kSEP;
  //  presetzzz << "amp_eg_mode=" << settings_.amp_eg_mode << kSEP;
  //  presetzzz << "amp_filter_type=" << settings_.amp_filter_type << kSEP;
  //  presetzzz << "amp_filter_fc=" << settings_.amp_filter_fc << kSEP;
  //  presetzzz << "amp_filter_q=" << settings_.amp_filter_q << kSEP;
  //
  presetzzz << std::endl;
  presetzzz.close();

  std::cout << "DRUMSYNTH -- saving -- DONE" << std::endl;
}

void DrumSynth::Update() {
  // GLOBALS
  volume = settings_.volume;
  // distortion_.SetParam("threshold", settings_.distortion_threshold);

  // //// TRANSIENT
  // noise_->m_amplitude = settings_.noise_amp;
  // noise_->Update();
  // noise_filter_->SetType(settings_.noise_filter_type);
  // noise_filter_->SetFcControl(settings_.noise_filter_fc);
  // noise_filter_->SetQControlGUI(settings_.noise_filter_q);
  // noise_filter_->Update();
  // noise_eg_.SetEgMode(settings_.noise_eg_mode);
  // noise_eg_.SetAttackTimeMsec(settings_.noise_eg_attack_ms);
  // noise_eg_.SetDecayTimeMsec(settings_.noise_eg_decay_ms);
  // noise_eg_.Update();

  // // PITCH
  // osc1_->m_waveform = settings_.osc1_wav;
  // osc1_->m_amplitude = settings_.osc1_amp;
  // osc1_->m_fo_ratio = settings_.osc1_ratio;
  // osc1_->m_pulse_width_control = settings_.pulse_width_pct;
  // osc1_->Update();
  // osc_eg_.SetEgMode(settings_.osc_eg_mode);
  // osc_eg_.SetAttackTimeMsec(settings_.osc_eg_attack_ms);
  // osc_eg_.SetDecayTimeMsec(settings_.osc_eg_decay_ms);
  // osc_eg_.Update();

  // // OUTPUT
  // amp_filter_->SetType(settings_.amp_filter_type);
  // amp_filter_->SetFcControl(settings_.amp_filter_fc);
  // amp_filter_->SetQControlGUI(settings_.amp_filter_q);
  // amp_filter_->Update();
  // amp_eg_.SetEgMode(settings_.amp_eg_mode);
  // amp_eg_.SetAttackTimeMsec(settings_.amp_eg_attack_ms);
  // amp_eg_.SetDecayTimeMsec(settings_.amp_eg_decay_ms);
  // amp_eg_.Update();

  // lfo_.m_waveform = settings_.lfo_wave;
  // lfo_.m_lfo_mode = settings_.lfo_mode;
  // lfo_.m_fo = settings_.lfo_rate;
  // lfo_.Update();
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
    if (key == "volume") preset.volume = dval;
    // else if (key == "pan")
    //   preset.pan = dval;
    // else if (key == "hard_sync")
    //   preset.hard_sync = dval;
    // else if (key == "detune_cents")
    //   preset.detune_cents = dval;
    // else if (key == "distortion_threshold")
    //   preset.distortion_threshold = dval;

    // else if (key == "noise_amp")
    //   preset.noise_amp = dval;
    // else if (key == "noise_eg_attack_ms")
    //   preset.noise_eg_attack_ms = dval;
    // else if (key == "noise_eg_decay_ms")
    //   preset.noise_eg_decay_ms = dval;
    // else if (key == "noise_eg_mode")
    //   preset.noise_eg_mode = dval;
    // else if (key == "noise_filter_type")
    //   preset.noise_filter_type = dval;
    // else if (key == "noise_filter_fc")
    //   preset.noise_filter_fc = dval;
    // else if (key == "noise_filter_q")
    //   preset.noise_filter_q = dval;

    // else if (key == "osc1_wav")
    //   preset.osc1_wav = dval;
    // else if (key == "osc1_amp")
    //   preset.osc1_amp = dval;
    // else if (key == "osc1_ratio")
    //   preset.osc1_ratio = dval;
    // else if (key == "osc_eg_attack_ms")
    //   preset.osc_eg_attack_ms = dval;
    // else if (key == "osc_eg_decay_ms")
    //   preset.osc_eg_decay_ms = dval;
    // else if (key == "osc_eg_mode")
    //   preset.osc_eg_mode = dval;

    // else if (key == "amp_eg_attack_ms")
    //   preset.amp_eg_attack_ms = dval;
    // else if (key == "amp_eg_decay_ms")
    //   preset.amp_eg_decay_ms = dval;
    // else if (key == "amp_eg_mode")
    //   preset.amp_eg_mode = dval;
    // else if (key == "amp_filter_type")
    //   preset.amp_filter_type = dval;
    // else if (key == "amp_filter_fc")
    //   preset.amp_filter_fc = dval;
    // else if (key == "amp_filter_q")
    //   preset.amp_filter_q = dval;
  }

  return preset;
}

}  // namespace SBAudio
