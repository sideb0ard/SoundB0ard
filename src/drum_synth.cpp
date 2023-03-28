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
  eg_.SetEgMode(ANALOG);
  eg_.m_output_eg = true;
  eg_.m_reset_to_zero = true;
  dca_.m_mod_source_eg = DEST_DCA_EG;

  osc1_ = std::make_unique<QBLimitedOscillator>();
  osc2_ = std::make_unique<QBLimitedOscillator>();
  filter1_ = std::make_unique<MoogLadder>();
  filter2_ = std::make_unique<MoogLadder>();

  // default
  LoadSettings(DrumSettings());
  active = true;
}

StereoVal DrumSynth::GenNext(mixer_timing_info tinfo) {
  StereoVal out = {.left = 0, .right = 0};
  if (!active) return out;
  ms_per_midi_tick_ = tinfo.ms_per_midi_tick;

  if (osc1_->m_note_on) {
    lfo_.Update();
    double lfo_out = lfo_.DoOscillate(0);
    double eg_out = eg_.DoEnvelope(nullptr);

    dca_.SetEgMod(eg_out);
    dca_.Update();

    ///////////

    double osc1_mod_val = 0;
    double osc2_mod_val = 0;

    if (settings_.modulations[ENVI][OSC1_PITCHD] &&
        settings_.modulations[LFOI][OSC1_PITCHD])
      osc1_mod_val = eg_out * lfo_out;
    else if (settings_.modulations[ENVI][OSC1_PITCHD])
      osc1_mod_val = eg_out;
    else if (settings_.modulations[LFOI][OSC1_PITCHD])
      osc1_mod_val = lfo_out;

    if (settings_.modulations[ENVI][OSC2_PITCHD] &&
        settings_.modulations[LFOI][OSC2_PITCHD])
      osc2_mod_val = eg_out * lfo_out;
    else if (settings_.modulations[ENVI][OSC2_PITCHD])
      osc2_mod_val = eg_out;
    else if (settings_.modulations[LFOI][OSC2_PITCHD])
      osc2_mod_val = lfo_out;

    if (settings_.modulations[ENVI][FILTER1_FCD]) {
      filter1_->SetFcMod(eg_out * settings_.pitch_range);
      filter1_->Update();
    }
    if (settings_.modulations[ENVI][FILTER1_QD]) {
      filter1_->m_q_control +=
          (eg_out * settings_.q_range) - settings_.q_range / 2;
      filter1_->Update();
    }
    if (settings_.modulations[ENVI][FILTER2_FCD]) {
      filter2_->SetFcMod(eg_out * settings_.pitch_range);
      filter2_->Update();
    }
    if (settings_.modulations[ENVI][FILTER2_QD]) {
      filter2_->m_q_control +=
          (eg_out * settings_.q_range) - settings_.q_range / 2;
      filter2_->Update();
    }

    // note - if ENV and LFO are enabled, LFO mod will overwrite ENV
    if (settings_.modulations[LFOI][FILTER1_FCD]) {
      filter1_->SetFcMod(lfo_out * settings_.pitch_range);
      filter1_->Update();
    }
    if (settings_.modulations[LFOI][FILTER1_QD]) {
      filter1_->m_q_control +=
          (lfo_out * settings_.q_range) - settings_.q_range / 2;
      filter1_->Update();
    }
    if (settings_.modulations[LFOI][FILTER2_FCD]) {
      filter2_->SetFcMod(lfo_out * settings_.pitch_range);
      filter2_->Update();
    }
    if (settings_.modulations[LFOI][FILTER2_QD]) {
      filter2_->m_q_control +=
          (lfo_out * settings_.q_range) - settings_.q_range / 2;
      filter2_->Update();
    }

    osc1_->m_osc_fo =
        settings_.base_frequency + osc1_mod_val * settings_.frequency_diff;
    osc1_->Update();
    double osc1_out = osc1_->DoOscillate(nullptr) * settings_.osc1_amp;
    if (settings_.filter1_en) {
      osc1_out = filter1_->DoFilter(osc1_out);
    }

    osc2_->m_osc_fo =
        settings_.base_frequency + osc2_mod_val * settings_.frequency_diff;
    osc2_->Update();
    double osc2_out = osc2_->DoOscillate(nullptr) * settings_.osc2_amp;
    if (settings_.filter2_en) {
      osc2_out = filter2_->DoFilter(osc2_out);
    }

    //////////////////////////////

    double out_left = 0.0;
    double out_right = 0.0;

    dca_.DoDCA(osc1_out + osc2_out, osc1_out + osc2_out, &out_left, &out_right);

    out = {.left = out_left * volume * settings_.amplitude,
           .right = out_right * volume * settings_.amplitude};
    out = distortion_.Process(out);
    out = Effector(out);
  }

  if (eg_.GetState() == OFFF) {
    osc1_->StopOscillator();
    osc2_->StopOscillator();
    eg_.StopEg();
    osc1_->m_note_on = false;
  }

  return out;
}

void DrumSynth::SetParam(std::string name, double val) {
  if (name == "distort") {
    settings_.distortion_threshold = val;
    distortion_.SetParam("threshold", val);
  } else if (name == "pitch_range") {
    settings_.pitch_range = val;
  } else if (name == "q_range") {
    settings_.q_range = val;
  }

  else if (name == "osc1") {
    settings_.osc1_wav = val;
    osc1_->m_waveform = val;
  } else if (name == "o1amp") {
    settings_.osc1_amp = val;
  } else if (name == "filter1_type") {
    settings_.filter1_type = val;
    filter1_->SetType(val);
  } else if (name == "filter1_fc") {
    settings_.filter1_fc = val;
    filter1_->SetFcControl(val);
  } else if (name == "filter1_q") {
    settings_.filter1_q = val;
    filter1_->SetQControl(val);
  } else if (name == "filter1_en") {
    settings_.filter1_en = val;
  }

  else if (name == "osc2") {
    settings_.osc2_wav = val;
    osc2_->m_waveform = val;
  } else if (name == "o2amp") {
    settings_.osc2_amp = val;
  }

  else if (name == "filter2_type") {
    settings_.filter2_type = val;
    filter2_->SetType(val);
  } else if (name == "filter2_fc") {
    settings_.filter2_fc = val;
    filter2_->SetFcControl(val);
  } else if (name == "filter2_q") {
    settings_.filter2_q = val;
    filter2_->SetQControl(val);
  } else if (name == "filter2_en") {
    settings_.filter2_en = val;
  }

  else if (name == "eg_attack") {
    settings_.eg_attack_ms = val;
    eg_.SetAttackTimeMsec(val);
  } else if (name == "eg_decay") {
    settings_.eg_decay_ms = val;
    eg_.SetDecayTimeMsec(val);
  } else if (name == "eg_sustain") {
    settings_.eg_sustain_level = val;
    eg_.SetSustainLevel(val);
  } else if (name == "eg_release") {
    settings_.eg_release_ms = val;
    eg_.SetReleaseTimeMsec(val);
  }

  else if (name == "lfowav") {
    settings_.lfo_wave = val;
    lfo_.m_waveform = val;
  } else if (name == "lfomode") {
    settings_.lfo_mode = val;
    lfo_.m_lfo_mode = val;
  } else if (name == "lforate") {
    settings_.lfo_rate = val;
    lfo_.m_osc_fo = val;
  }

  else if (name == "eg_o1_ptch") {
    settings_.modulations[ENVI][OSC1_PITCHD] = val;
  } else if (name == "eg_o2_ptch") {
    settings_.modulations[ENVI][OSC2_PITCHD] = val;
  } else if (name == "eg_f1_fc") {
    settings_.modulations[ENVI][FILTER1_FCD] = val;
  } else if (name == "eg_f1_q") {
    settings_.modulations[ENVI][FILTER1_QD] = val;
  } else if (name == "eg_f2_fc") {
    settings_.modulations[ENVI][FILTER2_FCD] = val;
  } else if (name == "eg_f2_q") {
    settings_.modulations[ENVI][FILTER2_QD] = val;
  }

  else if (name == "lfo_osc1") {
    settings_.modulations[LFOI][OSC1_PITCHD] = val;
  } else if (name == "lfo_osc2") {
    settings_.modulations[LFOI][OSC2_PITCHD] = val;
  } else if (name == "lfo_filter1_fc") {
    settings_.modulations[LFOI][FILTER1_FCD] = val;
  } else if (name == "lfo_filter1_q") {
    settings_.modulations[LFOI][FILTER1_QD] = val;
  } else if (name == "lfo_filter2_fc") {
    settings_.modulations[LFOI][FILTER2_FCD] = val;
  } else if (name == "lfo_filter2_q") {
    settings_.modulations[LFOI][FILTER2_QD] = val;
  }

  osc1_->Update();
  osc2_->Update();
  filter1_->Update();
  filter2_->Update();
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
  ss << "DrumZynth - " << COOL_COLOR_PINK2 << settings_.name
     << COOL_COLOR_YELLOW_MELLOW << " - vol:" << volume
     << " distort:" << settings_.distortion_threshold
     << " pitch_range:" << settings_.pitch_range
     << " q_range:" << settings_.q_range << std::endl;
  ss << "     osc1:" << GetOscType(settings_.osc1_wav) << "("
     << settings_.osc1_wav << ")"
     << " o1amp:" << settings_.osc1_amp
     << " filter1_type:" << k_filter_type_names[settings_.filter1_type] << "("
     << settings_.filter1_type << ")"
     << " filter1_fc:" << settings_.filter1_fc
     << " filter1_q:" << settings_.filter1_q
     << " filter1_en:" << settings_.filter1_en << std::endl;
  ss << "     osc2:" << GetOscType(settings_.osc2_wav) << "("
     << settings_.osc2_wav << ")"
     << " o2amp:" << settings_.osc2_amp
     << " filter2_type:" << k_filter_type_names[settings_.filter2_type] << "("
     << settings_.filter2_type << ")"
     << " filter2_fc:" << settings_.filter2_fc
     << " filter2_q:" << settings_.filter2_q
     << " filter2_en:" << settings_.filter2_en << std::endl;
  ss << COOL_COLOR_PINK2 << "     eg_attack:" << settings_.eg_attack_ms
     << " eg_decay:" << settings_.eg_decay_ms
     << " eg_sustain:" << settings_.eg_sustain_level
     << " eg_release:" << settings_.eg_release_ms
     << " eg_hold:" << settings_.eg_hold_time_ms
     << " ramp:" << settings_.eg_ramp_mode << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW
     << "     eg_o1_ptch:" << settings_.modulations[ENVI][OSC1_PITCHD]
     << " eg_o2_ptch:" << settings_.modulations[ENVI][OSC1_PITCHD]
     << " eg_f1_fc:" << settings_.modulations[ENVI][FILTER1_FCD]
     << " eg_f1_q:" << settings_.modulations[ENVI][FILTER1_QD]
     << " eg_f2_fc:" << settings_.modulations[ENVI][FILTER2_FCD]
     << " eg_f2_q:" << settings_.modulations[ENVI][FILTER2_QD] << std::endl;
  ss << COOL_COLOR_PINK2
     << "     lfowav:" << k_lfo_wave_names[settings_.lfo_wave] << "("
     << settings_.lfo_wave << ")"
     << " lfomode:" << k_lfo_mode_names[settings_.lfo_mode] << "("
     << settings_.lfo_mode << ")"
     << " lforate:" << settings_.lfo_rate << COOL_COLOR_YELLOW_MELLOW
     << " lfo_osc1:" << settings_.modulations[LFOI][OSC1_PITCHD]
     << " lfo_osc2:" << settings_.modulations[LFOI][OSC2_PITCHD] << std::endl;
  ss << "     lfo_filter1_fc:" << settings_.modulations[LFOI][FILTER1_FCD]
     << " lfo_filter1_q:" << settings_.modulations[LFOI][FILTER1_QD]
     << " lfo_filter2_fc:" << settings_.modulations[LFOI][FILTER2_FCD]
     << " lfo_filter2_q:" << settings_.modulations[LFOI][FILTER2_QD]
     << std::endl;

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
  settings_.amplitude = scaleybum(0, 127, 0, 1, velocity);

  if (ev.dur != 240) {
    settings_.eg_release_ms = ms_per_midi_tick_ * ev.dur;
  }

  settings_.base_frequency = get_midi_freq(midinote);
  settings_.starting_frequency =
      get_midi_freq(midinote + settings_.pitch_range);
  settings_.frequency_diff =
      settings_.starting_frequency - settings_.base_frequency;

  osc1_->m_note_on = true;
  osc1_->m_osc_fo = settings_.starting_frequency;
  osc1_->Update();
  osc1_->StartOscillator();

  osc2_->m_note_on = true;
  osc2_->m_osc_fo = settings_.starting_frequency;
  osc2_->Update();
  osc2_->StartOscillator();

  lfo_.m_note_on = true;
  lfo_.Update();
  lfo_.StartOscillator();

  eg_.StartEg();
}

void DrumSynth::Save(std::string new_preset_name) {
  std::cout << "DRUMSYNTH -- saving new preset '" << new_preset_name << "'"
            << std::endl;
  if (new_preset_name.empty()) {
    std::cerr << "Play tha game, pal, need a name to save yer synth settings"
              << std::endl;
    return;
  }
  settings_.name = new_preset_name;

  std::ofstream presetzzz;
  const std::string kSEP = "::";
  presetzzz.open(DRUM_SYNTH_PATCHES, std::ios::app);
  presetzzz << "name:" << settings_.name << kSEP;
  presetzzz << "distortion_threshold:" << settings_.distortion_threshold
            << kSEP;
  presetzzz << "pitch_range:" << settings_.pitch_range << kSEP;
  presetzzz << "q_range:" << settings_.q_range << kSEP;
  presetzzz << "starting_frequency:" << settings_.starting_frequency << kSEP;
  presetzzz << "base_frequency:" << settings_.base_frequency << kSEP;
  presetzzz << "frequency_diff:" << settings_.frequency_diff << kSEP;
  presetzzz << "osc1_wav:" << settings_.osc1_wav << kSEP;
  presetzzz << "osc1_amp:" << settings_.osc1_amp << kSEP;
  presetzzz << "filter1_en:" << settings_.filter1_en << kSEP;
  presetzzz << "filter1_type:" << settings_.filter1_type << kSEP;
  presetzzz << "filter1_fc:" << settings_.filter1_fc << kSEP;
  presetzzz << "filter1_fq:" << settings_.filter1_q << kSEP;
  presetzzz << "osc2_wav:" << settings_.osc2_wav << kSEP;
  presetzzz << "osc2_amp:" << settings_.osc2_amp << kSEP;
  presetzzz << "filter2_en:" << settings_.filter2_en << kSEP;
  presetzzz << "filter2_type:" << settings_.filter2_type << kSEP;
  presetzzz << "filter2_fc:" << settings_.filter2_fc << kSEP;
  presetzzz << "filter2_q:" << settings_.filter2_q << kSEP;
  presetzzz << "eg_attack_ms:" << settings_.eg_attack_ms << kSEP;
  presetzzz << "eg_decay_ms:" << settings_.eg_decay_ms << kSEP;
  presetzzz << "eg_susytain_level:" << settings_.eg_sustain_level << kSEP;
  presetzzz << "eg_release_ms:" << settings_.eg_release_ms << kSEP;
  presetzzz << "eg_hold_time_ms:" << settings_.eg_hold_time_ms << kSEP;
  presetzzz << "eg_ramp_mode:" << settings_.eg_ramp_mode << kSEP;
  presetzzz << "lfo_wave:" << settings_.lfo_wave << kSEP;
  presetzzz << "lfo_mode:" << settings_.lfo_mode << kSEP;
  presetzzz << "lfo_rate:" << settings_.lfo_rate << kSEP;
  presetzzz << "env_routes:";
  for (const auto &v : settings_.modulations[ENVI]) presetzzz << v;
  presetzzz << kSEP;
  presetzzz << "lfo_routes:";
  for (const auto &v : settings_.modulations[LFOI]) presetzzz << v;
  presetzzz << kSEP;
  presetzzz << std::endl;
  presetzzz.close();
  std::cout << "DRUMSYNTH -- saving -- DONE" << std::endl;
}
void DrumSynth::Load(std::string preset_name) {
  std::cout << "DRUMSYNTH -- loading preset '" << preset_name << "'"
            << std::endl;

  std::cout << "WAIIIT A SEC?!\n\n";
  std::ifstream presetzzz;
  const std::string kSEP = "::";
  presetzzz.open(DRUM_SYNTH_PATCHES);
  for (std::string line; getline(presetzzz, line);) {
    std::cout << "LINE:" << line << std::endl;
  }
  presetzzz.close();
}

void DrumSynth::LoadSettings(DrumSettings settings) {
  settings_ = settings;

  distortion_.SetParam("threshold", settings_.distortion_threshold);

  osc1_->m_waveform = settings_.osc1_wav;
  osc2_->m_waveform = settings_.osc2_wav;

  filter1_->SetFcControl(settings_.filter1_fc);
  filter1_->SetQControl(settings_.filter1_q);

  filter2_->SetFcControl(settings_.filter2_fc);
  filter2_->SetQControl(settings_.filter2_q);

  eg_.SetAttackTimeMsec(settings_.eg_attack_ms);
  eg_.SetDecayTimeMsec(settings_.eg_decay_ms);
  eg_.SetSustainLevel(settings_.eg_sustain_level);
  eg_.SetReleaseTimeMsec(settings_.eg_release_ms);
  eg_.SetRampMode(settings_.eg_ramp_mode);

  lfo_.m_waveform = settings_.lfo_wave;
  lfo_.m_lfo_mode = settings_.lfo_mode;
  lfo_.m_fo = settings_.lfo_rate;
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

DrumSettings GetDrumSettings(std::string preset_name) {
  DrumSettings preset;
  std::ifstream presetzzz;
  const std::string kSEP = "::";
  const std::string ktoken_SEP = ":";
  presetzzz.open(DRUM_SYNTH_PATCHES);
  bool preset_found{false};
  for (std::string line; getline(presetzzz, line);) {
    if (preset_found) return preset;
    size_t pos = 0;
    std::string token;
    while ((pos = line.find(kSEP)) != std::string::npos) {
      token = line.substr(0, pos);
      size_t token_pos = token.find(ktoken_SEP);
      auto key = token.substr(0, token_pos);
      auto val = token.substr(token_pos + ktoken_SEP.size(), std::string::npos);

      if (key == "name" && val != preset_name) {
        break;
      }
      preset_found = true;

      double dval = 0;
      if (key != "name") dval = std::stod(val);
      if (key == "name")
        preset.name = val;
      else if (key == "distortion_threshold")
        preset.distortion_threshold = dval;
      else if (key == "pitch_range")
        preset.pitch_range = dval;
      else if (key == "q_range")
        preset.q_range = dval;
      else if (key == "starting_frequency")
        preset.starting_frequency = dval;
      else if (key == "base_frequency")
        preset.base_frequency = dval;
      else if (key == "frequency_diff")
        preset.frequency_diff = dval;
      else if (key == "osc1_wav")
        preset.osc1_wav = dval;
      else if (key == "osc1_amp")
        preset.osc1_amp = dval;
      else if (key == "filter1_en")
        preset.filter1_en = dval;
      else if (key == "filter1_type")
        preset.filter1_type = dval;
      else if (key == "filter1_fc")
        preset.filter1_fc = dval;
      else if (key == "filter1_q")
        preset.filter1_q = dval;
      else if (key == "osc2_wav")
        preset.osc2_wav = dval;
      else if (key == "osc2_amp")
        preset.osc2_amp = dval;
      else if (key == "filter2_en")
        preset.filter2_en = dval;
      else if (key == "filter2_type")
        preset.filter2_type = dval;
      else if (key == "filter2_fc")
        preset.filter2_fc = dval;
      else if (key == "filter2_q")
        preset.filter2_q = dval;
      else if (key == "eg_attack_ms")
        preset.eg_attack_ms = dval;
      else if (key == "eg_decay_ms")
        preset.eg_decay_ms = dval;
      else if (key == "eg_sustain_level")
        preset.eg_sustain_level = dval;
      else if (key == "eg_release_ms")
        preset.eg_release_ms = dval;
      else if (key == "eg_hold_time_ms")
        preset.eg_hold_time_ms = dval;
      else if (key == "eg_ramp_mode")
        preset.eg_ramp_mode = dval;
      else if (key == "lfo_wave")
        preset.lfo_wave = dval;
      else if (key == "lfo_mode")
        preset.lfo_mode = dval;
      else if (key == "lfo_rate")
        preset.lfo_rate = dval;

      else if (key == "env_routes") {
        if (val.size() == preset.modulations[ENVI].size()) {
          for (int i = 0; i < val.size(); ++i) {
            char c = val[i];
            if (c == '1') {
              preset.modulations[ENVI][i] = 1;
            }
          }
        } else {
          std::cerr << "WOW< YOU GOT BIG PROBLEMS BUDDY!\n";
        }

      } else if (key == "lfo_routes") {
        if (val.size() == preset.modulations[LFOI].size()) {
          for (int i = 0; i < val.size(); ++i) {
            char c = val[i];
            if (c == '1') {
              preset.modulations[LFOI][i] = 1;
            }
          }
        } else {
          std::cerr << "WOW< YOU GOT BIG LFOPROBLEMS BUDDY!\n";
        }
      }

      std::cout << token << std::endl;
      line.erase(0, pos + kSEP.length());
    }
  }

  presetzzz.close();
  return preset;
}

}  // namespace SBAudio
