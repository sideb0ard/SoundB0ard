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
bool BoolGen() { return std::rand() % 2; }
}  // namespace

namespace SBAudio {

DrumSynth::DrumSynth() {
  eg_.SetEgMode(ANALOG);
  eg_.m_output_eg = true;
  // eg_.m_reset_to_zero = true;
  dca_.m_mod_source_eg = DEST_DCA_EG;

  osc1_ = std::make_unique<QBLimitedOscillator>();
  osc2_ = std::make_unique<QBLimitedOscillator>();
  osc3_ = std::make_unique<QBLimitedOscillator>();
  filter1_ = std::make_unique<MoogLadder>();
  filter2_ = std::make_unique<MoogLadder>();
  filter3_ = std::make_unique<MoogLadder>();
  master_filter_ = std::make_unique<MoogLadder>();

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

    double dca_mod_val = eg_out;
    if (settings_.lfo_master_amp_enable) dca_mod_val *= lfo_out;

    dca_.SetEgMod(dca_mod_val);
    dca_.Update();

    ///////////

    double osc1_mod_val = 1;
    double osc2_mod_val = 1;

    if (settings_.eg_osc1_pitch_enable && settings_.lfo_osc1_pitch_enable)
      osc1_mod_val = eg_out * lfo_out;
    else if (settings_.eg_osc1_pitch_enable)
      osc1_mod_val = eg_out;
    else if (settings_.lfo_osc1_pitch_enable)
      osc1_mod_val = lfo_out;

    if (settings_.eg_osc2_pitch_enable && settings_.lfo_osc2_pitch_enable)
      osc2_mod_val = eg_out * lfo_out;
    else if (settings_.eg_osc2_pitch_enable)
      osc2_mod_val = eg_out;
    else if (settings_.lfo_osc2_pitch_enable)
      osc2_mod_val = lfo_out;

    if (settings_.eg_filter1_freq_enable) {
      filter1_->SetFcMod(eg_out * settings_.pitch_range);
    }
    if (settings_.eg_filter1_q_enable) {
      filter1_->m_q_control +=
          (eg_out * settings_.q_range) - settings_.q_range / 2;
    }
    if (settings_.eg_filter2_freq_enable) {
      filter2_->SetFcMod(eg_out * settings_.pitch_range);
    }
    if (settings_.eg_filter2_q_enable) {
      filter2_->m_q_control +=
          (eg_out * settings_.q_range) - settings_.q_range / 2;
    }
    if (settings_.eg_master_filter_freq_enable) {
      master_filter_->SetFcMod(eg_out * settings_.pitch_range);
    }
    if (settings_.eg_master_filter_q_enable) {
      master_filter_->m_q_control +=
          (eg_out * settings_.q_range) - settings_.q_range / 2;
    }

    if (settings_.lfo_pw_enable) {
      // --- limits are 2% and 98%
      double pulse_width = settings_.pulse_width_pct +
                           lfo_out * (OSC_PULSEWIDTH_MAX - OSC_PULSEWIDTH_MIN) /
                               OSC_PULSEWIDTH_MIN;

      // --- bound the PWM to the range
      pulse_width = fmin(pulse_width, OSC_PULSEWIDTH_MAX);
      pulse_width = fmax(pulse_width, OSC_PULSEWIDTH_MIN);
      osc1_->m_pulse_width = pulse_width;
      osc2_->m_pulse_width = pulse_width;
    }

    // note - if ENV and LFO are enabled, LFO mod will overwrite ENV
    if (settings_.lfo_filter1_freq_enable) {
      filter1_->SetFcMod(lfo_out * settings_.pitch_range);
    }
    if (settings_.lfo_filter1_q_enable) {
      filter1_->m_q_control +=
          (lfo_out * settings_.q_range) - settings_.q_range / 2;
    }
    if (settings_.lfo_filter2_freq_enable) {
      filter2_->SetFcMod(lfo_out * settings_.pitch_range);
    }
    if (settings_.lfo_filter2_q_enable) {
      filter2_->m_q_control +=
          (lfo_out * settings_.q_range) - settings_.q_range / 2;
    }
    if (settings_.lfo_master_filter_freq_enable) {
      master_filter_->SetFcMod(lfo_out * settings_.pitch_range);
    }
    if (settings_.lfo_master_filter_q_enable) {
      master_filter_->m_q_control +=
          (lfo_out * settings_.q_range) - settings_.q_range / 2;
    }

    filter1_->Update();
    filter2_->Update();
    master_filter_->Update();

    osc1_->SetFoModLin(osc1_mod_val);
    if (settings_.pitch_bend)
      osc1_->m_osc_fo = base_frequency_ * settings_.osc1_ratio +
                        osc1_mod_val * frequency_diff_;
    osc1_->Update();

    osc2_->SetFoModLin(osc2_mod_val);
    if (settings_.pitch_bend)
      osc2_->m_osc_fo = base_frequency_ * settings_.osc2_ratio +
                        osc2_mod_val * frequency_diff_;
    osc2_->Update();

    osc3_->Update();
    double osc3_out = osc3_->DoOscillate(nullptr) * settings_.osc3_amp;

    double osc_mix = 0.;
    if (settings_.hard_sync) {
      osc1_->DoOscillate(nullptr);
      if (osc1_->just_wrapped) osc2_->StartOscillator();
      double osc2_out = osc2_->DoOscillate(nullptr) * settings_.osc2_amp;
      if (settings_.filter2_enable) {
        osc2_out = filter2_->DoFilter(osc2_out);
      }
      osc_mix = 0.666 * osc2_out + 0.333 * osc3_out;
    } else {
      double osc1_out = osc1_->DoOscillate(nullptr) * settings_.osc1_amp;
      if (settings_.filter1_enable) {
        osc1_out = filter1_->DoFilter(osc1_out);
      }
      double osc2_out = osc2_->DoOscillate(nullptr) * settings_.osc2_amp;
      if (settings_.filter2_enable) {
        osc2_out = filter2_->DoFilter(osc2_out);
      }
      osc_mix = 0.333 * osc1_out + 0.333 * osc2_out + 0.333 * osc3_out;
    }

    if (settings_.master_filter_enable) {
      osc_mix = master_filter_->DoFilter(osc_mix);
    }

    //////////////////////////////

    double out_left = 0.0;
    double out_right = 0.0;

    dca_.DoDCA(osc_mix, osc_mix, &out_left, &out_right);

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
  } else if (name == "pb") {
    settings_.pitch_bend = val;
  } else if (name == "pitch_range") {
    settings_.pitch_range = val;
  } else if (name == "q_range") {
    settings_.q_range = val;
  } else if (name == "sync") {
    settings_.hard_sync = val;
  } else if (name == "detune") {
    if (val >= -100 && val <= 100) settings_.detune_cents = val;
  } else if (name == "pw") {
    if (val > 0 && val < 100) settings_.pulse_width_pct = val;
  } else if (name == "osc1") {
    settings_.osc1_wav = val;
    osc1_->m_waveform = val;
  } else if (name == "o1amp") {
    settings_.osc1_amp = val;
  } else if (name == "o1ratio") {
    settings_.osc1_ratio = val;
  } else if (name == "f1_type") {
    settings_.filter1_type = val;
    filter1_->SetType(val);
  } else if (name == "f1_fc") {
    settings_.filter1_fc = val;
    filter1_->SetFcControl(val);
  } else if (name == "f1_q") {
    settings_.filter1_q = val;
    filter1_->SetQControlGUI(val);
  } else if (name == "f1_en") {
    settings_.filter1_enable = val;
  }

  else if (name == "osc2") {
    settings_.osc2_wav = val;
    osc2_->m_waveform = val;
  } else if (name == "o2amp") {
    settings_.osc2_amp = val;
  } else if (name == "o2ratio") {
    settings_.osc2_ratio = val;
  } else if (name == "f2_type") {
    settings_.filter2_type = val;
    filter2_->SetType(val);
  } else if (name == "f2_fc") {
    settings_.filter2_fc = val;
    filter2_->SetFcControl(val);
  } else if (name == "f2_q") {
    settings_.filter2_q = val;
    filter2_->SetQControl(val);
  } else if (name == "f2_en") {
    settings_.filter2_enable = val;
  }

  else if (name == "osc3") {
    settings_.osc3_wav = val;
    osc3_->m_waveform = val;
  } else if (name == "o3amp") {
    settings_.osc3_amp = val;
  } else if (name == "o3ratio") {
    settings_.osc3_ratio = val;
  } else if (name == "f3_type") {
    settings_.filter3_type = val;
    filter3_->SetType(val);
  } else if (name == "f3_fc") {
    settings_.filter3_fc = val;
    filter3_->SetFcControl(val);
  } else if (name == "f3_q") {
    settings_.filter3_q = val;
    filter3_->SetQControl(val);
  } else if (name == "f3_en") {
    settings_.filter3_enable = val;
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
  } else if (name == "eg_ramp") {
    settings_.eg_ramp_mode = val;
    eg_.SetRampMode(val);
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
    settings_.eg_osc1_pitch_enable = val;
  } else if (name == "eg_o2_ptch") {
    settings_.eg_osc2_pitch_enable = val;
  } else if (name == "eg_o3_ptch") {
    settings_.eg_osc3_pitch_enable = val;
  } else if (name == "eg_f1_fc") {
    settings_.eg_filter1_freq_enable = val;
  } else if (name == "eg_f1_q") {
    settings_.eg_filter1_q_enable = val;
  } else if (name == "eg_f2_fc") {
    settings_.eg_filter2_freq_enable = val;
  } else if (name == "eg_f2_q") {
    settings_.eg_filter2_q_enable = val;
  } else if (name == "eg_f3_fc") {
    settings_.eg_filter3_freq_enable = val;
  } else if (name == "eg_f3_q") {
    settings_.eg_filter3_q_enable = val;
  } else if (name == "eg_master_fc") {
    settings_.eg_master_filter_freq_enable = val;
  } else if (name == "eg_master_q") {
    settings_.eg_master_filter_q_enable = val;
  }

  else if (name == "lfo_amp") {
    settings_.lfo_master_amp_enable = val;
  } else if (name == "lfo_pw") {
    settings_.lfo_pw_enable = val;
  } else if (name == "lfo_osc1") {
    settings_.lfo_osc1_pitch_enable = val;
  } else if (name == "lfo_osc2") {
    settings_.lfo_osc2_pitch_enable = val;
  } else if (name == "lfo_filter1_fc") {
    settings_.lfo_filter1_freq_enable = val;
  } else if (name == "lfo_filter1_q") {
    settings_.lfo_filter1_q_enable = val;
  } else if (name == "lfo_filter2_fc") {
    settings_.lfo_filter2_freq_enable = val;
  } else if (name == "lfo_filter2_q") {
    settings_.lfo_filter2_q_enable = val;
  } else if (name == "lfo_master_fc") {
    settings_.lfo_master_filter_freq_enable = val;
  } else if (name == "lfo_master_q") {
    settings_.lfo_master_filter_q_enable = val;
  } else if (name == "master_filter_en") {
    settings_.master_filter_enable = val;
  } else if (name == "mf_type") {
    settings_.master_filter_type = val;
  } else if (name == "mf_fc") {
    settings_.master_filter_fc = val;
  } else if (name == "mf_q") {
    settings_.master_filter_q = val;
  }

  Update();
}

std::string DrumSynth::Info() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    //    ss << ANSI_COLOR_CYAN;
    ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "DrumZynth - " << COOL_COLOR_PINK2 << settings_.name
     << COOL_COLOR_YELLOW_MELLOW << " - vol:" << volume << std::endl;
  ss << "     distort:" << settings_.distortion_threshold
     << " pitch_range:" << settings_.pitch_range
     << " q_range:" << settings_.q_range << " sync:" << settings_.hard_sync
     << " detune:" << settings_.detune_cents << " osc2.cents:" << osc2_->m_cents
     << " pw:" << settings_.pulse_width_pct << " pb:" << settings_.pitch_bend
     << std::endl;
  ss << "     osc1:" << GetOscType(settings_.osc1_wav) << "("
     << settings_.osc1_wav << ")"
     << " o1amp:" << settings_.osc1_amp << " o1ratio:" << settings_.osc1_ratio
     << " f1_type:" << k_filter_type_names[settings_.filter1_type] << "("
     << settings_.filter1_type << ")"
     << " f1_fc:" << settings_.filter1_fc << " f1_q:" << settings_.filter1_q
     << " f1_en:" << settings_.filter1_enable << std::endl;
  ss << "     osc2:" << GetOscType(settings_.osc2_wav) << "("
     << settings_.osc2_wav << ")"
     << " o2amp:" << settings_.osc2_amp << " o2ratio:" << settings_.osc2_ratio
     << " f2_type:" << k_filter_type_names[settings_.filter2_type] << "("
     << settings_.filter2_type << ")"
     << " f2_fc:" << settings_.filter2_fc << " f2_q:" << settings_.filter2_q
     << " f2_en:" << settings_.filter2_enable << std::endl;
  ss << "     osc3:" << GetOscType(settings_.osc3_wav) << "("
     << settings_.osc3_wav << ")"
     << " o3amp:" << settings_.osc3_amp << " o3ratio:" << settings_.osc3_ratio
     << " f3_type:" << k_filter_type_names[settings_.filter3_type] << "("
     << settings_.filter3_type << ")"
     << " f3_fc:" << settings_.filter3_fc << " f3_q:" << settings_.filter3_q
     << " f3_en:" << settings_.filter3_enable << std::endl;
  ss << COOL_COLOR_PINK2 << "     eg_attack:" << settings_.eg_attack_ms
     << " eg_decay:" << settings_.eg_decay_ms
     << " eg_sustain:" << settings_.eg_sustain_level
     << " eg_release:" << settings_.eg_release_ms
     << " eg_hold:" << settings_.eg_hold_time_ms
     << " eg_ramp:" << settings_.eg_ramp_mode << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW
     << "     eg_o1_ptch:" << settings_.eg_osc1_pitch_enable << std::endl;
  ss << "     eg_o2_ptch:" << settings_.eg_osc2_pitch_enable << std::endl;
  ss << "     eg_o3_ptch:" << settings_.eg_osc3_pitch_enable << std::endl;
  ss << "     eg_f1_fc:" << settings_.eg_filter1_freq_enable << std::endl;
  ss << "     eg_f1_q:" << settings_.eg_filter1_q_enable << std::endl;
  ss << "     eg_f2_fc:" << settings_.eg_filter2_freq_enable << std::endl;
  ss << "     eg_f2_q:" << settings_.eg_filter2_q_enable << std::endl;
  ss << "     eg_f3_fc:" << settings_.eg_filter3_freq_enable << std::endl;
  ss << "     eg_f3_q:" << settings_.eg_filter3_q_enable << std::endl;
  ss << "     eg_master_fc:" << settings_.eg_master_filter_freq_enable
     << std::endl;
  ss << "     eg_master_q:" << settings_.eg_master_filter_q_enable << std::endl;
  ss << COOL_COLOR_PINK2
     << "     lfowav:" << k_lfo_wave_names[settings_.lfo_wave] << "("
     << settings_.lfo_wave << ")"
     << " lfomode:" << k_lfo_mode_names[settings_.lfo_mode] << "("
     << settings_.lfo_mode << ")"
     << " lforate:" << settings_.lfo_rate << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW
     << "     lfo_amp:" << settings_.lfo_master_amp_enable << std::endl;
  ss << "     lfo_pw:" << settings_.lfo_pw_enable << std::endl;
  ss << "     lfo_osc1:" << settings_.lfo_osc1_pitch_enable << std::endl;
  ss << "     lfo_osc2:" << settings_.lfo_osc2_pitch_enable << std::endl;
  ss << "     lfo_osc3:" << settings_.lfo_osc3_pitch_enable << std::endl;
  ss << "     lfo_filter1_fc:" << settings_.lfo_filter1_freq_enable
     << std::endl;
  ss << "     lfo_filter1_q:" << settings_.lfo_filter1_q_enable << std::endl;
  ss << "     lfo_filter2_fc:" << settings_.lfo_filter2_freq_enable
     << std::endl;
  ss << "     lfo_filter2_q:" << settings_.lfo_filter2_q_enable << std::endl;
  ss << "     lfo_filter3_fc:" << settings_.lfo_filter3_freq_enable
     << std::endl;
  ss << "     lfo_filter3_q:" << settings_.lfo_filter3_q_enable << std::endl;
  ss << "     lfo_master_fc:" << settings_.lfo_master_filter_freq_enable
     << std::endl;
  ss << "     lfo_master_q:" << settings_.lfo_master_filter_q_enable
     << std::endl;
  ss << COOL_COLOR_PINK2
     << "     master_filter_en:" << settings_.master_filter_enable
     << " mf_type:" << k_filter_type_names[settings_.master_filter_type] << "("
     << settings_.master_filter_type << ")"
     << " mf_fc:" << settings_.master_filter_fc
     << " mf_q:" << settings_.master_filter_q << std::endl;

  return ss.str();
}

std::string DrumSynth::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "DrumZynth - " << COOL_COLOR_PINK2 << settings_.name
     << COOL_COLOR_YELLOW_MELLOW << " - vol:" << volume;

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

void DrumSynth::NoteOff(midi_event ev) { eg_.NoteOff(); }

void DrumSynth::NoteOn(midi_event ev) {
  unsigned int midinote = ev.data1;
  unsigned int velocity = ev.data2;
  settings_.amplitude = scaleybum(0, 127, 0, 1, velocity);

  // if (ev.dur != 240) {
  //   settings_.eg_release_ms = ms_per_midi_tick_ * ev.dur;
  // }

  base_frequency_ = Midi2Freq(midinote);
  starting_frequency_ = Midi2Freq(midinote + settings_.pitch_range);
  frequency_diff_ = starting_frequency_ - base_frequency_;

  // if (!osc1_->m_note_on) {
  if (settings_.pitch_bend)
    osc1_->m_osc_fo = starting_frequency_;
  else
    osc1_->m_osc_fo = base_frequency_;
  osc1_->StartOscillator();

  if (settings_.pitch_bend)
    osc2_->m_osc_fo = starting_frequency_;
  else
    osc2_->m_osc_fo = base_frequency_;
  osc2_->StartOscillator();

  osc3_->m_osc_fo = base_frequency_;
  osc3_->StartOscillator();

  lfo_.StartOscillator();
  //}

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
  presetzzz.open(DRUM_PRESET_FILENAME, std::ios::app);
  presetzzz << "name=" << settings_.name << kSEP;
  presetzzz << "distortion_threshold=" << settings_.distortion_threshold
            << kSEP;
  presetzzz << "pitch_range=" << settings_.pitch_range << kSEP;
  presetzzz << "q_range=" << settings_.q_range << kSEP;
  presetzzz << "sync=" << settings_.hard_sync << kSEP;
  presetzzz << "detune=" << settings_.detune_cents << kSEP;
  presetzzz << "pw=" << settings_.pulse_width_pct << kSEP;
  presetzzz << "pb=" << settings_.pitch_bend << kSEP;
  presetzzz << "osc1_wav=" << settings_.osc1_wav << kSEP;
  presetzzz << "osc1_amp=" << settings_.osc1_amp << kSEP;
  presetzzz << "osc1_ratio=" << settings_.osc1_ratio << kSEP;
  presetzzz << "filter1_enable=" << settings_.filter1_enable << kSEP;
  presetzzz << "filter1_type=" << settings_.filter1_type << kSEP;
  presetzzz << "filter1_fc=" << settings_.filter1_fc << kSEP;
  presetzzz << "filter1_q=" << settings_.filter1_q << kSEP;
  presetzzz << "osc2_wav=" << settings_.osc2_wav << kSEP;
  presetzzz << "osc2_amp=" << settings_.osc2_amp << kSEP;
  presetzzz << "osc2_ratio=" << settings_.osc2_ratio << kSEP;
  presetzzz << "filter2_enable=" << settings_.filter2_enable << kSEP;
  presetzzz << "filter2_type=" << settings_.filter2_type << kSEP;
  presetzzz << "filter2_fc=" << settings_.filter2_fc << kSEP;
  presetzzz << "filter2_q=" << settings_.filter2_q << kSEP;
  presetzzz << "osc3_wav=" << settings_.osc3_wav << kSEP;
  presetzzz << "osc3_amp=" << settings_.osc3_amp << kSEP;
  presetzzz << "osc3_ratio=" << settings_.osc3_ratio << kSEP;
  presetzzz << "filter3_enable=" << settings_.filter3_enable << kSEP;
  presetzzz << "filter3_type=" << settings_.filter3_type << kSEP;
  presetzzz << "filter3_fc=" << settings_.filter3_fc << kSEP;
  presetzzz << "filter3_q=" << settings_.filter3_q << kSEP;
  presetzzz << "master_filter_enable=" << settings_.master_filter_enable
            << kSEP;
  presetzzz << "master_filter_type=" << settings_.master_filter_type << kSEP;
  presetzzz << "master_filter_fc=" << settings_.master_filter_fc << kSEP;
  presetzzz << "master_filter_q=" << settings_.master_filter_q << kSEP;
  presetzzz << "eg_attack_ms=" << settings_.eg_attack_ms << kSEP;
  presetzzz << "eg_decay_ms=" << settings_.eg_decay_ms << kSEP;
  presetzzz << "eg_sustain_level=" << settings_.eg_sustain_level << kSEP;
  presetzzz << "eg_release_ms=" << settings_.eg_release_ms << kSEP;
  presetzzz << "eg_hold_time_ms=" << settings_.eg_hold_time_ms << kSEP;
  presetzzz << "eg_ramp_mode=" << settings_.eg_ramp_mode << kSEP;
  presetzzz << "lfo_wave=" << settings_.lfo_wave << kSEP;
  presetzzz << "lfo_mode=" << settings_.lfo_mode << kSEP;
  presetzzz << "lfo_rate=" << settings_.lfo_rate << kSEP;

  presetzzz << "eg_osc1_pitch_enable=" << settings_.eg_osc1_pitch_enable
            << kSEP;
  presetzzz << "eg_osc2_pitch_enable=" << settings_.eg_osc2_pitch_enable
            << kSEP;
  presetzzz << "eg_osc3_pitch_enable=" << settings_.eg_osc3_pitch_enable
            << kSEP;
  presetzzz << "eg_filter1_freq_enable=" << settings_.eg_filter1_freq_enable
            << kSEP;
  presetzzz << "eg_filter1_q_enable=" << settings_.eg_filter1_q_enable << kSEP;
  presetzzz << "eg_filter2_freq_enable=" << settings_.eg_filter2_freq_enable
            << kSEP;
  presetzzz << "eg_filter2_q_enable=" << settings_.eg_filter2_q_enable << kSEP;
  presetzzz << "eg_filter3_freq_enable=" << settings_.eg_filter3_freq_enable
            << kSEP;
  presetzzz << "eg_filter3_q_enable=" << settings_.eg_filter3_q_enable << kSEP;
  presetzzz << "eg_master_freq_enable="
            << settings_.eg_master_filter_freq_enable << kSEP;
  presetzzz << "eg_master_q_enable=" << settings_.eg_master_filter_q_enable
            << kSEP;

  presetzzz << "lfo_master_amp_enable=" << settings_.lfo_master_amp_enable
            << kSEP;
  presetzzz << "lfo_pw_enable=" << settings_.lfo_pw_enable << kSEP;
  presetzzz << "lfo_osc1_pitch_enable=" << settings_.lfo_osc1_pitch_enable
            << kSEP;
  presetzzz << "lfo_osc2_pitch_enable=" << settings_.lfo_osc2_pitch_enable
            << kSEP;
  presetzzz << "lfo_filter1_freq_enable=" << settings_.lfo_filter1_freq_enable
            << kSEP;
  presetzzz << "lfo_filter1_q_enable=" << settings_.lfo_filter1_q_enable
            << kSEP;
  presetzzz << "lfo_filter2_freq_enable=" << settings_.lfo_filter2_freq_enable
            << kSEP;
  presetzzz << "lfo_filter2_q_enable=" << settings_.lfo_filter2_q_enable
            << kSEP;
  presetzzz << "lfo_filter3_freq_enable=" << settings_.lfo_filter3_freq_enable
            << kSEP;
  presetzzz << "lfo_filter3_q_enable=" << settings_.lfo_filter3_q_enable
            << kSEP;

  presetzzz << std::endl;
  presetzzz.close();
  std::cout << "DRUMSYNTH -- saving -- DONE" << std::endl;
}

void DrumSynth::Update() {
  distortion_.SetParam("threshold", settings_.distortion_threshold);

  osc1_->m_waveform = settings_.osc1_wav;
  osc1_->m_amplitude = settings_.osc1_amp;
  osc1_->m_fo_ratio = settings_.osc1_ratio;
  osc1_->m_pulse_width_control = settings_.pulse_width_pct;
  osc1_->Update();

  osc2_->m_waveform = settings_.osc2_wav;
  osc2_->m_amplitude = settings_.osc2_amp;
  osc2_->m_fo_ratio = settings_.osc2_ratio;
  osc2_->m_pulse_width_control = settings_.pulse_width_pct;
  osc2_->m_cents = settings_.detune_cents;
  osc2_->Update();

  // noise
  osc3_->m_waveform = settings_.osc3_wav;
  osc3_->m_amplitude = settings_.osc3_amp;
  osc3_->m_fo_ratio = settings_.osc3_ratio;
  osc3_->Update();

  filter1_->SetFcControl(settings_.filter1_fc);
  filter1_->SetQControl(settings_.filter1_q);

  filter2_->SetFcControl(settings_.filter2_fc);
  filter2_->SetQControl(settings_.filter2_q);

  master_filter_->SetFcControl(settings_.master_filter_fc);
  master_filter_->SetQControl(settings_.master_filter_q);

  eg_.SetAttackTimeMsec(settings_.eg_attack_ms);
  eg_.SetDecayTimeMsec(settings_.eg_decay_ms);
  eg_.SetSustainLevel(settings_.eg_sustain_level);
  eg_.SetReleaseTimeMsec(settings_.eg_release_ms);
  eg_.SetRampMode(settings_.eg_ramp_mode);

  lfo_.m_waveform = settings_.lfo_wave;
  lfo_.m_lfo_mode = settings_.lfo_mode;
  lfo_.m_fo = settings_.lfo_rate;

  filter1_->Update();
  filter2_->Update();
  master_filter_->Update();
  eg_.Update();
  lfo_.Update();
}

void DrumSynth::LoadSettings(DrumSettings settings) {
  settings_ = settings;
  Update();
}

void DrumSynth::PrintSettings(DrumSettings settingz) {
  std::cout << "AAIIGht, settings for" << settingz.name << std::endl;

  std::cout << "dist:" << settingz.distortion_threshold << std::endl;
  std::cout << "amp:" << settingz.amplitude << std::endl;

  std::cout << "pitch range:" << settingz.pitch_range << std::endl;
  std::cout << "q range" << settingz.q_range << std::endl;

  std::cout << "osc1wave:" << settingz.osc1_wav << std::endl;
  std::cout << "osc1amp:" << settingz.osc1_amp << std::endl;

  std::cout << "filter1en:" << settingz.filter1_enable << std::endl;
  std::cout << "f1type:" << settingz.filter1_type << std::endl;
  std::cout << "f1fc:" << settingz.filter1_fc << std::endl;
  std::cout << "f1q:" << settingz.filter1_q << std::endl;
  std::cout << "osc1_rat:" << settingz.osc1_ratio << std::endl;

  std::cout << "osc2wave:" << settingz.osc2_wav << std::endl;
  std::cout << "osc2amp:" << settingz.osc2_amp << std::endl;
  std::cout << "filter2en:" << settingz.filter2_enable << std::endl;
  std::cout << "f2type:" << settingz.filter2_type << std::endl;
  std::cout << "f2fc:" << settingz.filter2_fc << std::endl;
  std::cout << "f2q:" << settingz.filter2_q << std::endl;
  std::cout << "osc2_rat:" << settingz.osc2_ratio << std::endl;

  std::cout << "master_filter_enable:" << settingz.master_filter_enable
            << std::endl;
  std::cout << "mf_type:" << settingz.master_filter_type << std::endl;
  std::cout << "mf_fc:" << settingz.master_filter_fc << std::endl;
  std::cout << "mf_q:" << settingz.master_filter_q << std::endl;

  std::cout << "eg_att:" << settingz.eg_attack_ms << std::endl;
  std::cout << "eg_dec:" << settingz.eg_decay_ms << std::endl;
  std::cout << "eg_sus:" << settingz.eg_sustain_level << std::endl;
  std::cout << "eg_rel:" << settingz.eg_release_ms << std::endl;
  std::cout << "eg_hold:" << settingz.eg_hold_time_ms << std::endl;
  std::cout << "eg_ramp:" << settingz.eg_ramp_mode << std::endl;

  std::cout << "lfo:" << settingz.lfo_wave << std::endl;
  std::cout << "lfomode:" << settingz.lfo_mode << std::endl;
  std::cout << "lforate" << settingz.lfo_rate << std::endl;

  std::cout << "eg_osc1_pitch_enable:" << settings_.eg_osc1_pitch_enable
            << std::endl;
  std::cout << "eg_osc2_pitch_enable:" << settings_.eg_osc2_pitch_enable
            << std::endl;
  std::cout << "eg_filter1_freq_enable:" << settings_.eg_filter1_freq_enable
            << std::endl;
  std::cout << "eg_filter1_q_enable:" << settings_.eg_filter1_q_enable
            << std::endl;
  std::cout << "eg_filter2_freq_enable:" << settings_.eg_filter2_freq_enable
            << std::endl;
  std::cout << "eg_filter2_q_enable:" << settings_.eg_filter2_q_enable
            << std::endl;
  std::cout << "lfo_master_amp_enable:" << settings_.lfo_master_amp_enable
            << std::endl;
  std::cout << "lfo_osc1_pitch_enable:" << settings_.lfo_osc1_pitch_enable
            << std::endl;
  std::cout << "lfo_osc2_pitch_enable:" << settings_.lfo_osc2_pitch_enable
            << std::endl;
  std::cout << "lfo_filter1_freq_enable:" << settings_.lfo_filter1_freq_enable
            << std::endl;
  std::cout << "lfo_filter1_q_enable:" << settings_.lfo_filter1_q_enable
            << std::endl;
  std::cout << "lfo_filter2_freq_enable:" << settings_.lfo_filter2_freq_enable
            << std::endl;
  std::cout << "lfo_filter2_q_enable:" << settings_.lfo_filter2_q_enable
            << std::endl;
}

void DrumSynth::Randomize() {
  DrumSettings rand_settings;
  rand_settings.name = "RANDWOOF";

  rand_settings.distortion_threshold = 0.7;
  rand_settings.amplitude = 1;

  rand_settings.pitch_bend = BoolGen();
  rand_settings.pitch_range = rand() % 30 + 10;
  rand_settings.q_range = rand() % 7 + 1;

  rand_settings.osc1_wav = rand() % MAX_OSC;
  rand_settings.osc1_amp = (float)rand() / RAND_MAX;
  rand_settings.filter1_enable = BoolGen();
  rand_settings.filter1_type = rand() % NUM_FILTER_TYPES;
  rand_settings.filter1_fc = rand() % 10000 + 2000;
  rand_settings.filter1_q = rand() % 8 + 1;
  rand_settings.osc1_ratio = rand() % 4 + 1;

  rand_settings.osc2_wav = rand() % MAX_OSC;
  rand_settings.osc2_amp = (float)rand() / RAND_MAX;
  rand_settings.filter2_enable = BoolGen();
  rand_settings.filter2_type = rand() % NUM_FILTER_TYPES;
  rand_settings.filter2_fc = rand() % 10000 + 2000;
  rand_settings.filter2_q = rand() % 8 + 1;
  rand_settings.osc2_ratio = rand() % 4 + 1;

  rand_settings.osc3_wav = rand() % MAX_OSC;
  rand_settings.osc3_amp = (float)rand() / RAND_MAX;
  rand_settings.filter3_enable = BoolGen();
  rand_settings.filter3_type = rand() % NUM_FILTER_TYPES;
  rand_settings.filter3_fc = rand() % 10000 + 3000;
  rand_settings.filter3_q = rand() % 8 + 1;
  rand_settings.osc3_ratio = rand() % 4 + 1;

  // rand_settings.master_filter_enable = BoolGen();
  // rand_settings.master_filter_type = rand() % NUM_FILTER_TYPES;
  // rand_settings.master_filter_fc = rand() % 10000 + 2000;
  // rand_settings.master_filter_q = rand() % 8 + 1;

  rand_settings.eg_attack_ms = rand() % 100;
  rand_settings.eg_decay_ms = rand() % 100;
  rand_settings.eg_sustain_level = (float)rand() / RAND_MAX;
  rand_settings.eg_release_ms = rand() % 300;
  rand_settings.eg_hold_time_ms = 0;
  rand_settings.eg_ramp_mode = BoolGen();

  rand_settings.lfo_wave = rand() % MAX_OSC;
  rand_settings.lfo_mode = rand() % LFO_MAX_MODE;
  rand_settings.lfo_rate =
      ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) + MIN_LFO_RATE;

  rand_settings.eg_osc1_pitch_enable = BoolGen();
  rand_settings.eg_osc2_pitch_enable = BoolGen();
  rand_settings.eg_filter1_freq_enable = BoolGen();
  rand_settings.eg_filter1_q_enable = BoolGen();
  rand_settings.eg_filter2_freq_enable = BoolGen();
  rand_settings.eg_filter2_q_enable = BoolGen();
  rand_settings.eg_master_filter_freq_enable = BoolGen();
  rand_settings.eg_master_filter_q_enable = BoolGen();

  rand_settings.lfo_master_amp_enable = BoolGen();
  rand_settings.lfo_osc1_pitch_enable = BoolGen();
  rand_settings.lfo_osc2_pitch_enable = BoolGen();
  rand_settings.lfo_filter1_freq_enable = BoolGen();
  rand_settings.lfo_filter1_q_enable = BoolGen();
  rand_settings.lfo_filter2_freq_enable = BoolGen();
  rand_settings.lfo_filter2_q_enable = BoolGen();
  rand_settings.lfo_master_filter_freq_enable = BoolGen();
  rand_settings.lfo_master_filter_q_enable = BoolGen();

  PrintSettings(rand_settings);
  LoadSettings(rand_settings);
}

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
    if (key == "distortion_threshold")
      preset.distortion_threshold = dval;
    else if (key == "pb")
      preset.pitch_bend = dval;
    else if (key == "pitch_range")
      preset.pitch_range = dval;
    else if (key == "q_range")
      preset.q_range = dval;
    else if (key == "sync")
      preset.hard_sync = dval;
    else if (key == "detune")
      preset.detune_cents = dval;
    else if (key == "pw")
      preset.pulse_width_pct = dval;
    else if (key == "osc1_wav")
      preset.osc1_wav = dval;
    else if (key == "osc1_amp")
      preset.osc1_amp = dval;
    else if (key == "osc1_ratio")
      preset.osc1_ratio = dval;
    else if (key == "filter1_enable")
      preset.filter1_enable = dval;
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
    else if (key == "osc2_ratio")
      preset.osc2_ratio = dval;
    else if (key == "filter2_enable")
      preset.filter2_enable = dval;
    else if (key == "filter2_type")
      preset.filter2_type = dval;
    else if (key == "filter2_fc")
      preset.filter2_fc = dval;
    else if (key == "filter2_q")
      preset.filter2_q = dval;
    else if (key == "osc3_wav")
      preset.osc3_wav = dval;
    else if (key == "osc3_amp")
      preset.osc3_amp = dval;
    else if (key == "osc3_ratio")
      preset.osc3_ratio = dval;
    else if (key == "filter3_enable")
      preset.filter3_enable = dval;
    else if (key == "filter3_type")
      preset.filter3_type = dval;
    else if (key == "filter3_fc")
      preset.filter3_fc = dval;
    else if (key == "filter3_q")
      preset.filter3_q = dval;
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

    else if (key == "eg_osc1_pitch_enable")
      preset.eg_osc1_pitch_enable = dval;
    else if (key == "eg_osc2_pitch_enable")
      preset.eg_osc2_pitch_enable = dval;
    else if (key == "eg_osc3_pitch_enable")
      preset.eg_osc3_pitch_enable = dval;
    else if (key == "eg_filter1_freq_enable")
      preset.eg_filter1_freq_enable = dval;
    else if (key == "eg_filter1_q_enable")
      preset.eg_filter1_q_enable = dval;
    else if (key == "eg_filter2_freq_enable")
      preset.eg_filter2_freq_enable = dval;
    else if (key == "eg_filter2_q_enable")
      preset.eg_filter2_q_enable = dval;
    else if (key == "eg_filter3_freq_enable")
      preset.eg_filter3_freq_enable = dval;
    else if (key == "eg_filter3_q_enable")
      preset.eg_filter3_q_enable = dval;
    else if (key == "eg_master_filter_freq_enable")
      preset.eg_master_filter_freq_enable = dval;
    else if (key == "eg_master_filter_q_enable")
      preset.eg_master_filter_q_enable = dval;

    else if (key == "lfo_master_amp_enable")
      preset.lfo_master_amp_enable = dval;
    else if (key == "lfo_pw_enable")
      preset.lfo_pw_enable = dval;
    else if (key == "lfo_osc1_pitch_enable")
      preset.lfo_osc1_pitch_enable = dval;
    else if (key == "lfo_osc2_pitch_enable")
      preset.lfo_osc2_pitch_enable = dval;
    else if (key == "lfo_filter1_freq_enable")
      preset.lfo_filter1_freq_enable = dval;
    else if (key == "lfo_filter1_q_enable")
      preset.lfo_filter1_q_enable = dval;
    else if (key == "lfo_filter2_freq_enable")
      preset.lfo_filter2_freq_enable = dval;
    else if (key == "lfo_filter2_q_enable")
      preset.lfo_filter2_q_enable = dval;
    else if (key == "lfo_master_filter_freq_enable")
      preset.lfo_master_filter_freq_enable = dval;
    else if (key == "lfo_master_filter_q_enable")
      preset.lfo_master_filter_q_enable = dval;

    else if (key == "master_filter_enable")
      preset.master_filter_enable = dval;
    else if (key == "master_filter_type")
      preset.master_filter_type = dval;
    else if (key == "master_filter_fc")
      preset.master_filter_fc = dval;
    else if (key == "master_filter_q")
      preset.master_filter_q = dval;
  }

  return preset;
}

}  // namespace SBAudio
