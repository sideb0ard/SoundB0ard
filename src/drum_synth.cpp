#include <drum_synth.h>

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
  eg_.SetEgMode(ANALOG);
  eg_.m_output_eg = true;
  eg_.m_reset_to_zero = true;
  dca_.m_mod_source_eg = DEST_DCA_EG;

  // default
  LoadSettings(DrumSettings());

  active = true;
}

StereoVal DrumSynth::GenNext(mixer_timing_info tinfo) {
  StereoVal out = {.left = 0, .right = 0};
  if (!active) return out;

  if (osc1_.m_note_on) {
    lfo_.Update();
    double lfo_out = lfo_.DoOscillate(0);
    double eg_out = eg_.DoEnvelope(nullptr);

    dca_.SetEgMod(eg_out);
    dca_.Update();

    ///////////

    double osc1_mod_val = 0;
    double osc2_mod_val = 0;
    double filter1_fc_mod_val = 0;
    double filter1_q_mod_val = 0;
    double filter2_fc_mod_val = 0;
    double filter2_q_mod_val = 0;

    if (modulations_[ENVI][OSC1_PITCHD] && modulations_[LFOI][OSC1_PITCHD])
      osc1_mod_val = eg_out * lfo_out;
    else
      osc1_mod_val = std::max(eg_out, lfo_out);

    if (modulations_[ENVI][OSC2_PITCHD] && modulations_[LFOI][OSC2_PITCHD])
      osc2_mod_val = eg_out * lfo_out;
    else
      osc2_mod_val = std::max(eg_out, lfo_out);

    if (modulations_[ENVI][FILTER1_FCD] && modulations_[LFOI][FILTER1_FCD])
      filter1_fc_mod_val = eg_out * lfo_out;
    else
      filter1_fc_mod_val = std::max(eg_out, lfo_out);

    if (modulations_[ENVI][FILTER1_QD] && modulations_[LFOI][FILTER1_QD])
      filter1_q_mod_val = eg_out * lfo_out;
    else
      filter1_q_mod_val = std::max(eg_out, lfo_out);

    if (modulations_[ENVI][FILTER2_FCD] && modulations_[LFOI][FILTER2_FCD])
      filter2_fc_mod_val = eg_out * lfo_out;
    else
      filter2_fc_mod_val = std::max(eg_out, lfo_out);

    if (modulations_[ENVI][FILTER2_QD] && modulations_[LFOI][FILTER2_QD])
      filter2_q_mod_val = eg_out * lfo_out;
    else
      filter2_q_mod_val = std::max(eg_out, lfo_out);

    ////////////////////////////////////
    if (filter1_fc_mod_val) {
      filter1_.m_fc_control +=
          (filter1_fc_mod_val * freq_range_) - freq_range_ / 2;
      filter1_.Update();
    }

    if (filter1_q_mod_val) {
      filter1_.m_q_control += (filter1_q_mod_val * q_range_) - q_range_ / 2;
      filter1_.Update();
    }

    if (filter2_fc_mod_val) {
      filter2_.m_fc_control +=
          (filter2_fc_mod_val * freq_range_) - freq_range_ / 2;
      filter2_.Update();
    }

    if (filter2_q_mod_val) {
      filter2_.m_q_control += (filter2_q_mod_val * q_range_) - q_range_ / 2;
      filter2_.Update();
    }

    osc1_.m_osc_fo = base_frequency_ + osc1_mod_val * frequency_diff_;
    osc1_.Update();
    double osc1_out = osc1_.DoOscillate(nullptr) * osc1_amp_;
    if (filter1_en_) {
      osc1_out = filter1_.DoFilter(osc1_out);
    }

    osc2_.m_osc_fo = base_frequency_ + osc2_mod_val * frequency_diff_;
    osc2_.Update();
    double osc2_out = osc2_.DoOscillate(nullptr) * osc2_amp_;
    if (filter2_en_) {
      osc2_out = filter2_.DoFilter(osc2_out);
    }

    //////////////////////////////

    double out_left = 0.0;
    double out_right = 0.0;

    dca_.DoDCA(osc1_out + osc2_out, osc1_out + osc2_out, &out_left, &out_right);

    out = {.left = out_left * volume, .right = out_right * volume};
    out = distortion_.Process(out);
    out = Effector(out);
  }

  if (eg_.GetState() == OFFF) {
    osc1_.StopOscillator();
    osc2_.StopOscillator();
    eg_.StopEg();
    osc1_.m_note_on = false;
  }

  return out;
}

void DrumSynth::SetParam(std::string name, double val) {
  if (name == "distort")
    distortion_.SetParam("threshold", val);
  else if (name == "pitch_range")
    pitch_range_ = val;
  else if (name == "freq_range")
    freq_range_ = val;
  else if (name == "q_range")
    q_range_ = val;

  else if (name == "osc1")
    osc1_.m_waveform = val;
  else if (name == "o1amp")
    osc1_amp_ = val;

  else if (name == "filter1_fc")
    filter1_.SetFcControl(val);
  else if (name == "filter1_q")
    filter1_.SetQControl(val);
  else if (name == "filter1_en")
    filter1_en_ = val;

  else if (name == "osc2")
    osc2_.m_waveform = val;
  else if (name == "o2amp")
    osc2_amp_ = val;

  else if (name == "filter2_fc")
    filter2_.SetFcControl(val);
  else if (name == "filter2_q")
    filter2_.SetQControl(val);
  else if (name == "filter2_en")
    filter2_en_ = val;

  else if (name == "eg_attack")
    eg_.SetAttackTimeMsec(val);
  else if (name == "eg_decay")
    eg_.SetDecayTimeMsec(val);
  else if (name == "eg_sustain")
    eg_.SetSustainLevel(val);
  else if (name == "eg_release")
    eg_.SetReleaseTimeMsec(val);

  else if (name == "lfowav")
    lfo_.m_waveform = val;
  else if (name == "lfomode")
    lfo_.m_lfo_mode = val;
  else if (name == "lforate")
    lfo_.m_osc_fo = val;
  else if (name == "lfoamp")
    lfo_.m_amplitude = val;

  else if (name == "eg_o1_ptch")
    modulations_[ENVI][OSC1_PITCHD] = val;
  else if (name == "eg_o2_ptch")
    modulations_[ENVI][OSC2_PITCHD] = val;
  else if (name == "eg_f1_fc")
    modulations_[ENVI][FILTER1_FCD] = val;
  else if (name == "eg_f1_q")
    modulations_[ENVI][FILTER1_QD] = val;
  else if (name == "eg_f2_fc")
    modulations_[ENVI][FILTER2_FCD] = val;
  else if (name == "eg_f2_q")
    modulations_[ENVI][FILTER2_QD] = val;

  else if (name == "lfo_osc1")
    modulations_[LFOI][OSC1_PITCHD] = val;
  else if (name == "lfo_osc2")
    modulations_[LFOI][OSC2_PITCHD] = val;
  else if (name == "lfo_filter1_fc")
    modulations_[LFOI][FILTER1_FCD] = val;
  else if (name == "lfo_filter1_q")
    modulations_[LFOI][FILTER1_QD] = val;
  else if (name == "lfo_filter2_fc")
    modulations_[LFOI][FILTER2_FCD] = val;
  else if (name == "lfo_filter2_q")
    modulations_[LFOI][FILTER2_QD] = val;

  osc1_.Update();
  osc2_.Update();
  filter1_.Update();
  filter2_.Update();
  eg_.Update();
  lfo_.Update();
}

std::string DrumSynth::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    //    ss << ANSI_COLOR_CYAN;
    ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "DrumZynth - " << COOL_COLOR_PINK2 << patch_name_
     << COOL_COLOR_YELLOW_MELLOW << " - vol:" << volume
     << " distort:" << distortion_.m_threshold_
     << " pitch_range:" << pitch_range_ << " freq_range:" << freq_range_
     << " q_range:" << q_range_ << std::endl;
  ss << "     osc1:" << GetOscType(osc1_.m_waveform) << " o1amp:" << osc1_amp_
     << " filter1_type:" << k_filter_type_names[filter1_.m_filter_type] << "("
     << filter1_.m_filter_type << ")"
     << " filter1_fc:" << filter1_.m_fc << " filter1_q:" << filter1_.m_q
     << " filter1_en:" << filter1_en_ << std::endl;
  ss << "     osc2:" << GetOscType(osc2_.m_waveform) << " o2amp:" << osc2_amp_
     << " filter2_type:" << k_filter_type_names[filter1_.m_filter_type] << "("
     << filter1_.m_filter_type << ")"
     << " filter2_fc:" << filter2_.m_fc << " filter2_q:" << filter2_.m_q
     << " filter2_en:" << filter2_en_ << std::endl;
  ss << COOL_COLOR_PINK2 << "     eg_attack:" << eg_.m_attack_time_msec
     << " eg_decay:" << eg_.m_decay_time_msec
     << " eg_sustain:" << eg_.m_sustain_level
     << " eg_release:" << eg_.m_release_time_msec
     << " eg_hold:" << eg_.hold_time_ms_ << " ramp:" << eg_.ramp_mode
     << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW
     << "     eg_o1_ptch:" << modulations_[ENVI][OSC1_PITCHD]
     << " eg_o2_ptch:" << modulations_[ENVI][OSC1_PITCHD]
     << " eg_f1_fc:" << modulations_[ENVI][FILTER1_FCD]
     << " eg_f1_q:" << modulations_[ENVI][FILTER1_QD]
     << " eg_f2_fc:" << modulations_[ENVI][FILTER2_FCD]
     << " eg_f2_q:" << modulations_[ENVI][FILTER2_QD] << std::endl;
  ss << COOL_COLOR_PINK2 << "     lfowav:" << k_lfo_wave_names[lfo_.m_waveform]
     << "(" << lfo_.m_waveform << ")"
     << " lfomode:" << k_lfo_mode_names[lfo_.m_lfo_mode] << "("
     << lfo_.m_lfo_mode << ")"
     << " lforate:" << lfo_.m_osc_fo << " lfoamp:" << lfo_.m_amplitude
     << COOL_COLOR_YELLOW_MELLOW
     << " lfo_osc1:" << modulations_[LFOI][OSC1_PITCHD]
     << " lfo_osc2:" << modulations_[LFOI][OSC2_PITCHD] << std::endl;
  ss << "     lfo_filter1_fc:" << modulations_[LFOI][FILTER1_FCD]
     << " lfo_filter1_q:" << modulations_[LFOI][FILTER1_QD]
     << " lfo_filter2_fc:" << modulations_[LFOI][FILTER2_FCD]
     << " lfo_filter2_q:" << modulations_[LFOI][FILTER2_QD] << std::endl;

  return ss.str();
}

std::string DrumSynth::Info() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << ANSI_COLOR_CYAN;
  ss << "Drumsynth~!";

  return ss.str();
}

void DrumSynth::start() {
  if (active) return;  // no-op
  active = true;
}

void DrumSynth::stop() {
  if (active) return;  // no-op
  active = false;
}

void DrumSynth::noteOff(midi_event ev) { eg_.NoteOff(); }

void DrumSynth::noteOn(midi_event ev) {
  unsigned int midinote = ev.data1;
  unsigned int velocity = ev.data2;

  base_frequency_ = get_midi_freq(midinote);
  starting_frequency_ = get_midi_freq(midinote + pitch_range_);
  frequency_diff_ = starting_frequency_ - base_frequency_;

  osc1_.m_note_on = true;
  osc1_.m_osc_fo = starting_frequency_;
  osc1_.Update();
  osc1_.StartOscillator();

  osc2_.m_note_on = true;
  osc2_.m_osc_fo = starting_frequency_;
  osc2_.Update();
  osc2_.StartOscillator();

  lfo_.m_note_on = true;
  lfo_.Update();
  lfo_.StartOscillator();

  eg_.StartEg();
}

void DrumSynth::Save(std::string new_preset_name) {
  if (new_preset_name.empty()) {
    printf(
        "Play tha game, pal, need a name to save yer synth settings "
        "with\n");
    return;
  }
  const char *preset_name = new_preset_name.c_str();

  printf("Saving '%s' settings for Minisynth to file %s\n", preset_name,
         DRUM_SYNTH_PATCHES);
  FILE *presetzzz = fopen(DRUM_SYNTH_PATCHES, "a+");
  if (presetzzz == NULL) {
    printf("Couldn't save settings!!\n");
    return;
  }

  patch_name_ = new_preset_name;
  int settings_count = 0;

  fprintf(presetzzz, "::name=%s", preset_name);
  settings_count++;

  fprintf(presetzzz, "::osc1=%d", osc1_.m_waveform);
  settings_count++;
  fprintf(presetzzz, "::o1amp=%f", osc1_amp_);
  settings_count++;

  fprintf(presetzzz, "::osc2=%d", osc2_.m_waveform);
  settings_count++;
  fprintf(presetzzz, "::o2amp=%f", osc2_amp_);
  settings_count++;

  fprintf(presetzzz, "::eg_attack=%f", eg_.m_attack_time_msec);
  settings_count++;
  fprintf(presetzzz, "::eg_decay=%f", eg_.m_decay_time_msec);
  settings_count++;
  fprintf(presetzzz, "::eg_sustain=%f", eg_.m_sustain_level);
  settings_count++;
  fprintf(presetzzz, "::eg_release=%f", eg_.m_release_time_msec);
  settings_count++;

  fprintf(presetzzz, ":::\n");
  fclose(presetzzz);
  printf("Wrote %d settings\n", settings_count);
}

void DrumSynth::LoadSettings(DrumSettings settings) {
  patch_name_ = settings.name;
  distortion_.SetParam("threshold", settings.distortion_threshold);

  pitch_range_ = settings.pitch_range;
  freq_range_ = settings.freq_range;
  q_range_ = settings.q_range;

  osc1_.m_waveform = settings.osc1_wav;
  osc1_amp_ = settings.osc1_amp;
  filter1_en_ = settings.filter1_en;

  osc2_.m_waveform = settings.osc2_wav;
  osc2_amp_ = settings.osc2_amp;
  filter2_en_ = settings.filter2_en;

  eg_.SetAttackTimeMsec(settings.eg_attack_ms);
  eg_.SetDecayTimeMsec(settings.eg_decay_ms);
  eg_.SetSustainLevel(settings.eg_sustain_level);
  eg_.SetReleaseTimeMsec(settings.eg_release_ms);
  eg_.SetRampMode(settings.eg_ramp_mode);

  lfo_.m_waveform = settings.lfo_wave;
  lfo_.m_lfo_mode = settings.lfo_mode;
  lfo_.m_fo = settings.lfo_rate;

  modulations_ = settings.modulations;
}

void DrumSynth::ListPresets() {
  FILE *presetzzz = fopen(DRUM_SYNTH_PATCHES, "r+");
  if (presetzzz == NULL) return;

  char line[256];
  while (fgets(line, sizeof(line), presetzzz)) {
    printf("%s\n", line);
  }

  fclose(presetzzz);
}

}  // namespace SBAudio
