#include <drum_synth.h>

#include <iostream>
#include <sstream>

#include "midi_freq_table.h"
#include "utils.h"

namespace {
std::string GetOscType(int type) {
  std::string the_type = "dunno";
  switch (type) {
    case 0:
      the_type = "SINE";
      break;
    case 1:
      the_type = "SAW1";
      break;
    case 2:
      the_type = "SAW2";
      break;
    case 3:
      the_type = "SAW3";
      break;
    case 4:
      the_type = "TRI";
      break;
    case 5:
      the_type = "SQUARE";
      break;
    case 6:
      the_type = "NOISE";
      break;
    case 7:
      the_type = "PNOISE";
      break;
  }

  return the_type;
}
}  // namespace

namespace SBAudio {

DrumSynth::DrumSynth() {
  osc1.m_waveform = SINE;
  osc1_amp = 1;
  osc2.m_waveform = NOISE;
  osc2_amp = 0;

  m_eg.SetEgMode(ANALOG);
  m_eg.SetAttackTimeMsec(1);
  m_eg.SetDecayTimeMsec(0);
  m_eg.SetSustainLevel(1);
  m_eg.SetReleaseTimeMsec(20);
  m_eg.m_output_eg = true;
  m_eg.SetRampMode(true);
  m_eg.m_reset_to_zero = true;
  m_eg.Update();

  m_dca.m_mod_source_eg = DEST_DCA_EG;

  active = true;
}

StereoVal DrumSynth::GenNext(mixer_timing_info tinfo) {
  StereoVal out = {.left = 0, .right = 0};
  if (!active) return out;

  if (osc1.m_note_on) {
    m_eg.Update();
    double m_eg_val = 0.0;
    double eg_out = m_eg.DoEnvelope(&m_eg_val);

    osc1.m_osc_fo = base_frequency_ + eg_out * frequency_diff_;
    osc1.Update();

    m_dca.SetEgMod(eg_out);
    m_dca.Update();

    double osc1_out = osc1.DoOscillate(nullptr);

    double out_left = 0.0;
    double out_right = 0.0;

    m_dca.DoDCA(osc1_out, osc1_out, &out_left, &out_right);

    out = {.left = out_left * volume, .right = out_right * volume};
    out = Effector(out);
  }

  if (m_eg.GetState() == OFFF) {
    osc1.StopOscillator();
    osc2.StopOscillator();
    m_eg.StopEg();
    osc1.m_note_on = false;
  }

  return out;
}

void DrumSynth::SetParam(std::string name, double val) {
  if (name == "osc1")
    osc1.m_waveform = val;
  else if (name == "osc2")
    osc2.m_waveform = val;
  else if (name == "o1amp")
    osc1_amp = val;
  else if (name == "o2amp")
    osc2_amp = val;
  if (name == "eg_attack") m_eg.SetAttackTimeMsec(val);
  if (name == "eg_decay") m_eg.SetDecayTimeMsec(val);
  if (name == "eg_sustain") m_eg.SetSustainLevel(val);
  if (name == "eg_release") m_eg.SetReleaseTimeMsec(val);
  if (name == "pitch_range") pitch_range_ = val;
}

std::string DrumSynth::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    //    ss << ANSI_COLOR_CYAN;
    ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "DrumZynth  // pitch_range:" << pitch_range_ << std::endl;
  ss << "     osc1:" << GetOscType(osc1.m_waveform) << " o1amp:" << osc1_amp
     << " filter1_fc:" << filter1_.m_fc << " filter1_q:" << filter1_.m_q
     << std::endl;
  ss << "     osc2:" << GetOscType(osc2.m_waveform) << " o2amp:" << osc2_amp
     << " filter2_fc:" << filter2_.m_fc << " filter2_q:" << filter2_.m_q
     << std::endl;
  ss << "     eg_attack:" << m_eg.m_attack_time_msec
     << " eg_decay:" << m_eg.m_decay_time_msec
     << " eg_sustain:" << m_eg.m_sustain_level
     << " eg_release:" << m_eg.m_release_time_msec
     << " eg_hold:" << m_eg.hold_time_ms_ << " ramp:" << m_eg.ramp_mode
     << std::endl;
  ss << "     eg_o1_ptch:" << eg_o1_pitch_
     << " eg1_o1_ptch_int:" << eg_o1_pitch_int_
     << " eg_o2_ptch:" << eg_o2_pitch_;
  ss << " eg_o2_ptch_int:" << eg_o2_pitch_int_ << std::endl;
  ss << "     eg_f1_fc:" << eg_f1_fc_ << " eg_f1_fc_int:" << eg_f1_fc_int_
     << " eg_f1_q:" << eg_f1_q_ << " eg_f2_fc:" << eg_f2_fc_
     << " eg_f2_q:" << eg_f2_q_ << std::endl;

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

void DrumSynth::noteOff(midi_event ev) { m_eg.NoteOff(); }

void DrumSynth::noteOn(midi_event ev) {
  unsigned int midinote = ev.data1;
  unsigned int velocity = ev.data2;

  base_frequency_ = get_midi_freq(midinote);
  starting_frequency_ = get_midi_freq(midinote + pitch_range_);
  frequency_diff_ = starting_frequency_ - base_frequency_;

  osc1.m_note_on = true;
  osc1.m_osc_fo = starting_frequency_;

  osc1.Update();
  osc1.StartOscillator();

  m_eg.StartEg();
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

  patch_name = new_preset_name;
  int settings_count = 0;

  fprintf(presetzzz, "::name=%s", preset_name);
  settings_count++;

  fprintf(presetzzz, "::osc1=%d", osc1.m_waveform);
  settings_count++;
  fprintf(presetzzz, "::o1amp=%f", osc1_amp);
  settings_count++;

  fprintf(presetzzz, "::osc2=%d", osc2.m_waveform);
  settings_count++;
  fprintf(presetzzz, "::o2amp=%f", osc2_amp);
  settings_count++;

  fprintf(presetzzz, "::eg_attack=%f", m_eg.m_attack_time_msec);
  settings_count++;
  fprintf(presetzzz, "::eg_decay=%f", m_eg.m_decay_time_msec);
  settings_count++;
  fprintf(presetzzz, "::eg_sustain=%f", m_eg.m_sustain_level);
  settings_count++;
  fprintf(presetzzz, "::eg_release=%f", m_eg.m_release_time_msec);
  settings_count++;

  fprintf(presetzzz, ":::\n");
  fclose(presetzzz);
  printf("Wrote %d settings\n", settings_count);
}

void DrumSynth::Load(std::string preset_name) {
  if (preset_name.empty()) {
    printf(
        "Play tha game, pal, need a name to LOAD yer synth settings "
        "with\n");
    return;
  }
  const char *preset_to_load = preset_name.c_str();

  char line[2048];
  char setting_key[512];
  char setting_val[512];
  double scratch_val = 0.;

  FILE *presetzzz = fopen(DRUM_SYNTH_PATCHES, "r+");
  if (presetzzz == NULL) return;

  char *tok, *last_tok;
  char const *sep = "::";

  while (fgets(line, sizeof(line), presetzzz)) {
    int settings_count = 0;

    for (tok = strtok_r(line, sep, &last_tok); tok;
         tok = strtok_r(NULL, sep, &last_tok)) {
      sscanf(tok, "%[^=]=%s", setting_key, setting_val);
      sscanf(setting_val, "%lf", &scratch_val);
      if (strcmp(setting_key, "name") == 0) {
        if (strcmp(setting_val, preset_to_load) != 0) break;
        patch_name = setting_val;
        settings_count++;
      } else {
        SetParam(setting_key, scratch_val);
      }
    }
  }

  fclose(presetzzz);
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
