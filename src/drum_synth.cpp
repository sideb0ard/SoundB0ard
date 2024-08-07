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
  cp_ = std::make_unique<HandClap>();

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

  auto snare_out = sd_->Generate();
  out.left += snare_out.left;
  out.right += snare_out.right;

  auto clap_out = cp_->Generate();
  out.left += clap_out.left;
  out.right += clap_out.right;

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
  else if (name == "sd_osc1_freq")
    settings_.sd_osc1_freq = val;
  else if (name == "sd_osc2_freq")
    settings_.sd_osc2_freq = val;
  else if (name == "sd_dist")
    settings_.sd_distortion_threshold = val;
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
  else if (name == " cp_nattack")
    settings_.cp_nattack = val;
  else if (name == " cp_ndecay")
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
  ss << "     bd_delay_mode:" << settings_.bd_delay_mode
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
  ss << "     sd_osc1_freq:" << settings_.sd_osc1_freq
     << " sd_osc2_freq:" << settings_.sd_osc2_freq
     << " sd_dist:" << settings_.sd_distortion_threshold << std::endl;
  ss << "     sd_delay_mode:" << settings_.sd_delay_mode
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
  ss << COOL_COLOR_ORANGE "     cp(3): cp_vol:" << settings_.cp_vol
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
  sd_->osc1_->m_osc_fo = settings_.sd_osc1_freq;
  sd_->osc2_->m_osc_fo = -(settings_.sd_osc2_freq);
  sd_->distortion_.SetParam("threshold", settings_.sd_distortion_threshold);
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
