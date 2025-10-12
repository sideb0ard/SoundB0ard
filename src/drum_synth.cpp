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
  sd_ = std::make_unique<SnareDrum>();
  hh_ = std::make_unique<HiHat>();
  hh2_ = std::make_unique<HiHat>();
  cp_ = std::make_unique<HandClap>();
  fm1_ = std::make_unique<FMDrum>();
  fm2_ = std::make_unique<FMDrum>();
  fm3_ = std::make_unique<FMDrum>();
  lz_ = std::make_unique<Lazer>();

  LoadSettings(DrumSettings());
  active = true;
}

StereoVal DrumSynth::GenNext(mixer_timing_info tinfo) {
  StereoVal out = {.left = 0, .right = 0};
  if (!active) return out;

  auto bd_out = bd_->Generate();
  out.left += bd_out.left;
  out.right += bd_out.right;

  auto hh_out = hh_->Generate();
  out.left += hh_out.left;
  out.right += hh_out.right;

  auto hh2_out = hh2_->Generate();
  out.left += hh2_out.left;
  out.right += hh2_out.right;

  auto snare_out = sd_->Generate();
  out.left += snare_out.left;
  out.right += snare_out.right;

  auto clap_out = cp_->Generate();
  out.left += clap_out.left;
  out.right += clap_out.right;

  auto fm1_out = fm1_->Generate();
  out.left += fm1_out.left;
  out.right += fm1_out.right;

  auto fm2_out = fm2_->Generate();
  out.left += fm2_out.left;
  out.right += fm2_out.right;

  auto fm3_out = fm3_->Generate();
  out.left += fm3_out.left;
  out.right += fm3_out.right;

  auto lz_out = lz_->Generate();
  out.left += lz_out.left;
  out.right += lz_out.right;

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
    settings_.bd_ntone = val;
  else if (name == "bd_nq")
    settings_.bd_q = val;
  else if (name == "bd_decay")
    settings_.bd_decay = val;
  else if (name == "bd_octave")
    settings_.bd_octave = val;
  else if (name == "bd_key") {
    settings_.bd_key = val;
  } else if (name == "bd_detune")
    settings_.bd_detune_cents = val;
  else if (name == "bd_hard_sync")
    settings_.bd_hard_sync = val;
  else if (name == "bd_dist_en") {
    settings_.bd_use_distortion = val;
  } else if (name == "bd_dist") {
    settings_.bd_distortion_threshold = val;
  } else if (name == "bd_delay_en")
    settings_.bd_use_delay = val;
  else if (name == "bd_delay_mode")
    settings_.bd_delay_mode = val;
  else if (name == "bd_delay_ms")
    settings_.bd_delay_ms = val;
  else if (name == "bd_delay_feedback_pct")
    settings_.bd_delay_feedback_pct = val;
  else if (name == "bd_delay_ratio")
    settings_.bd_distortion_threshold = val;
  else if (name == "bd_delay_wetmix")
    settings_.bd_distortion_threshold = val;
  else if (name == "bd_delay_sync_tempo")
    settings_.bd_delay_sync_tempo = val;
  else if (name == "bd_delay_sync_len")
    settings_.bd_delay_sync_len = val;

  else if (name == "hh_vol")
    settings_.hh_vol = val;
  else if (name == "hh_pan")
    settings_.hh_pan = val;
  else if (name == "hh_attack")
    settings_.hh_attack = val;
  else if (name == "hh_decay")
    settings_.hh_decay = val;
  else if (name == "hh_sqamp")
    settings_.hh_sqamp = val;
  else if (name == "hh_midf")
    settings_.hh_midf = val;
  else if (name == "hh_hif")
    settings_.hh_hif = val;
  else if (name == "hh_hif_q")
    settings_.hh_hif_q = val;
  else if (name == "hh_dist")
    settings_.hh_distortion_threshold = val;
  else if (name == "hh_delay_mode")
    settings_.hh_delay_mode = val;
  else if (name == "hh_delay_ms")
    settings_.hh_delay_ms = val;
  else if (name == "hh_delay_feedback_pct")
    settings_.hh_delay_feedback_pct = val;
  else if (name == "hh_delay_ratio")
    settings_.hh_distortion_threshold = val;
  else if (name == "hh_delay_wetmix")
    settings_.hh_distortion_threshold = val;
  else if (name == "hh_delay_sync_tempo")
    settings_.hh_delay_sync_tempo = val;
  else if (name == "hh_delay_sync_len")
    settings_.hh_delay_sync_len = val;

  else if (name == "hh2_vol")
    settings_.hh2_vol = val;
  else if (name == "hh2_pan")
    settings_.hh2_pan = val;
  else if (name == "hh2_attack")
    settings_.hh2_attack = val;
  else if (name == "hh2_decay")
    settings_.hh2_decay = val;
  else if (name == "hh2_sqamp")
    settings_.hh2_sqamp = val;
  else if (name == "hh2_midf")
    settings_.hh2_midf = val;
  else if (name == "hh2_hif")
    settings_.hh2_hif = val;
  else if (name == "hh2_hif_q")
    settings_.hh2_hif_q = val;
  else if (name == "hh2_dist")
    settings_.hh2_distortion_threshold = val;
  else if (name == "hh2_delay_mode")
    settings_.hh2_delay_mode = val;
  else if (name == "hh2_delay_ms")
    settings_.hh2_delay_ms = val;
  else if (name == "hh2_delay_feedback_pct")
    settings_.hh2_delay_feedback_pct = val;
  else if (name == "hh2_delay_ratio")
    settings_.hh2_distortion_threshold = val;
  else if (name == "hh2_delay_wetmix")
    settings_.hh2_distortion_threshold = val;
  else if (name == "hh2_delay_sync_tempo")
    settings_.hh2_delay_sync_tempo = val;
  else if (name == "hh2_delay_sync_len")
    settings_.hh2_delay_sync_len = val;

  else if (name == "sd_vol")
    settings_.sd_vol = val;
  else if (name == "sd_pan")
    settings_.sd_pan = val;
  else if (name == "sd_nvol")
    settings_.sd_noise_vol = val;
  else if (name == "sd_noise_decay")
    settings_.sd_noise_decay = val;
  else if (name == "sd_tone")
    settings_.sd_tone = val;
  else if (name == "sd_decay")
    settings_.sd_decay = val;
  else if (name == "sd_octave")
    settings_.sd_octave = val;
  else if (name == "sd_key")
    settings_.sd_key = val;
  else if (name == "sd_lo_osc_wav")
    settings_.sd_lo_osc_waveform = val;
  else if (name == "sd_hi_osc_wav")
    settings_.sd_hi_osc_waveform = val;
  else if (name == "sd_dist")
    settings_.sd_distortion_threshold = val;
  else if (name == "sd_delay_en")
    settings_.sd_use_delay = val;
  else if (name == "sd_delay_mode")
    settings_.sd_delay_mode = val;
  else if (name == "sd_delay_ms")
    settings_.sd_delay_ms = val;
  else if (name == "sd_delay_feedback_pct")
    settings_.sd_delay_feedback_pct = val;
  else if (name == "sd_delay_ratio")
    settings_.sd_distortion_threshold = val;
  else if (name == "sd_delay_wetmix")
    settings_.sd_distortion_threshold = val;
  else if (name == "sd_delay_sync_tempo")
    settings_.sd_delay_sync_tempo = val;
  else if (name == "sd_delay_sync_len")
    settings_.sd_delay_sync_len = val;

  else if (name == "cp_vol")
    settings_.cp_vol = val;
  else if (name == "cp_pan")
    settings_.cp_pan = val;
  else if (name == "cp_nvol")
    settings_.cp_nvol = val;
  else if (name == "cp_nattack") {
    settings_.cp_nattack = val;
  } else if (name == "cp_ndecay")
    settings_.cp_ndecay = val;
  else if (name == "cp_tone")
    settings_.cp_tone = val;
  else if (name == "cp_fq")
    settings_.cp_fq = val;
  else if (name == "cp_eg_attack")
    settings_.cp_eg_attack = val;
  else if (name == "cp_eg_decay")
    settings_.cp_eg_decay = val;
  else if (name == "cp_eg_sustain")
    settings_.cp_eg_sustain = val;
  else if (name == "cp_eg_release")
    settings_.cp_eg_release = val;
  else if (name == "cp_lfo_type")
    settings_.cp_lfo_type = val;
  else if (name == "cp_lfo_rate")
    settings_.cp_lfo_rate = val;
  else if (name == "cp_dist")
    settings_.cp_distortion_threshold = val;
  else if (name == "cp_delay_mode")
    settings_.cp_delay_mode = val;
  else if (name == "cp_delay_ms")
    settings_.cp_delay_ms = val;
  else if (name == "cp_delay_feedback_pct")
    settings_.cp_delay_feedback_pct = val;
  else if (name == "cp_delay_ratio")
    settings_.cp_distortion_threshold = val;
  else if (name == "cp_delay_wetmix")
    settings_.cp_distortion_threshold = val;
  else if (name == "cp_delay_sync_tempo")
    settings_.cp_delay_sync_tempo = val;
  else if (name == "cp_delay_sync_len")
    settings_.cp_delay_sync_len = val;

  else if (name == "fm1_vol")
    settings_.fm1_vol = val;
  else if (name == "fm1_pan")
    settings_.fm1_pan = val;
  else if (name == "fm1_car_freq")
    settings_.fm1_carrier_freq = val;
  else if (name == "fm1_mod_freq_rat")
    settings_.fm1_modulator_freq_ratio = val;
  else if (name == "fm1_car_eg_attack")
    settings_.fm1_carrier_eg_attack = val;
  else if (name == "fm1_car_eg_decay")
    settings_.fm1_carrier_eg_decay = val;
  else if (name == "fm1_car_eg_sustain")
    settings_.fm1_carrier_eg_sustain = val;
  else if (name == "fm1_car_eg_release")
    settings_.fm1_carrier_eg_release = val;
  else if (name == "fm1_mod_eg_attack")
    settings_.fm1_modulator_eg_attack = val;
  else if (name == "fm1_mod_eg_decay")
    settings_.fm1_modulator_eg_decay = val;
  else if (name == "fm1_mod_eg_sustain")
    settings_.fm1_modulator_eg_sustain = val;
  else if (name == "fm1_mod_eg_release")
    settings_.fm1_modulator_eg_release = val;

  else if (name == "fm2_vol")
    settings_.fm2_vol = val;
  else if (name == "fm2_pan")
    settings_.fm2_pan = val;
  else if (name == "fm2_car_freq")
    settings_.fm2_carrier_freq = val;
  else if (name == "fm2_mod_freq_rat")
    settings_.fm2_modulator_freq_ratio = val;
  else if (name == "fm2_car_eg_attack")
    settings_.fm2_carrier_eg_attack = val;
  else if (name == "fm2_car_eg_decay")
    settings_.fm2_carrier_eg_decay = val;
  else if (name == "fm2_car_eg_sustain")
    settings_.fm2_carrier_eg_sustain = val;
  else if (name == "fm2_car_eg_release")
    settings_.fm2_carrier_eg_release = val;
  else if (name == "fm2_mod_eg_attack")
    settings_.fm2_modulator_eg_attack = val;
  else if (name == "fm2_mod_eg_decay")
    settings_.fm2_modulator_eg_decay = val;
  else if (name == "fm2_mod_eg_sustain")
    settings_.fm2_modulator_eg_sustain = val;
  else if (name == "fm2_mod_eg_release")
    settings_.fm2_modulator_eg_release = val;

  else if (name == "fm3_vol")
    settings_.fm3_vol = val;
  else if (name == "fm3_pan")
    settings_.fm3_pan = val;
  else if (name == "fm3_car_freq")
    settings_.fm3_carrier_freq = val;
  else if (name == "fm3_mod_freq_rat")
    settings_.fm3_modulator_freq_ratio = val;
  else if (name == "fm3_car_eg_attack")
    settings_.fm3_carrier_eg_attack = val;
  else if (name == "fm3_car_eg_decay")
    settings_.fm3_carrier_eg_decay = val;
  else if (name == "fm3_car_eg_sustain")
    settings_.fm3_carrier_eg_sustain = val;
  else if (name == "fm3_car_eg_release")
    settings_.fm3_carrier_eg_release = val;
  else if (name == "fm3_mod_eg_attack")
    settings_.fm3_modulator_eg_attack = val;
  else if (name == "fm3_mod_eg_decay")
    settings_.fm3_modulator_eg_decay = val;
  else if (name == "fm3_mod_eg_sustain")
    settings_.fm3_modulator_eg_sustain = val;
  else if (name == "fm3_mod_eg_release")
    settings_.fm3_modulator_eg_release = val;

  else if (name == "lz_vol")
    settings_.lz_vol = val;
  else if (name == "lz_pan")
    settings_.lz_pan = val;
  else if (name == "lz_freq")
    settings_.lz_freq = val;
  else if (name == "lz_attack")
    settings_.lz_attack = val;
  else if (name == "lz_decay")
    settings_.lz_decay = val;
  else if (name == "lz_range")
    settings_.lz_osc_range = val;
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
     << " bd_dist_en:" << settings_.bd_use_distortion
     << " bd_dist:" << settings_.bd_distortion_threshold << std::endl;
  ss << "     bd_delay_mode:" << settings_.bd_delay_mode
     << " bd_delay_en:" << settings_.bd_use_delay
     << " bd_delay_ms:" << settings_.bd_delay_ms
     << " bd_delay_feedback_pct:" << settings_.bd_delay_feedback_pct
     << " bd_delay_ratio:" << settings_.bd_delay_ratio << std::endl;
  ss << "     bd_delay_wetmix:" << settings_.bd_delay_wetmix
     << " bd_delay_sync_tempo:" << settings_.bd_delay_sync_tempo
     << " bd_delay_sync_len:" << settings_.bd_delay_sync_len << std::endl;
  ss << COOL_COLOR_ORANGE "     sd(1): sd_vol:" << settings_.sd_vol
     << " sd_pan:" << settings_.sd_pan << " sd_nvol:" << settings_.sd_noise_vol
     << " sd_noise_decay:" << settings_.sd_noise_decay
     << " sd_tone:" << settings_.sd_tone << " sd_decay:" << settings_.sd_decay
     << std::endl;
  ss << "     sd_octave:" << settings_.sd_octave
     << " sd_key:" << settings_.sd_key
     << " sd_lo_osc_wav:" << settings_.sd_lo_osc_waveform
     << " sd_hi_osc_wav:" << settings_.sd_hi_osc_waveform
     << " sd_dist:" << settings_.sd_distortion_threshold << std::endl;
  ss << "     sd_delay_mode:" << settings_.sd_delay_mode
     << " sd_delay_en:" << settings_.sd_use_delay
     << " sd_delay_ms:" << settings_.sd_delay_ms
     << " sd_delay_feedback_pct:" << settings_.sd_delay_feedback_pct
     << " sd_delay_ratio:" << settings_.sd_delay_ratio << std::endl;
  ss << "     sd_delay_wetmix:" << settings_.sd_delay_wetmix
     << " sd_delay_sync_tempo:" << settings_.sd_delay_sync_tempo
     << " sd_delay_sync_len:" << settings_.sd_delay_sync_len << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW "     hh(2): hh_vol:" << settings_.bd_vol
     << " hh_pan:" << settings_.hh_pan << " hh_attack:" << settings_.hh_attack
     << " hh_decay:" << settings_.hh_decay << std::endl;
  ss << "     hh_sqamp:" << settings_.hh_sqamp
     << " hh_midf:" << settings_.hh_midf << " hh_hif:" << settings_.hh_hif
     << " hh_hif_q:" << settings_.hh_hif_q
     << " hh_dist:" << settings_.hh_distortion_threshold << std::endl;
  ss << "     hh_delay_mode:" << settings_.hh_delay_mode
     << " hh_delay_ms:" << settings_.hh_delay_ms
     << " hh_delay_feedback_pct:" << settings_.hh_delay_feedback_pct
     << " hh_delay_ratio:" << settings_.hh_delay_ratio << std::endl;
  ss << "     hh_delay_wetmix:" << settings_.hh_delay_wetmix
     << " hh_delay_sync_tempo:" << settings_.hh_delay_sync_tempo
     << " hh_delay_sync_len:" << settings_.hh_delay_sync_len << std::endl;
  ss << COOL_COLOR_ORANGE "     hh2(4): hh2_vol:" << settings_.hh2_vol
     << " hh2_pan:" << settings_.hh2_pan
     << " hh2_attack:" << settings_.hh2_attack
     << " hh2_decay:" << settings_.hh2_decay << std::endl;
  ss << "     hh2_sqamp:" << settings_.hh2_sqamp
     << " hh2_midf:" << settings_.hh2_midf << " hh2_hif:" << settings_.hh2_hif
     << " hh2_hif_q:" << settings_.hh2_hif_q
     << " hh2_dist:" << settings_.hh2_distortion_threshold << std::endl;
  ss << "     hh2_delay_mode:" << settings_.hh2_delay_mode
     << " hh2_delay_ms:" << settings_.hh2_delay_ms
     << " hh2_delay_feedback_pct:" << settings_.hh2_delay_feedback_pct
     << " hh2_delay_ratio:" << settings_.hh2_delay_ratio << std::endl;
  ss << "     hh2_delay_wetmix:" << settings_.hh2_delay_wetmix
     << " hh2_delay_sync_tempo:" << settings_.hh2_delay_sync_tempo
     << " hh2_delay_sync_len:" << settings_.hh2_delay_sync_len << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW "     cp(3): cp_vol:" << settings_.cp_vol
     << " cp_pan:" << settings_.cp_pan << " cp_nvol:" << settings_.cp_nvol
     << " cp_nattack:" << settings_.cp_nattack
     << " cp_ndecay:" << settings_.cp_ndecay << " cp_tone:" << settings_.cp_tone
     << " cp_fq:" << settings_.cp_fq << std::endl;
  ss << "     cp_eg_attack:" << settings_.cp_eg_attack
     << " cp_eg_decay:" << settings_.cp_eg_decay
     << " cp_eg_sustain:" << settings_.cp_eg_sustain
     << " cp_eg_release:" << settings_.cp_eg_release << std::endl;
  ss << "     cp_lfo_type:" << settings_.cp_lfo_type
     << " cp_lfo_rate:" << settings_.cp_lfo_rate
     << " cp_dist:" << settings_.cp_distortion_threshold << std::endl;
  ss << "     cp_delay_mode:" << settings_.cp_delay_mode
     << " cp_delay_ms:" << settings_.cp_delay_ms
     << " cp_delay_feedback_pct:" << settings_.cp_delay_feedback_pct
     << " cp_delay_ratio:" << settings_.cp_delay_ratio << std::endl;
  ss << "     cp_delay_wetmix:" << settings_.cp_delay_wetmix
     << " cp_delay_sync_tempo:" << settings_.cp_delay_sync_tempo
     << " cp_delay_sync_len:" << settings_.cp_delay_sync_len << std::endl;
  ss << COOL_COLOR_ORANGE "     fm1(5): fm1_vol:" << settings_.fm1_vol
     << " fm1_pan:" << settings_.fm1_pan
     << " fm1_car_freq:" << settings_.fm1_carrier_freq
     << " fm1_mod_freq_rat:" << settings_.fm1_modulator_freq_ratio << std::endl;
  ss << "     fm1_car_eg_attack:" << settings_.fm1_carrier_eg_attack
     << " fm1_car_eg_decay:" << settings_.fm1_carrier_eg_decay
     << " fm1_car_eg_sustain:" << settings_.fm1_carrier_eg_sustain
     << " fm1_car_eg_release:" << settings_.fm1_carrier_eg_release << std::endl;
  ss << "     fm1_mod_eg_attack:" << settings_.fm1_modulator_eg_attack
     << " fm1_mod_eg_decay:" << settings_.fm1_modulator_eg_decay
     << " fm1_mod_eg_sustain:" << settings_.fm1_modulator_eg_sustain
     << " fm1_mod_eg_release:" << settings_.fm1_modulator_eg_release
     << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW "     fm2(6): fm2_vol:" << settings_.fm2_vol
     << " fm2_pan:" << settings_.fm2_pan
     << " fm2_car_freq:" << settings_.fm2_carrier_freq
     << " fm2_mod_freq_rat:" << settings_.fm2_modulator_freq_ratio << std::endl;
  ss << "     fm2_car_eg_attack:" << settings_.fm2_carrier_eg_attack
     << " fm2_car_eg_decay:" << settings_.fm2_carrier_eg_decay
     << " fm2_car_eg_sustain:" << settings_.fm2_carrier_eg_sustain
     << " fm2_car_eg_release:" << settings_.fm2_carrier_eg_release << std::endl;
  ss << "     fm2_mod_eg_attack:" << settings_.fm2_modulator_eg_attack
     << " fm2_mod_eg_decay:" << settings_.fm2_modulator_eg_decay
     << " fm2_mod_eg_sustain:" << settings_.fm2_modulator_eg_sustain
     << " fm2_mod_eg_release:" << settings_.fm2_modulator_eg_release
     << std::endl;
  ss << COOL_COLOR_ORANGE "     fm3(7): fm3_vol:" << settings_.fm3_vol
     << " fm3_pan:" << settings_.fm3_pan
     << " fm3_car_freq:" << settings_.fm3_carrier_freq
     << " fm3_mod_freq_rat:" << settings_.fm3_modulator_freq_ratio << std::endl;
  ss << "     fm3_car_eg_attack:" << settings_.fm3_carrier_eg_attack
     << " fm3_car_eg_decay:" << settings_.fm3_carrier_eg_decay
     << " fm3_car_eg_sustain:" << settings_.fm3_carrier_eg_sustain
     << " fm3_car_eg_release:" << settings_.fm3_carrier_eg_release << std::endl;
  ss << "     fm3_mod_eg_attack:" << settings_.fm3_modulator_eg_attack
     << " fm3_mod_eg_decay:" << settings_.fm3_modulator_eg_decay
     << " fm3_mod_eg_sustain:" << settings_.fm3_modulator_eg_sustain
     << " fm3_mod_eg_release:" << settings_.fm3_modulator_eg_release
     << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW "     lz(8): lz_vol:" << settings_.lz_vol
     << " lz_pan:" << settings_.lz_pan << " lz_freq:" << settings_.lz_freq
     << " lz_attack:" << settings_.lz_attack
     << " lz_decay:" << settings_.lz_decay
     << " lz_range:" << settings_.lz_osc_range << std::endl;

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

  double velocity = scaleybum(0, 127, 0, 1, ev.data2);

  switch (drum_module_num) {
    case (0):
      // Bass Drum
      bd_->NoteOn(velocity);
      break;
    case (1):
      // Snare Drum
      sd_->NoteOn(velocity);
      break;
    case (2):
      // Hi Hat
      hh_->NoteOn(velocity);
      break;
    case (3):
      // HandClap
      cp_->NoteOn(velocity);
      break;
    case (4):
      // Hi Hat 2 // Open Hat
      hh2_->NoteOn(velocity);
      break;
    case (5):
      // FM Drum 1
      fm1_->NoteOn(velocity);
      break;
    case (6):
      // FM Drum 2
      fm2_->NoteOn(velocity);
      break;
    case (7):
      // FM Drum 3
      fm3_->NoteOn(velocity);
      break;
    case (8):
      // Lazer
      lz_->NoteOn(velocity);
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
  presetzzz << "bd_tone=" << settings_.bd_tone << kSEP;
  presetzzz << "bd_q=" << settings_.bd_q << kSEP;
  presetzzz << "bd_noise_vol=" << settings_.bd_noise_vol << kSEP;
  presetzzz << "bd_ntone=" << settings_.bd_ntone << kSEP;
  presetzzz << "bd_nq=" << settings_.bd_nq << kSEP;
  presetzzz << "bd_decay=" << settings_.bd_decay << kSEP;
  presetzzz << "bd_octave=" << settings_.bd_octave << kSEP;
  presetzzz << "bd_key=" << settings_.bd_key << kSEP;
  presetzzz << "bd_detune_cents=" << settings_.bd_detune_cents << kSEP;
  presetzzz << "bd_use_distortion=" << settings_.bd_use_distortion << kSEP;
  presetzzz << "bd_distortion_threshold=" << settings_.bd_distortion_threshold
            << kSEP;
  presetzzz << "bd_hard_sync=" << settings_.bd_hard_sync << kSEP;
  presetzzz << "bd_use_delay=" << settings_.bd_use_delay << kSEP;
  presetzzz << "bd_delay_mode=" << settings_.bd_delay_mode << kSEP;
  presetzzz << "bd_delay_ms=" << settings_.bd_delay_ms << kSEP;
  presetzzz << "bd_delay_feedback_pct=" << settings_.bd_delay_feedback_pct
            << kSEP;
  presetzzz << "bd_delay_ratio=" << settings_.bd_delay_ratio << kSEP;
  presetzzz << "bd_delay_wetmix=" << settings_.bd_delay_wetmix << kSEP;
  presetzzz << "bd_delay_sync_tempo=" << settings_.bd_delay_sync_tempo << kSEP;
  presetzzz << "bd_delay_sync_len=" << settings_.bd_delay_sync_len << kSEP;

  // 1 - SnareDum Settings
  presetzzz << "sd_vol=" << settings_.sd_vol << kSEP;
  presetzzz << "sd_pan=" << settings_.sd_pan << kSEP;
  presetzzz << "sd_use_delay=" << settings_.sd_use_delay << kSEP;
  presetzzz << "sd_noise_vol=" << settings_.sd_noise_vol << kSEP;
  presetzzz << "sd_noise_decay=" << settings_.sd_noise_decay << kSEP;
  presetzzz << "sd_tone=" << settings_.sd_tone << kSEP;
  presetzzz << "sd_decay=" << settings_.sd_decay << kSEP;
  presetzzz << "sd_octave=" << settings_.sd_octave << kSEP;
  presetzzz << "sd_key=" << settings_.sd_key << kSEP;
  presetzzz << "sd_hi_osc_waveform=" << settings_.sd_hi_osc_waveform << kSEP;
  presetzzz << "sd_lo_osc_waveform=" << settings_.sd_lo_osc_waveform << kSEP;
  presetzzz << "sd_distortion_threshold=" << settings_.sd_distortion_threshold
            << kSEP;
  presetzzz << "sd_delay_mode=" << settings_.sd_delay_mode << kSEP;
  presetzzz << "sd_delay_ms=" << settings_.sd_delay_ms << kSEP;
  presetzzz << "sd_delay_feedback_pct=" << settings_.sd_delay_feedback_pct
            << kSEP;
  presetzzz << "sd_delay_ratio=" << settings_.sd_delay_ratio << kSEP;
  presetzzz << "sd_delay_wetmix=" << settings_.sd_delay_wetmix << kSEP;
  presetzzz << "sd_delay_sync_tempo=" << settings_.sd_delay_sync_tempo << kSEP;
  presetzzz << "sd_delay_sync_len=" << settings_.sd_delay_sync_len << kSEP;

  // 2 - Closed hat Settings
  presetzzz << "hh_vol=" << settings_.hh_vol << kSEP;
  presetzzz << "hh_pan=" << settings_.hh_pan << kSEP;
  presetzzz << "hh_sqamp=" << settings_.hh_sqamp << kSEP;
  presetzzz << "hh_attack=" << settings_.hh_attack << kSEP;
  presetzzz << "hh_decay=" << settings_.hh_decay << kSEP;
  presetzzz << "hh_midf=" << settings_.hh_midf << kSEP;
  presetzzz << "hh_hif=" << settings_.hh_hif << kSEP;
  presetzzz << "hh_hif_q=" << settings_.hh_hif_q << kSEP;
  presetzzz << "hh_distortion_threshold=" << settings_.hh_distortion_threshold
            << kSEP;
  presetzzz << "hh_delay_mode=" << settings_.hh_delay_mode << kSEP;
  presetzzz << "hh_delay_ms=" << settings_.hh_delay_ms << kSEP;
  presetzzz << "hh_delay_feedback_pct=" << settings_.hh_delay_feedback_pct
            << kSEP;
  presetzzz << "hh_delay_ratio=" << settings_.hh_delay_ratio << kSEP;
  presetzzz << "hh_delay_wetmix=" << settings_.hh_delay_wetmix << kSEP;
  presetzzz << "hh_delay_sync_tempo=" << settings_.hh_delay_sync_tempo << kSEP;
  presetzzz << "hh_delay_sync_len=" << settings_.hh_delay_sync_len << kSEP;

  // 3 - Clap
  presetzzz << "cp_vol=" << settings_.cp_vol << kSEP;
  presetzzz << "cp_pan=" << settings_.cp_pan << kSEP;
  presetzzz << "cp_nvol=" << settings_.cp_nvol << kSEP;
  presetzzz << "cp_nattack=" << settings_.cp_nattack << kSEP;
  presetzzz << "cp_ndecay=" << settings_.cp_ndecay << kSEP;
  presetzzz << "cp_tone=" << settings_.cp_tone << kSEP;
  presetzzz << "cp_fq=" << settings_.cp_fq << kSEP;
  presetzzz << "cp_eg_attack=" << settings_.cp_eg_attack << kSEP;
  presetzzz << "cp_eg_decay=" << settings_.cp_eg_decay << kSEP;
  presetzzz << "cp_eg_sustain=" << settings_.cp_eg_sustain << kSEP;
  presetzzz << "cp_eg_release=" << settings_.cp_eg_release << kSEP;
  presetzzz << "cp_lfo_type=" << settings_.cp_lfo_type << kSEP;
  presetzzz << "cp_lfo_rate=" << settings_.cp_lfo_rate << kSEP;
  presetzzz << "cp_distortion_threshold=" << settings_.cp_distortion_threshold
            << kSEP;
  presetzzz << "cp_delay_mode=" << settings_.cp_delay_mode << kSEP;
  presetzzz << "cp_delay_ms=" << settings_.cp_delay_ms << kSEP;
  presetzzz << "cp_delay_feedback_pct=" << settings_.cp_delay_feedback_pct
            << kSEP;
  presetzzz << "cp_delay_ratio=" << settings_.cp_delay_ratio << kSEP;
  presetzzz << "cp_delay_wetmix=" << settings_.cp_delay_wetmix << kSEP;
  presetzzz << "cp_delay_sync_tempo=" << settings_.cp_delay_sync_tempo << kSEP;
  presetzzz << "cp_delay_sync_len=" << settings_.cp_delay_sync_len << kSEP;

  // 4 - Open hat Settings
  presetzzz << "hh2_vol=" << settings_.hh2_vol << kSEP;
  presetzzz << "hh2_pan=" << settings_.hh2_pan << kSEP;
  presetzzz << "hh2_sqamp=" << settings_.hh2_sqamp << kSEP;
  presetzzz << "hh2_attack=" << settings_.hh2_attack << kSEP;
  presetzzz << "hh2_decay=" << settings_.hh2_decay << kSEP;
  presetzzz << "hh2_midf=" << settings_.hh2_midf << kSEP;
  presetzzz << "hh2_hif=" << settings_.hh2_hif << kSEP;
  presetzzz << "hh2_hif_q=" << settings_.hh2_hif_q << kSEP;
  presetzzz << "hh2_distortion_threshold=" << settings_.hh2_distortion_threshold
            << kSEP;
  presetzzz << "hh2_delay_mode=" << settings_.hh2_delay_mode << kSEP;
  presetzzz << "hh2_delay_ms=" << settings_.hh2_delay_ms << kSEP;
  presetzzz << "hh2_delay_feedback_pct=" << settings_.hh2_delay_feedback_pct
            << kSEP;
  presetzzz << "hh2_delay_ratio=" << settings_.hh2_delay_ratio << kSEP;
  presetzzz << "hh2_delay_wetmix=" << settings_.hh2_delay_wetmix << kSEP;
  presetzzz << "hh2_delay_sync_tempo=" << settings_.hh2_delay_sync_tempo
            << kSEP;
  presetzzz << "hh2_delay_sync_len=" << settings_.hh2_delay_sync_len << kSEP;

  // 5 - FM Drum
  presetzzz << "fm1_vol=" << settings_.fm1_vol << kSEP;
  presetzzz << "fm1_pan=" << settings_.fm1_pan << kSEP;
  presetzzz << "fm1_car_freq=" << settings_.fm1_carrier_freq << kSEP;
  presetzzz << "fm1_mod_freq_rat=" << settings_.fm1_modulator_freq_ratio
            << kSEP;
  presetzzz << "fm1_car_eg_attack=" << settings_.fm1_carrier_eg_attack << kSEP;
  presetzzz << "fm1_car_eg_decay=" << settings_.fm1_carrier_eg_decay << kSEP;
  presetzzz << "fm1_car_eg_sustain=" << settings_.fm1_carrier_eg_sustain
            << kSEP;
  presetzzz << "fm1_car_eg_release=" << settings_.fm1_carrier_eg_release
            << kSEP;
  presetzzz << "fm1_mod_eg_attack=" << settings_.fm1_modulator_eg_attack
            << kSEP;
  presetzzz << "fm1_mod_eg_decay=" << settings_.fm1_modulator_eg_decay << kSEP;
  presetzzz << "fm1_mod_eg_sustain=" << settings_.fm1_modulator_eg_sustain
            << kSEP;
  presetzzz << "fm1_mod_eg_release=" << settings_.fm1_modulator_eg_release
            << kSEP;
  // 6 - FM Drum
  presetzzz << "fm2_vol=" << settings_.fm2_vol << kSEP;
  presetzzz << "fm2_pan=" << settings_.fm2_pan << kSEP;
  presetzzz << "fm2_car_freq=" << settings_.fm2_carrier_freq << kSEP;
  presetzzz << "fm2_mod_freq_rat=" << settings_.fm2_modulator_freq_ratio
            << kSEP;
  presetzzz << "fm2_car_eg_attack=" << settings_.fm2_carrier_eg_attack << kSEP;
  presetzzz << "fm2_car_eg_decay=" << settings_.fm2_carrier_eg_decay << kSEP;
  presetzzz << "fm2_car_eg_sustain=" << settings_.fm2_carrier_eg_sustain
            << kSEP;
  presetzzz << "fm2_car_eg_release=" << settings_.fm2_carrier_eg_release
            << kSEP;
  presetzzz << "fm2_mod_eg_attack=" << settings_.fm2_modulator_eg_attack
            << kSEP;
  presetzzz << "fm2_mod_eg_decay=" << settings_.fm2_modulator_eg_decay << kSEP;
  presetzzz << "fm2_mod_eg_sustain=" << settings_.fm2_modulator_eg_sustain
            << kSEP;
  presetzzz << "fm2_mod_eg_release=" << settings_.fm2_modulator_eg_release
            << kSEP;
  // 7 - FM Drum
  presetzzz << "fm3_vol=" << settings_.fm3_vol << kSEP;
  presetzzz << "fm3_pan=" << settings_.fm3_pan << kSEP;
  presetzzz << "fm3_car_freq=" << settings_.fm3_carrier_freq << kSEP;
  presetzzz << "fm3_mod_freq_rat=" << settings_.fm3_modulator_freq_ratio
            << kSEP;
  presetzzz << "fm3_car_eg_attack=" << settings_.fm3_carrier_eg_attack << kSEP;
  presetzzz << "fm3_car_eg_decay=" << settings_.fm3_carrier_eg_decay << kSEP;
  presetzzz << "fm3_car_eg_sustain=" << settings_.fm3_carrier_eg_sustain
            << kSEP;
  presetzzz << "fm3_car_eg_release=" << settings_.fm3_carrier_eg_release
            << kSEP;
  presetzzz << "fm3_mod_eg_attack=" << settings_.fm3_modulator_eg_attack
            << kSEP;
  presetzzz << "fm3_mod_eg_decay=" << settings_.fm3_modulator_eg_decay << kSEP;
  presetzzz << "fm3_mod_eg_sustain=" << settings_.fm3_modulator_eg_sustain
            << kSEP;
  presetzzz << "fm3_mod_eg_release=" << settings_.fm3_modulator_eg_release
            << kSEP;
  // 8 - Lazer
  presetzzz << "lz_vol=" << settings_.lz_vol << kSEP;
  presetzzz << "lz_pan=" << settings_.lz_pan << kSEP;
  presetzzz << "lz_freq=" << settings_.lz_freq << kSEP;
  presetzzz << "lz_attack=" << settings_.lz_attack << kSEP;
  presetzzz << "lz_decay=" << settings_.lz_decay << kSEP;
  presetzzz << "lz_osc_range=" << settings_.lz_decay << kSEP;

  presetzzz << std::endl;
  presetzzz.close();

  std::cout << "DRUMSYNTH -- saving -- DONE" << std::endl;
}

void DrumSynth::Update() {
  volume = settings_.volume;
  bd_->dca_.m_amplitude_control = settings_.bd_vol;
  bd_->dca_.m_pan_control = settings_.bd_pan;
  bd_->noise_->m_amplitude = settings_.bd_noise_vol;
  bd_->noise_enabled_ = settings_.bd_noise_enabled;
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
  bd_->use_distortion_ = settings_.bd_use_distortion;
  bd_->distortion_.SetParam("threshold", settings_.bd_distortion_threshold);
  bd_->use_delay_ = settings_.bd_use_delay;
  bd_->delay_->SetMode(settings_.bd_delay_mode);
  bd_->delay_->SetDelayTimeMs(settings_.bd_delay_ms);
  bd_->delay_->SetFeedbackPercent(settings_.bd_delay_feedback_pct);
  bd_->delay_->SetDelayRatio(settings_.bd_delay_ratio);
  bd_->delay_->SetWetMix(settings_.bd_delay_wetmix);
  bd_->delay_->SetSync(settings_.bd_delay_sync_tempo);
  bd_->delay_->SetSyncLen(settings_.bd_delay_sync_len);

  sd_->dca_.m_amplitude_control = settings_.sd_vol;
  sd_->dca_.m_pan_control = settings_.sd_pan;
  sd_->noise_->m_amplitude = settings_.sd_noise_vol;
  sd_->noise_eg_.SetDecayTimeMsec(settings_.sd_noise_decay);
  sd_->noise_filter_->SetFcControl(settings_.sd_tone);
  sd_->eg_.SetDecayTimeMsec(settings_.sd_decay);
  sd_->lo_osc_->m_waveform = settings_.sd_lo_osc_waveform;
  sd_->lo_osc_->m_osc_fo =
      Midi2Freq((settings_.sd_octave + 1) * 12 + (settings_.sd_key % 12));
  sd_->hi_osc_->m_waveform = settings_.sd_hi_osc_waveform;
  sd_->hi_osc_->m_osc_fo = sd_->lo_osc_->m_osc_fo * 2;
  sd_->distortion_.SetParam("threshold", settings_.sd_distortion_threshold);
  sd_->use_delay_ = settings_.sd_use_delay;
  sd_->delay_->SetMode(settings_.sd_delay_mode);
  sd_->delay_->SetDelayTimeMs(settings_.sd_delay_ms);
  sd_->delay_->SetFeedbackPercent(settings_.sd_delay_feedback_pct);
  sd_->delay_->SetDelayRatio(settings_.sd_delay_ratio);
  sd_->delay_->SetWetMix(settings_.sd_delay_wetmix);
  sd_->delay_->SetSync(settings_.sd_delay_sync_tempo);
  sd_->delay_->SetSyncLen(settings_.sd_delay_sync_len);

  hh_->dca_.m_amplitude_control = settings_.hh_vol;
  hh_->dca_.m_pan_control = settings_.hh_pan;
  hh_->eg_.SetAttackTimeMsec(settings_.hh_decay);
  hh_->eg_.SetDecayTimeMsec(settings_.hh_decay);
  hh_->SetAmplitude(settings_.hh_sqamp);
  hh_->mid_filter_->SetFcControl(settings_.hh_midf);
  hh_->high_filter_->SetFcControl(settings_.hh_hif);
  hh_->high_filter_->SetQControl(settings_.hh_hif_q);
  hh_->distortion_.SetParam("threshold", settings_.hh_distortion_threshold);
  hh_->delay_->SetMode(settings_.hh_delay_mode);
  hh_->delay_->SetDelayTimeMs(settings_.hh_delay_ms);
  hh_->delay_->SetFeedbackPercent(settings_.hh_delay_feedback_pct);
  hh_->delay_->SetDelayRatio(settings_.hh_delay_ratio);
  hh_->delay_->SetWetMix(settings_.hh_delay_wetmix);
  hh_->delay_->SetSync(settings_.hh_delay_sync_tempo);
  hh_->delay_->SetSyncLen(settings_.hh_delay_sync_len);

  hh2_->dca_.m_amplitude_control = settings_.hh2_vol;
  hh2_->dca_.m_pan_control = settings_.hh2_pan;
  hh2_->eg_.SetAttackTimeMsec(settings_.hh2_decay);
  hh2_->eg_.SetDecayTimeMsec(settings_.hh2_decay);
  hh2_->SetAmplitude(settings_.hh2_sqamp);
  hh2_->mid_filter_->SetFcControl(settings_.hh2_midf);
  hh2_->high_filter_->SetFcControl(settings_.hh2_hif);
  hh2_->high_filter_->SetQControl(settings_.hh2_hif_q);
  hh2_->distortion_.SetParam("threshold", settings_.hh2_distortion_threshold);
  hh2_->delay_->SetMode(settings_.hh2_delay_mode);
  hh2_->delay_->SetDelayTimeMs(settings_.hh2_delay_ms);
  hh2_->delay_->SetFeedbackPercent(settings_.hh2_delay_feedback_pct);
  hh2_->delay_->SetDelayRatio(settings_.hh2_delay_ratio);
  hh2_->delay_->SetWetMix(settings_.hh2_delay_wetmix);
  hh2_->delay_->SetSync(settings_.hh2_delay_sync_tempo);
  hh2_->delay_->SetSyncLen(settings_.hh2_delay_sync_len);

  cp_->dca_.m_amplitude_control = settings_.cp_vol;
  cp_->dca_.m_pan_control = settings_.cp_pan;
  cp_->noise_->m_amplitude = settings_.cp_nvol;
  cp_->noise_eg_.SetAttackTimeMsec(settings_.cp_nattack);
  cp_->noise_eg_.SetDecayTimeMsec(settings_.cp_ndecay);
  cp_->noise_filter_->SetFcControl(settings_.cp_tone);
  cp_->noise_filter_->SetQControl(settings_.cp_fq);
  cp_->eg_.SetAttackTimeMsec(settings_.cp_eg_attack);
  cp_->eg_.SetDecayTimeMsec(settings_.cp_eg_decay);
  cp_->eg_.SetSustainLevel(settings_.cp_eg_sustain);
  cp_->eg_.SetReleaseTimeMsec(settings_.cp_eg_release);
  cp_->lfo_->m_waveform = settings_.cp_lfo_type;
  cp_->lfo_->m_osc_fo = settings_.cp_lfo_type;
  cp_->distortion_.SetParam("threshold", settings_.cp_distortion_threshold);
  cp_->delay_->SetMode(settings_.cp_delay_mode);
  cp_->delay_->SetDelayTimeMs(settings_.cp_delay_ms);
  cp_->delay_->SetFeedbackPercent(settings_.cp_delay_feedback_pct);
  cp_->delay_->SetDelayRatio(settings_.cp_delay_ratio);
  cp_->delay_->SetWetMix(settings_.cp_delay_wetmix);
  cp_->delay_->SetSync(settings_.cp_delay_sync_tempo);
  cp_->delay_->SetSyncLen(settings_.cp_delay_sync_len);

  fm1_->dca_.m_amplitude_control = settings_.fm1_vol;
  fm1_->dca_.m_pan_control = settings_.fm1_pan;
  fm1_->carrier_->m_osc_fo = settings_.fm1_carrier_freq;
  fm1_->eg_.SetAttackTimeMsec(settings_.fm1_carrier_eg_attack);
  fm1_->eg_.SetDecayTimeMsec(settings_.fm1_carrier_eg_decay);
  fm1_->eg_.SetSustainLevel(settings_.fm1_carrier_eg_sustain);
  fm1_->eg_.SetReleaseTimeMsec(settings_.fm1_carrier_eg_release);
  //
  fm1_->modulator_->m_osc_fo =
      fm1_->carrier_->m_osc_fo * settings_.fm1_modulator_freq_ratio;
  fm1_->modulator_eg_.SetAttackTimeMsec(settings_.fm1_modulator_eg_attack);
  fm1_->modulator_eg_.SetDecayTimeMsec(settings_.fm1_modulator_eg_decay);
  fm1_->modulator_eg_.SetSustainLevel(settings_.fm1_modulator_eg_sustain);
  fm1_->modulator_eg_.SetReleaseTimeMsec(settings_.fm1_modulator_eg_release);

  fm2_->dca_.m_amplitude_control = settings_.fm2_vol;
  fm2_->dca_.m_pan_control = settings_.fm2_pan;
  fm2_->carrier_->m_osc_fo = settings_.fm2_carrier_freq;
  fm2_->eg_.SetAttackTimeMsec(settings_.fm2_carrier_eg_attack);
  fm2_->eg_.SetDecayTimeMsec(settings_.fm2_carrier_eg_decay);
  fm2_->eg_.SetSustainLevel(settings_.fm2_carrier_eg_sustain);
  fm2_->eg_.SetReleaseTimeMsec(settings_.fm2_carrier_eg_release);
  //
  fm2_->modulator_->m_osc_fo =
      fm2_->carrier_->m_osc_fo * settings_.fm2_modulator_freq_ratio;
  fm2_->modulator_eg_.SetAttackTimeMsec(settings_.fm2_modulator_eg_attack);
  fm2_->modulator_eg_.SetDecayTimeMsec(settings_.fm2_modulator_eg_decay);
  fm2_->modulator_eg_.SetSustainLevel(settings_.fm2_modulator_eg_sustain);
  fm2_->modulator_eg_.SetReleaseTimeMsec(settings_.fm2_modulator_eg_release);

  fm3_->dca_.m_amplitude_control = settings_.fm3_vol;
  fm3_->dca_.m_pan_control = settings_.fm3_pan;
  fm3_->carrier_->m_osc_fo = settings_.fm3_carrier_freq;
  fm3_->eg_.SetAttackTimeMsec(settings_.fm3_carrier_eg_attack);
  fm3_->eg_.SetDecayTimeMsec(settings_.fm3_carrier_eg_decay);
  fm3_->eg_.SetSustainLevel(settings_.fm3_carrier_eg_sustain);
  fm3_->eg_.SetReleaseTimeMsec(settings_.fm3_carrier_eg_release);
  //
  fm3_->modulator_->m_osc_fo =
      fm3_->carrier_->m_osc_fo * settings_.fm3_modulator_freq_ratio;
  fm3_->modulator_eg_.SetAttackTimeMsec(settings_.fm3_modulator_eg_attack);
  fm3_->modulator_eg_.SetDecayTimeMsec(settings_.fm3_modulator_eg_decay);
  fm3_->modulator_eg_.SetSustainLevel(settings_.fm3_modulator_eg_sustain);
  fm3_->modulator_eg_.SetReleaseTimeMsec(settings_.fm3_modulator_eg_release);

  lz_->dca_.m_amplitude_control = settings_.lz_vol;
  lz_->dca_.m_pan_control = settings_.lz_pan;
  lz_->osc1_->m_osc_fo = settings_.lz_freq;
  lz_->pitch_osc_range_ = settings_.lz_osc_range;
  lz_->eg_.SetAttackTimeMsec(settings_.lz_attack);
  lz_->eg_.SetDecayTimeMsec(settings_.lz_decay);
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
    else if (key == "bd_tone")
      preset.bd_tone = dval;
    else if (key == "bd_q")
      preset.bd_q = dval;
    else if (key == "bd_noise_vol")
      preset.bd_noise_vol = dval;
    else if (key == "bd_ntone")
      preset.bd_ntone = dval;
    else if (key == "bd_nq")
      preset.bd_nq = dval;
    else if (key == "bd_decay")
      preset.bd_decay = dval;
    else if (key == "bd_octave")
      preset.bd_octave = dval;
    else if (key == "bd_key")
      preset.bd_key = dval;
    else if (key == "bd_detune_cents")
      preset.bd_detune_cents = dval;
    else if (key == "bd_use_distortion")
      preset.bd_use_distortion = dval;
    else if (key == "bd_distortion_threshold")
      preset.bd_distortion_threshold = dval;
    else if (key == "bd_hard_sync")
      preset.bd_hard_sync = dval;
    else if (key == "bd_use_delay")
      preset.bd_use_delay = dval;
    else if (key == "bd_delay_mode")
      preset.bd_delay_mode = dval;
    else if (key == "bd_delay_ms")
      preset.bd_delay_ms = dval;
    else if (key == "bd_delay_feedback_pct")
      preset.bd_delay_feedback_pct = dval;
    else if (key == "bd_delay_ratio")
      preset.bd_delay_ratio = dval;
    else if (key == "bd_delay_wetmix")
      preset.bd_delay_wetmix = dval;
    else if (key == "bd_delay_sync_tempo")
      preset.bd_delay_sync_tempo = dval;
    else if (key == "bd_delay_sync_len")
      preset.bd_delay_sync_len = dval;

    // // 1 - SnareDum Settings
    else if (key == "sd_vol")
      preset.sd_vol = dval;
    else if (key == "sd_pan")
      preset.sd_pan = dval;
    else if (key == "sd_noise_vol")
      preset.sd_noise_vol = dval;
    else if (key == "sd_noise_decay")
      preset.sd_noise_decay = dval;
    else if (key == "sd_tone")
      preset.sd_tone = dval;
    else if (key == "sd_decay")
      preset.sd_decay = dval;
    else if (key == "sd_octave")
      preset.sd_octave = dval;
    else if (key == "sd_key")
      preset.sd_key = dval;
    else if (key == "sd_hi_osc_waveform")
      preset.sd_hi_osc_waveform = dval;
    else if (key == "sd_lo_osc_waveform")
      preset.sd_lo_osc_waveform = dval;
    else if (key == "sd_distortion_threshold")
      preset.sd_distortion_threshold = dval;
    else if (key == "sd_use_delay")
      preset.sd_use_delay = dval;
    else if (key == "sd_delay_mode")
      preset.sd_delay_mode = dval;
    else if (key == "sd_delay_ms")
      preset.sd_delay_ms = dval;
    else if (key == "sd_delay_feedback_pct")
      preset.sd_delay_feedback_pct = dval;
    else if (key == "sd_delay_ratio")
      preset.sd_delay_ratio = dval;
    else if (key == "sd_delay_wetmix")
      preset.sd_delay_wetmix = dval;
    else if (key == "sd_delay_sync_tempo")
      preset.sd_delay_sync_tempo = dval;
    else if (key == "sd_delay_sync_len")
      preset.sd_delay_sync_len = dval;

    // // 2 - Closed hat Settings
    else if (key == "hh_vol")
      preset.hh_vol = dval;
    else if (key == "hh_pan")
      preset.hh_pan = dval;
    else if (key == "hh_sqamp")
      preset.hh_sqamp = dval;
    else if (key == "hh_attack")
      preset.hh_attack = dval;
    else if (key == "hh_decay")
      preset.hh_decay = dval;
    else if (key == "hh_midf")
      preset.hh_midf = dval;
    else if (key == "hh_hif")
      preset.hh_hif = dval;
    else if (key == "hh_hif_q")
      preset.hh_hif_q = dval;
    else if (key == "hh_distortion_threshold")
      preset.hh_distortion_threshold = dval;
    else if (key == "hh_delay_mode")
      preset.hh_delay_mode = dval;
    else if (key == "hh_delay_ms")
      preset.hh_delay_ms = dval;
    else if (key == "hh_delay_feedback_pct")
      preset.hh_delay_feedback_pct = dval;
    else if (key == "hh_delay_ratio")
      preset.hh_delay_ratio = dval;
    else if (key == "hh_delay_wetmix")
      preset.hh_delay_wetmix = dval;
    else if (key == "hh_delay_sync_tempo")
      preset.hh_delay_sync_tempo = dval;
    else if (key == "hh_delay_sync_len")
      preset.hh_delay_sync_len = dval;

    // // 3 - Clap
    else if (key == "cp_vol")
      preset.cp_vol = dval;
    else if (key == "cp_pan")
      preset.cp_pan = dval;
    else if (key == "cp_nvol")
      preset.cp_nvol = dval;
    else if (key == "cp_nattack")
      preset.cp_nattack = dval;
    else if (key == "cp_ndecay")
      preset.cp_ndecay = dval;
    else if (key == "cp_tone")
      preset.cp_tone = dval;
    else if (key == "cp_fq")
      preset.cp_fq = dval;
    else if (key == "cp_eg_attack")
      preset.cp_eg_attack = dval;
    else if (key == "cp_eg_decay")
      preset.cp_eg_decay = dval;
    else if (key == "cp_eg_sustain")
      preset.cp_eg_sustain = dval;
    else if (key == "cp_eg_release")
      preset.cp_eg_release = dval;
    else if (key == "cp_lfo_type")
      preset.cp_lfo_type = dval;
    else if (key == "cp_lfo_rate")
      preset.cp_lfo_rate = dval;
    else if (key == "cp_distortion_threshold")
      preset.cp_distortion_threshold = dval;
    else if (key == "cp_delay_mode")
      preset.cp_delay_mode = dval;
    else if (key == "cp_delay_ms")
      preset.cp_delay_ms = dval;
    else if (key == "cp_delay_feedback_pct")
      preset.cp_delay_feedback_pct = dval;
    else if (key == "cp_delay_ratio")
      preset.cp_delay_ratio = dval;
    else if (key == "cp_delay_wetmix")
      preset.cp_delay_wetmix = dval;
    else if (key == "cp_delay_sync_tempo")
      preset.cp_delay_sync_tempo = dval;
    else if (key == "cp_delay_sync_len")
      preset.cp_delay_sync_len = dval;

    // // 4 - Open hat Settings
    else if (key == "hh2_vol")
      preset.hh2_vol = dval;
    else if (key == "hh2_pan")
      preset.hh2_pan = dval;
    else if (key == "hh2_sqamp")
      preset.hh2_sqamp = dval;
    else if (key == "hh2_attack")
      preset.hh2_attack = dval;
    else if (key == "hh2_decay")
      preset.hh2_decay = dval;
    else if (key == "hh2_midf")
      preset.hh2_midf = dval;
    else if (key == "hh2_hif")
      preset.hh2_hif = dval;
    else if (key == "hh2_hif_q")
      preset.hh2_hif_q = dval;
    else if (key == "hh2_distortion_threshold")
      preset.hh2_distortion_threshold = dval;
    else if (key == "hh2_delay_mode")
      preset.hh2_delay_mode = dval;
    else if (key == "hh2_delay_ms")
      preset.hh2_delay_ms = dval;
    else if (key == "hh2_delay_feedback_pct")
      preset.hh2_delay_feedback_pct = dval;
    else if (key == "hh2_delay_ratio")
      preset.hh2_delay_ratio = dval;
    else if (key == "hh2_delay_wetmix")
      preset.hh2_delay_wetmix = dval;
    else if (key == "hh2_delay_sync_tempo")
      preset.hh2_delay_sync_tempo = dval;
    else if (key == "hh2_delay_sync_len")
      preset.hh2_delay_sync_len = dval;

    // 5 - FM1
    else if (key == "fm1_vol")
      preset.fm1_vol = dval;
    else if (key == "fm1_pan")
      preset.fm1_pan = dval;
    else if (key == "fm1_car_freq")
      preset.fm1_carrier_freq = dval;
    else if (key == "fm1_car_eg_attack")
      preset.fm1_carrier_eg_attack = dval;
    else if (key == "fm1_car_eg_decay")
      preset.fm1_carrier_eg_decay = dval;
    else if (key == "fm1_car_eg_sustain")
      preset.fm1_carrier_eg_sustain = dval;
    else if (key == "fm1_car_eg_release")
      preset.fm1_carrier_eg_release = dval;
    else if (key == "fm1_mod_freq_rat")
      preset.fm1_modulator_freq_ratio = dval;
    else if (key == "fm1_mod_eg_attack")
      preset.fm1_modulator_eg_attack = dval;
    else if (key == "fm1_mod_eg_decay")
      preset.fm1_modulator_eg_decay = dval;
    else if (key == "fm1_mod_eg_sustain")
      preset.fm1_modulator_eg_sustain = dval;
    else if (key == "fm1_mod_eg_release")
      preset.fm1_modulator_eg_release = dval;
    // 6 - FM2
    else if (key == "fm2_vol")
      preset.fm2_vol = dval;
    else if (key == "fm2_pan")
      preset.fm2_pan = dval;
    else if (key == "fm2_car_freq")
      preset.fm2_carrier_freq = dval;
    else if (key == "fm2_car_eg_attack")
      preset.fm2_carrier_eg_attack = dval;
    else if (key == "fm2_car_eg_decay")
      preset.fm2_carrier_eg_decay = dval;
    else if (key == "fm2_car_eg_sustain")
      preset.fm2_carrier_eg_sustain = dval;
    else if (key == "fm2_car_eg_release")
      preset.fm2_carrier_eg_release = dval;
    else if (key == "fm2_mod_freq_rat")
      preset.fm2_modulator_freq_ratio = dval;
    else if (key == "fm2_mod_eg_attack")
      preset.fm2_modulator_eg_attack = dval;
    else if (key == "fm2_mod_eg_decay")
      preset.fm2_modulator_eg_decay = dval;
    else if (key == "fm2_mod_eg_sustain")
      preset.fm2_modulator_eg_sustain = dval;
    else if (key == "fm2_mod_eg_release")
      preset.fm2_modulator_eg_release = dval;
    // 7 - FM3
    else if (key == "fm3_vol")
      preset.fm3_vol = dval;
    else if (key == "fm3_pan")
      preset.fm3_pan = dval;
    else if (key == "fm3_car_freq")
      preset.fm3_carrier_freq = dval;
    else if (key == "fm3_car_eg_attack")
      preset.fm3_carrier_eg_attack = dval;
    else if (key == "fm3_car_eg_decay")
      preset.fm3_carrier_eg_decay = dval;
    else if (key == "fm3_car_eg_sustain")
      preset.fm3_carrier_eg_sustain = dval;
    else if (key == "fm3_car_eg_release")
      preset.fm3_carrier_eg_release = dval;
    else if (key == "fm3_mod_freq_rat")
      preset.fm3_modulator_freq_ratio = dval;
    else if (key == "fm3_mod_eg_attack")
      preset.fm3_modulator_eg_attack = dval;
    else if (key == "fm3_mod_eg_decay")
      preset.fm3_modulator_eg_decay = dval;
    else if (key == "fm3_mod_eg_sustain")
      preset.fm3_modulator_eg_sustain = dval;
    else if (key == "fm3_mod_eg_release")
      preset.fm3_modulator_eg_release = dval;

    // 8 - Lazer
    else if (key == "lz_vol")
      preset.lz_vol = dval;
    else if (key == "lz_pan")
      preset.lz_pan = dval;
    else if (key == "lz_freq")
      preset.lz_freq = dval;
    else if (key == "lz_attack")
      preset.lz_attack = dval;
    else if (key == "lz_decay")
      preset.lz_decay = dval;
    else if (key == "lz_osc_range")
      preset.lz_osc_range = dval;
  }

  return preset;
}

}  // namespace SBAudio
