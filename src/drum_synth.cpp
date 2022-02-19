#include <drum_synth.h>

#include <iostream>
#include <sstream>

#include "midi_freq_table.h"

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

  pitch_env.SetEgMode(ANALOG);
  pitch_env.SetAttackTimeMsec(0);
  pitch_env.SetDecayTimeMsec(70);
  pitch_env.SetSustainLevel(0);
  pitch_env.SetReleaseTimeMsec(0);
  pitch_env.ramp_mode = true;
  pitch_env_to_osc1 = true;

  amp_env.SetEgMode(ANALOG);
  amp_env.SetAttackTimeMsec(0);
  amp_env.SetDecayTimeMsec(300);
  amp_env.SetSustainLevel(0);
  amp_env.SetReleaseTimeMsec(3000);
  amp_env.m_output_eg = true;
  amp_env.ramp_mode = true;
  amp_env.m_reset_to_zero = true;

  m_dca.m_mod_source_eg = DEST_DCA_EG;

  active = true;
}

stereo_val DrumSynth::genNext(mixer_timing_info tinfo) {
  stereo_val out = {.left = 0, .right = 0};
  if (!active) return out;

  if (osc1.m_note_on) {
    // Pitch Envelope
    double pitch_env_val = 0.0;
    pitch_env.DoEnvelope(&pitch_env_val);

    if (pitch_env_to_osc1) {
      osc1.SetFoModExp(pitch_env_int * OSC_FO_MOD_RANGE * pitch_env_val);
      osc1.Update();
    }
    double osc1_out = osc1.DoOscillate(nullptr);

    if (pitch_env_to_osc2) {
      osc2.SetFoModExp(pitch_env_int * OSC_FO_MOD_RANGE * pitch_env_val);
      osc2.Update();
    }
    double osc2_out = osc2.DoOscillate(nullptr);

    double osc_mix = osc1_out * osc1_amp + osc2_out * osc2_amp;

    // Amp Envelope
    double amp_env_val = 0.0;
    double eg_out = amp_env.DoEnvelope(&amp_env_val);

    m_dca.SetEgMod(eg_out);
    m_dca.Update();

    filter1.Update();
    double filter_out = filter1.DoFilter(osc_mix);

    double out_left = 0.0;
    double out_right = 0.0;

    m_dca.DoDCA(filter_out, filter_out, &out_left, &out_right);

    out = {.left = out_left * volume, .right = out_right * volume};
    out = Effector(out);
  }

  if (amp_env.GetState() == OFFF) {
    osc1.StopOscillator();
    osc2.StopOscillator();
    amp_env.StopEg();
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
  else if (name == "filter1")
    filter1.SetType(val);
  else if (name == "f1fc")
    filter1.SetFcControl(val);
  else if (name == "f1fq")
    filter1.SetQControl(val);
  else if (name == "f1_osc1_enable")
    f1_osc1_enable = val;
  else if (name == "f1_osc2_enable")
    f1_osc2_enable = val;
  else if (name == "filter2")
    filter2.SetType(val);
  else if (name == "f2fc")
    filter2.SetFcControl(val);
  else if (name == "f2fq")
    filter2.SetQControl(val);
  else if (name == "f2_osc1_enable")
    f2_osc1_enable = val;
  else if (name == "f2_osc2_enable")
    f2_osc2_enable = val;
  else if (name == "pitch_env_attack")
    pitch_env.SetAttackTimeMsec(val);
  if (name == "pitch_env_decay") pitch_env.SetDecayTimeMsec(val);
  if (name == "pitch_env_sustain") pitch_env.SetSustainLevel(val);
  if (name == "pitch_env_release") pitch_env.SetReleaseTimeMsec(val);
  if (name == "pitch_env_int") pitch_env_int = val;
  if (name == "pitch_env_to_osc1") pitch_env_to_osc1 = val;
  if (name == "pitch_env_to_osc2") pitch_env_to_osc2 = val;
  if (name == "amp_env_attack") amp_env.SetAttackTimeMsec(val);
  if (name == "amp_env_decay") amp_env.SetDecayTimeMsec(val);
  if (name == "amp_env_sustain") amp_env.SetSustainLevel(val);
  if (name == "amp_env_release") amp_env.SetReleaseTimeMsec(val);
  if (name == "amp_env_int") amp_env_int = val;
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
  ss << "     filter1:" << filter1.m_filter_type << " f1fc:" << filter1.m_fc
     << " f1fq:" << filter1.m_q
     << " f1_osc1_enable:" << (f1_osc1_enable ? "true" : "false")
     << " f1_osc2_enable:" << (f1_osc2_enable ? "true" : "false") << std::endl;
  ss << "     filter2:" << filter2.m_filter_type << " f2fc:" << filter2.m_fc
     << " f2fq:" << filter1.m_q
     << " f2_osc1_enable:" << (f2_osc1_enable ? "true" : "false")
     << " f2_osc2_enable:" << (f2_osc2_enable ? "true" : "false") << std::endl;

  ss << "     pitch_env_attack:" << pitch_env.m_attack_time_msec
     << " pitch_env_decay:" << pitch_env.m_decay_time_msec
     << " pitch_env_sustain:" << pitch_env.m_sustain_level
     << " pitch_env_release:" << pitch_env.m_release_time_msec << std::endl;
  ss << "     pitch_env_to_osc1:" << (pitch_env_to_osc1 ? "true" : "false")
     << " pitch_env_to_osc2:" << (pitch_env_to_osc2 ? "true" : "false")
     << " pitch_env_int:" << pitch_env_int << std::endl;
  ss << "     amp_env_attack:" << amp_env.m_attack_time_msec
     << " amp_env_decay:" << amp_env.m_decay_time_msec
     << " amp_env_sustain:" << amp_env.m_sustain_level
     << " amp_env_release:" << amp_env.m_release_time_msec << std::endl;
  ss << "     amp_env_to_osc1:" << (amp_env_to_osc1 ? "true" : "false")
     << " amp_env_to_osc2:" << (amp_env_to_osc2 ? "true" : "false")
     << " amp_env_int:" << amp_env_int;

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

double DrumSynth::GetParam(std::string name) { return 0; }

void DrumSynth::start() {
  if (active) return;  // no-op
  active = true;
}

void DrumSynth::noteOn(midi_event ev) {
  unsigned int midinote = ev.data1;
  unsigned int velocity = ev.data2;

  osc1.m_note_on = true;
  osc1.m_osc_fo = get_midi_freq(midinote);
  osc1.StartOscillator();

  osc2.m_note_on = true;
  osc2.m_osc_fo = get_midi_freq(midinote);
  osc2.StartOscillator();

  pitch_env.StartEg();
  amp_env.StartEg();
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

  fprintf(presetzzz, "::filter1=%d", filter1.m_filter_type);
  settings_count++;
  fprintf(presetzzz, "::f1fc=%f", filter1.m_fc);
  settings_count++;
  fprintf(presetzzz, "::f1fq=%f", filter1.m_q);
  settings_count++;
  fprintf(presetzzz, "::f1_osc1_enable=%d", f1_osc1_enable);
  settings_count++;
  fprintf(presetzzz, "::f1_osc2_enable=%d", f1_osc2_enable);
  settings_count++;

  fprintf(presetzzz, "::filter2=%d", filter2.m_filter_type);
  settings_count++;
  fprintf(presetzzz, "::f2fc=%f", filter2.m_fc);
  settings_count++;
  fprintf(presetzzz, "::f2fq=%f", filter2.m_q);
  settings_count++;
  fprintf(presetzzz, "::f2_osc1_enable=%d", f2_osc1_enable);
  settings_count++;
  fprintf(presetzzz, "::f2_osc2_enable=%d", f2_osc2_enable);
  settings_count++;

  fprintf(presetzzz, "::pitch_env_attack=%f", pitch_env.m_attack_time_msec);
  settings_count++;
  fprintf(presetzzz, "::pitch_env_decay=%f", pitch_env.m_decay_time_msec);
  settings_count++;
  fprintf(presetzzz, "::pitch_env_sustain=%f", pitch_env.m_sustain_level);
  settings_count++;
  fprintf(presetzzz, "::pitch_env_release=%f", pitch_env.m_release_time_msec);
  settings_count++;
  fprintf(presetzzz, "::pitch_env_to_osc1=%d", pitch_env_to_osc1);
  settings_count++;
  fprintf(presetzzz, "::pitch_env_to_osc2=%d", pitch_env_to_osc1);
  settings_count++;

  fprintf(presetzzz, "::amp_env_attack=%f", amp_env.m_attack_time_msec);
  settings_count++;
  fprintf(presetzzz, "::amp_env_decay=%f", amp_env.m_decay_time_msec);
  settings_count++;
  fprintf(presetzzz, "::amp_env_sustain=%f", amp_env.m_sustain_level);
  settings_count++;
  fprintf(presetzzz, "::amp_env_release=%f", amp_env.m_release_time_msec);
  settings_count++;
  fprintf(presetzzz, "::amp_env_int=%f", amp_env_int);
  settings_count++;
  fprintf(presetzzz, "::amp_env_to_osc1=%d", amp_env_to_osc1);
  settings_count++;
  fprintf(presetzzz, "::amp_env_to_osc2=%d", amp_env_to_osc1);
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

};  // namespace SBAudio
