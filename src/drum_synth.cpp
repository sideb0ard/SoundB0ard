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
  m_eg.SetDecayTimeMsec(100);
  m_eg.SetSustainLevel(0.7);
  m_eg.SetReleaseTimeMsec(50);
  m_eg.m_output_eg = true;
  // m_eg.ramp_mode = true;
  m_eg.m_reset_to_zero = true;
  m_eg.Update();

  m_dca.m_mod_source_eg = DEST_DCA_EG;

  active = true;
}

StereoVal DrumSynth::GenNext(mixer_timing_info tinfo) {
  frames_per_midi_tick_ = tinfo.frames_per_midi_tick;
  StereoVal out = {.left = 0, .right = 0};
  if (!active) return out;

  if (osc1.m_note_on) {
    // m_eg.Update();
    // double m_eg_val = 0.0;
    // double eg_out = m_eg.DoEnvelope(&m_eg_val);

    double concav = concave_inverted_transform(modulo_);
    if (concav < 0) {
      osc1.m_note_on = false;
    }
    osc1.m_osc_fo = concav * starting_frequency_;
    modulo_ += inc_;

    m_dca.SetEgMod(concav);
    m_dca.Update();

    osc1.Update();
    // std::cout << "CONCA:" << concav << " MOD:" << modulo_
    //           << " FOOO:" << osc1.m_fo << " (INC:" << inc_ << ")" <<
    //           std::endl;
    double osc1_out = osc1.DoOscillate(nullptr);

    double out_left = 0.0;
    double out_right = 0.0;

    m_dca.DoDCA(osc1_out, osc1_out, &out_left, &out_right);

    out = {.left = out_left * volume, .right = out_right * volume};
    out = Effector(out);
  }

  // if (m_eg.GetState() == OFFF) {
  //   osc1.StopOscillator();
  //   osc2.StopOscillator();
  //   m_eg.StopEg();
  // }

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
}

std::string DrumSynth::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << ANSI_COLOR_CYAN;
  ss << "DrumSynth osc1:" << GetOscType(osc1.m_waveform)
     << " o1amp:" << osc1_amp << " osc2:" << GetOscType(osc2.m_waveform)
     << " o2amp:" << osc2_amp << std::endl;
  ss << "     eg_attack:" << m_eg.m_attack_time_msec
     << " eg_decay:" << m_eg.m_decay_time_msec
     << " eg_sustain:" << m_eg.m_sustain_level
     << " eg_release:" << m_eg.m_release_time_msec;

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

  osc1.m_note_on = true;
  osc1.m_osc_fo = get_midi_freq(midinote);

  starting_frequency_ = osc1.m_osc_fo;
  modulo_ = 0;
  double duration_in_frames = frames_per_midi_tick_ * ev.dur;
  inc_ = 1. / duration_in_frames;

  osc1.Update();
  osc1.StartOscillator();

  // osc2.m_note_on = true;
  // osc2.m_osc_fo = get_midi_freq(midinote);
  // osc2.StartOscillator();

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
