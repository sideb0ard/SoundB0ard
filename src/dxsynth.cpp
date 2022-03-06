#include <dxsynth.h>
#include <math.h>
#include <midi_freq_table.h>
#include <mixer.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include <iomanip>
#include <iostream>

extern Mixer *mixr;

namespace SBAudio {

static const char *s_dx_dest_names[] = {"dx_dest_none", "dx_dest_amp_mod",
                                        "dx_dest_vibrato"};

static const float dx_ratios[] = {
    0.5,   0.71,  0.78,  0.87,  1.0,   1.41,  1.57,  1.73,  2.,    2.82,  3.,
    3.14,  3.46,  4.,    4.24,  4.71,  5.,    5.19,  5.65,  6.0,   6.28,  6.92,
    7.,    7.07,  7.85,  8.0,   8.48,  8.65,  9.0,   9.42,  9.89,  10.,   10.38,
    10.99, 11.0,  11.30, 12.0,  12.11, 12.56, 12.72, 13.0,  13.84, 14.0,  14.1,
    14.13, 15.,   15.55, 15.37, 15.70, 16.96, 17.27, 17.30, 18.37, 18.84, 19.03,
    19.78, 20.41, 20.76, 21.20, 21.98, 22.49, 23.53, 24.22, 25.95};
static const int NUM_RATIOS = 64;

std::vector<std::string> waveform_names = {"SINE",  "SAW1",   "SAW2",
                                           "SAW3",  "TRI",    "SQUARE",
                                           "NOISE", "PNOISE", "MAX_OSC"};

std::vector<std::string> lfo_wave_names = {"sine", "usaw", "dsaw", "tri ",
                                           "squa", "expo", "rsh ", "qrsh"};

namespace {
float GetRandRatio() { return dx_ratios[rand() % NUM_RATIOS]; }
}  // namespace

DXSynth::DXSynth() {
  type = DXSYNTH_TYPE;
  std::cout << "Added, FM Synth, yo!\n";
  active_midi_osc = 1;
  volume = 0.3;

  Reset();

  for (int i = 0; i < MAX_VOICES; i++) {
    voices_[i] = std::make_shared<DXSynthVoice>();
    voices_[i]->InitGlobalParameters(&global_synth_params);
  }

  // use first voice to setup global
  voices_[0]->InitializeModMatrix(&modmatrix);

  for (auto v : voices_) {
    v->SetModMatrixCore(modmatrix.GetModMatrixCore());
  }

  m_last_note_frequency = -1.0;

  PrepareForPlay();

  Load("RAVER");

  active = true;
}

void DXSynth::start() { active = true; }

void DXSynth::stop() {
  active = false;
  allNotesOff();
}

std::string DXSynth::Status() {
  std::stringstream ss;
  ss << std::setprecision(2) << std::fixed;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << COOL_COLOR_ORANGE;
  ss << "DxSynth(" << m_settings.m_settings_name << ")"
     << " vol:" << volume << " pan:" << pan
     << " algo:" << m_settings.m_voice_mode << ANSI_COLOR_RESET;
  return ss.str();
}

std::string DXSynth::Info() {
  std::stringstream ss;
  ss << std::setprecision(2) << std::fixed;
  char *INSTRUMENT_COLOR_A = (char *)ANSI_COLOR_RESET;
  char *INSTRUMENT_COLOR_B = (char *)ANSI_COLOR_RESET;
  if (active) {
    INSTRUMENT_COLOR_A = (char *)COOL_COLOR_BLUE;
    INSTRUMENT_COLOR_B = (char *)COOL_COLOR_PINK2;
  }

  ss << "\n"
     << ANSI_COLOR_WHITE << "## " << INSTRUMENT_COLOR_B
     << "FM SZYNTHZZZZZZZizzzzer" << ANSI_COLOR_WHITE << " ("
     << INSTRUMENT_COLOR_B << m_settings.m_settings_name << ANSI_COLOR_WHITE
     << ") ####\n";

  ss << ANSI_COLOR_WHITE
     << "## Voice Section ####################################\n";
  ss << INSTRUMENT_COLOR_A;
  ss << "   algo:" << m_settings.m_voice_mode << " vol:" << volume
     << " pan:" << pan << "\n";

  ss << "   midi_osc:" << active_midi_osc
     << " porta:" << m_settings.m_portamento_time_ms
     << " pitchrange:" << m_settings.m_pitchbend_range
     << " op4fb:" << m_settings.m_op4_feedback << "\n";

  ss << "   vel2att:" << m_settings.m_velocity_to_attack_scaling
     << " note2dec:" << m_settings.m_note_number_to_decay_scaling
     << " reset2zero:" << m_settings.m_reset_to_zero
     << " legato:" << m_settings.m_legato_mode << "\n";
  ss << "   op1out:" << m_settings.m_op1_output_lvl
     << " op2out:" << m_settings.m_op2_output_lvl
     << " op3out:" << m_settings.m_op3_output_lvl
     << " op4out:" << m_settings.m_op4_output_lvl << "\n\n";

  ss << ANSI_COLOR_WHITE
     << "## Operator 1 ####################################\n";
  ss << INSTRUMENT_COLOR_B;
  ss << "   o1wav: " << waveform_names[m_settings.m_op1_waveform] << "("
     << m_settings.m_op1_waveform << ") "
     << " o1rat:" << m_settings.m_op1_ratio
     << " o1det:" << m_settings.m_op1_detune_cents
     << " o1sus:" << m_settings.m_op1_sustain_override
     << "\n   e1att:" << m_settings.m_eg1_attack_ms
     << " e1dec:" << m_settings.m_eg1_decay_ms
     << " e1sus:" << m_settings.m_eg1_sustain_lvl
     << " e1rel:" << m_settings.m_eg1_release_ms << "\n";
  ss << "   op1freq:" << voices_[0]->m_op1.m_osc_fo << "\n\n";

  ss << ANSI_COLOR_WHITE
     << "## Operator 2 ####################################\n";
  ss << INSTRUMENT_COLOR_B;
  ss << "   o2wav: " << waveform_names[m_settings.m_op2_waveform] << "("
     << m_settings.m_op2_waveform << ") "
     << " o2rat:" << m_settings.m_op2_ratio
     << " o2det:" << m_settings.m_op2_detune_cents
     << " o2sus:" << m_settings.m_op2_sustain_override
     << "\n   e2att:" << m_settings.m_eg2_attack_ms
     << " e2dec:" << m_settings.m_eg2_decay_ms
     << " e2sus:" << m_settings.m_eg2_sustain_lvl
     << " e2rel:" << m_settings.m_eg2_release_ms << "\n";
  ss << "   op2freq:" << voices_[0]->m_op2.m_osc_fo << "\n\n";

  ss << ANSI_COLOR_WHITE
     << "## Operator 3 ####################################\n";
  ss << INSTRUMENT_COLOR_B;
  ss << "   o3wav: " << waveform_names[m_settings.m_op3_waveform] << "("
     << m_settings.m_op3_waveform << ") "
     << " o3rat:" << m_settings.m_op3_ratio
     << " o3det:" << m_settings.m_op3_detune_cents
     << " o3sus:" << m_settings.m_op3_sustain_override
     << "\n   e3att:" << m_settings.m_eg3_attack_ms
     << " e3dec:" << m_settings.m_eg3_decay_ms
     << " e3sus:" << m_settings.m_eg3_sustain_lvl
     << " e3rel:" << m_settings.m_eg3_release_ms << "\n";
  ss << "   op3freq:" << voices_[0]->m_op3.m_osc_fo << "\n\n";

  ss << ANSI_COLOR_WHITE
     << "## Operator 4 ####################################\n";
  ss << INSTRUMENT_COLOR_B;
  ss << "   o4wav: " << waveform_names[m_settings.m_op4_waveform] << "("
     << m_settings.m_op4_waveform << ") "
     << " o4rat:" << m_settings.m_op4_ratio
     << " o4det:" << m_settings.m_op4_detune_cents
     << " o4sus:" << m_settings.m_op4_sustain_override
     << "\n   e4att:" << m_settings.m_eg4_attack_ms
     << " e4dec:" << m_settings.m_eg4_decay_ms
     << " e4sus:" << m_settings.m_eg4_sustain_lvl
     << " e4rel:" << m_settings.m_eg4_release_ms << "\n";
  ss << "   op4freq:" << voices_[0]->m_op4.m_osc_fo << "\n\n";

  ss << ANSI_COLOR_WHITE
     << "## LFO Routing ####################################\n";
  ss << INSTRUMENT_COLOR_A;
  ss << "   l1_wav:" << lfo_wave_names[m_settings.m_lfo1_waveform] << "("
     << m_settings.m_lfo1_waveform << ")"
     << " l1_int:" << m_settings.m_lfo1_intensity
     << " l1_rate:" << m_settings.m_lfo1_rate << "\n";
  ss << "   l1_dest1:" << s_dx_dest_names[m_settings.m_lfo1_mod_dest1] << "\n";
  ss << "   l1_dest2:" << s_dx_dest_names[m_settings.m_lfo1_mod_dest2] << "\n";
  ss << "   l1_dest3:" << s_dx_dest_names[m_settings.m_lfo1_mod_dest3] << "\n";
  ss << "   l1_dest4:" << s_dx_dest_names[m_settings.m_lfo1_mod_dest4]
     << std::endl;

  return ss.str();
}

stereo_val DXSynth::GenNext(mixer_timing_info tinfo) {
  (void)tinfo;
  if (!active) return (stereo_val){0, 0};

  double accum_out_left = 0.0;
  double accum_out_right = 0.0;

  // float mix = 1.0 / MAX_DX_VOICES;
  float mix = 0.25;

  double out_left = 0.0;
  double out_right = 0.0;

  for (auto v : voices_) {
    v->DoVoice(&out_left, &out_right);

    accum_out_left += mix * out_left;
    accum_out_right += mix * out_right;
  }

  pan = fmin(pan, 1.0);
  pan = fmax(pan, -1.0);
  double pan_left = 0.707;
  double pan_right = 0.707;
  calculate_pan_values(pan, &pan_left, &pan_right);

  stereo_val out = {.left = accum_out_left * volume * pan_left,
                    .right = accum_out_right * volume * pan_right};
  out = Effector(out);
  return out;
}

void DXSynth::SetOpFreq(unsigned int op, float val) {
  for (auto v : voices_) {
    switch (op) {
      case 1:
        if (v->m_osc1) v->m_osc1->m_osc_fo = val;
        break;
      case 2:
        if (v->m_osc2) v->m_osc2->m_osc_fo = val;
        break;
      case 3:
        if (v->m_osc3) v->m_osc3->m_osc_fo = val;
        break;
      case 4:
        if (v->m_osc4) v->m_osc4->m_osc_fo = val;
        break;
    }
  }
  m_last_note_frequency = val;
}

void DXSynth::noteOn(midi_event ev) {
  bool steal_note = true;
  for (auto v : voices_) {
    if (!v->m_note_on) {
      IncrementVoiceTimestamps();
      v->NoteOn(ev.data1, ev.data2, get_midi_freq(ev.data1),
                m_last_note_frequency);

      m_last_note_frequency = get_midi_freq(ev.data1);
      steal_note = false;
      break;
    }
  }
  if (steal_note) {
    auto v = GetOldestVoice();
    if (v) {
      IncrementVoiceTimestamps();
      v->NoteOn(ev.data1, ev.data2, get_midi_freq(ev.data1),
                m_last_note_frequency);
    }
    m_last_note_frequency = get_midi_freq(ev.data1);
  }
}

void DXSynth::allNotesOff() {
  for (auto v : voices_) v->NoteOff(-1);
}

void DXSynth::noteOff(midi_event ev) {
  for (auto v : voices_) v->NoteOff(ev.data1);
}

void DXSynth::ChordOn(midi_event ev) {
  allNotesOff();
  for (unsigned int d : ev.dataz) {
    midi_event note_on = {.event_type = MIDI_ON,
                          .data1 = d,
                          .data2 = ev.data2,
                          .delete_after_use = true,
                          .source = EXTERNAL_OSC};
    noteOn(note_on);
  }
}

void DXSynth::control(midi_event ev) { (void)ev; }

void DXSynth::pitchBend(midi_event ev) {
  unsigned int data1 = ev.data1;
  unsigned int data2 = ev.data2;
  // printf("Pitch bend, babee: %d %d\n", data1, data2);
  int actual_pitch_bent_val = (int)((data1 & 0x7F) | ((data2 & 0x7F) << 7));

  if (actual_pitch_bent_val != 8192) {
    double normalized_pitch_bent_val =
        (float)(actual_pitch_bent_val - 0x2000) / (float)(0x2000);
    double scaley_val =
        // scaleybum(0, 16383, -100, 100, normalized_pitch_bent_val);
        scaleybum(0, 16383, -600, 600, actual_pitch_bent_val);
    // printf("Cents to bend - %f\n", scaley_val);
    for (int i = 0; i < MAX_DX_VOICES; i++) {
      voices_[i]->m_osc1->m_cents = scaley_val;
      voices_[i]->m_osc2->m_cents = scaley_val + 2.5;
      voices_[i]->m_osc3->m_cents = scaley_val;
      voices_[i]->m_osc4->m_cents = scaley_val + 2.5;
      voices_[i]->modmatrix.sources[SOURCE_PITCHBEND] =
          normalized_pitch_bent_val;
    }
  } else {
    for (int i = 0; i < MAX_DX_VOICES; i++) {
      voices_[i]->m_osc1->m_cents = 0;
      voices_[i]->m_osc2->m_cents = 2.5;
      voices_[i]->m_osc3->m_cents = 0;
      voices_[i]->m_osc4->m_cents = 2.5;
    }
  }
}

void DXSynth::Reset() {
  strncpy(m_settings.m_settings_name, "default", 7);
  m_settings.m_volume_db = 0;
  m_settings.m_voice_mode = 0;
  m_settings.m_portamento_time_ms = 0;
  m_settings.m_pitchbend_range = 1;  // 0 -12
  m_settings.m_velocity_to_attack_scaling = 0;
  m_settings.m_note_number_to_decay_scaling = 0;
  m_settings.m_reset_to_zero = 0;
  m_settings.m_legato_mode = 0;

  m_settings.m_lfo1_intensity = 1;
  m_settings.m_lfo1_rate = 0.5;
  m_settings.m_lfo1_waveform = 0;
  m_settings.m_lfo1_mod_dest1 = DX_LFO_DEST_NONE;
  m_settings.m_lfo1_mod_dest2 = DX_LFO_DEST_NONE;
  m_settings.m_lfo1_mod_dest3 = DX_LFO_DEST_NONE;
  m_settings.m_lfo1_mod_dest4 = DX_LFO_DEST_NONE;

  m_settings.m_op1_waveform = SINE;
  m_settings.m_op1_ratio = 1;  // 0.01-10
  m_settings.m_op1_detune_cents = 0;
  m_settings.m_eg1_attack_ms = 100;
  m_settings.m_eg1_decay_ms = 100;
  m_settings.m_eg1_sustain_lvl = 0.707;
  m_settings.m_eg1_release_ms = 2000;
  m_settings.m_op1_output_lvl = 90;

  m_settings.m_op2_waveform = SINE;
  m_settings.m_op2_ratio = 1;  // 0.01-10
  m_settings.m_op2_detune_cents = 0;
  m_settings.m_eg2_attack_ms = 100;
  m_settings.m_eg2_decay_ms = 100;
  m_settings.m_eg2_sustain_lvl = 0.707;
  m_settings.m_eg2_release_ms = 2000;
  m_settings.m_op2_output_lvl = 75;

  m_settings.m_op3_waveform = SINE;
  m_settings.m_op3_ratio = 1;  // 0.01-10
  m_settings.m_op3_detune_cents = 0;
  m_settings.m_eg3_attack_ms = 100;
  m_settings.m_eg3_decay_ms = 100;
  m_settings.m_eg3_sustain_lvl = 0.707;
  m_settings.m_eg3_release_ms = 2000;
  m_settings.m_op3_output_lvl = 75;

  m_settings.m_op4_waveform = SINE;
  m_settings.m_op4_ratio = 1;  // 0.01-10
  m_settings.m_op4_detune_cents = 0;
  m_settings.m_eg4_attack_ms = 100;
  m_settings.m_eg4_decay_ms = 100;
  m_settings.m_eg4_sustain_lvl = 0.707;
  m_settings.m_eg4_release_ms = 2000;
  m_settings.m_op4_output_lvl = 75;
  m_settings.m_op4_feedback = 0;  // 0-70
}

bool DXSynth::PrepareForPlay() {
  for (auto v : voices_) v->PrepareForPlay();

  Update();

  return true;
}

void DXSynth::Update() {
  global_synth_params.voice_params.voice_mode = m_settings.m_voice_mode;
  global_synth_params.voice_params.op4_feedback =
      m_settings.m_op4_feedback / 100.0;
  global_synth_params.voice_params.portamento_time_msec =
      m_settings.m_portamento_time_ms;

  global_synth_params.voice_params.osc_fo_pitchbend_mod_range =
      m_settings.m_pitchbend_range;

  global_synth_params.voice_params.lfo1_osc_mod_intensity =
      m_settings.m_lfo1_intensity;

  global_synth_params.osc1_params.amplitude =
      calculate_dx_amp(m_settings.m_op1_output_lvl);
  global_synth_params.osc2_params.amplitude =
      calculate_dx_amp(m_settings.m_op2_output_lvl);
  global_synth_params.osc3_params.amplitude =
      calculate_dx_amp(m_settings.m_op3_output_lvl);
  global_synth_params.osc4_params.amplitude =
      calculate_dx_amp(m_settings.m_op4_output_lvl);

  global_synth_params.osc1_params.fo_ratio = m_settings.m_op1_ratio;
  global_synth_params.osc2_params.fo_ratio = m_settings.m_op2_ratio;
  global_synth_params.osc3_params.fo_ratio = m_settings.m_op3_ratio;
  global_synth_params.osc4_params.fo_ratio = m_settings.m_op4_ratio;

  global_synth_params.osc1_params.waveform = m_settings.m_op1_waveform;
  global_synth_params.osc2_params.waveform = m_settings.m_op2_waveform;
  global_synth_params.osc3_params.waveform = m_settings.m_op3_waveform;
  global_synth_params.osc4_params.waveform = m_settings.m_op4_waveform;

  global_synth_params.osc1_params.cents = m_settings.m_op1_detune_cents;
  global_synth_params.osc2_params.cents = m_settings.m_op2_detune_cents;
  global_synth_params.osc3_params.cents = m_settings.m_op3_detune_cents;
  global_synth_params.osc4_params.cents = m_settings.m_op4_detune_cents;

  // EG1
  global_synth_params.eg1_params.attack_time_msec = m_settings.m_eg1_attack_ms;
  global_synth_params.eg1_params.decay_time_msec = m_settings.m_eg1_decay_ms;
  global_synth_params.eg1_params.sustain_level = m_settings.m_eg1_sustain_lvl;
  global_synth_params.eg1_params.release_time_msec =
      m_settings.m_eg1_release_ms;
  global_synth_params.eg1_params.reset_to_zero =
      (bool)m_settings.m_reset_to_zero;
  global_synth_params.eg1_params.legato_mode = (bool)m_settings.m_legato_mode;
  global_synth_params.eg1_params.sustain_override =
      (bool)m_settings.m_op1_sustain_override;

  // EG2
  global_synth_params.eg2_params.attack_time_msec = m_settings.m_eg2_attack_ms;
  global_synth_params.eg2_params.decay_time_msec = m_settings.m_eg2_decay_ms;
  global_synth_params.eg2_params.sustain_level = m_settings.m_eg2_sustain_lvl;
  global_synth_params.eg2_params.release_time_msec =
      m_settings.m_eg2_release_ms;
  global_synth_params.eg2_params.reset_to_zero =
      (bool)m_settings.m_reset_to_zero;
  global_synth_params.eg2_params.legato_mode = (bool)m_settings.m_legato_mode;
  global_synth_params.eg2_params.sustain_override =
      (bool)m_settings.m_op2_sustain_override;

  // EG3
  global_synth_params.eg3_params.attack_time_msec = m_settings.m_eg3_attack_ms;
  global_synth_params.eg3_params.decay_time_msec = m_settings.m_eg3_decay_ms;
  global_synth_params.eg3_params.sustain_level = m_settings.m_eg3_sustain_lvl;
  global_synth_params.eg3_params.release_time_msec =
      m_settings.m_eg3_release_ms;
  global_synth_params.eg3_params.reset_to_zero =
      (bool)m_settings.m_reset_to_zero;
  global_synth_params.eg3_params.legato_mode = (bool)m_settings.m_legato_mode;
  global_synth_params.eg3_params.sustain_override =
      (bool)m_settings.m_op3_sustain_override;

  // EG4
  global_synth_params.eg4_params.attack_time_msec = m_settings.m_eg4_attack_ms;
  global_synth_params.eg4_params.decay_time_msec = m_settings.m_eg4_decay_ms;
  global_synth_params.eg4_params.sustain_level = m_settings.m_eg4_sustain_lvl;
  global_synth_params.eg4_params.release_time_msec =
      m_settings.m_eg4_release_ms;
  global_synth_params.eg4_params.reset_to_zero =
      (bool)m_settings.m_reset_to_zero;
  global_synth_params.eg4_params.legato_mode = (bool)m_settings.m_legato_mode;
  global_synth_params.eg4_params.sustain_override =
      (bool)m_settings.m_op4_sustain_override;

  // LFO1
  global_synth_params.lfo1_params.waveform = m_settings.m_lfo1_waveform;
  global_synth_params.lfo1_params.osc_fo = m_settings.m_lfo1_rate;

  // DCA
  global_synth_params.dca_params.amplitude_db = m_settings.m_volume_db;

  // LFO1 DEST_1_
  if (m_settings.m_lfo1_mod_dest1 == DX_LFO_DEST_NONE) {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC1_OUTPUT_AMP, false);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC1_FO, false);
  } else if (m_settings.m_lfo1_mod_dest1 == DX_LFO_DEST_AMP_MOD) {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC1_OUTPUT_AMP, true);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC1_FO, false);
  } else  // vibrato
  {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC1_OUTPUT_AMP, true);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC1_FO, false);
  }

  // LFO1 DEST2
  if (m_settings.m_lfo1_mod_dest2 == DX_LFO_DEST_NONE) {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC2_OUTPUT_AMP, false);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC2_FO, false);
  } else if (m_settings.m_lfo1_mod_dest2 == DX_LFO_DEST_AMP_MOD) {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC2_OUTPUT_AMP, true);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC2_FO, false);
  } else  // vibrato
  {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC2_OUTPUT_AMP, true);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC2_FO, false);
  }

  // LFO1 DEST3
  if (m_settings.m_lfo1_mod_dest3 == DX_LFO_DEST_NONE) {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC3_OUTPUT_AMP, false);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC3_FO, false);
  } else if (m_settings.m_lfo1_mod_dest3 == DX_LFO_DEST_AMP_MOD) {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC3_OUTPUT_AMP, true);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC3_FO, false);
  } else  // vibrato
  {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC3_OUTPUT_AMP, true);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC3_FO, false);
  }

  // LFO1 DEST4
  if (m_settings.m_lfo1_mod_dest4 == DX_LFO_DEST_NONE) {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC4_OUTPUT_AMP, false);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC4_FO, false);
  } else if (m_settings.m_lfo1_mod_dest4 == DX_LFO_DEST_AMP_MOD) {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC4_OUTPUT_AMP, true);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC4_FO, false);
  } else  // vibrato
  {
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC4_OUTPUT_AMP, true);
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_OSC4_FO, false);
  }
}

void DXSynth::ResetVoices() {
  for (auto v : voices_) v->Reset();
}

void DXSynth::IncrementVoiceTimestamps() {
  for (auto v : voices_) {
    if (v->m_note_on) v->m_timestamp++;
  }
}

std::shared_ptr<DXSynthVoice> DXSynth::GetOldestVoice() {
  int timestamp = -1;
  std::shared_ptr<DXSynthVoice> found_voice = NULL;
  for (auto v : voices_) {
    if (v->m_note_on && (int)v->m_timestamp > timestamp) {
      found_voice = v;
      timestamp = (int)v->m_timestamp;
    }
  }
  return found_voice;
}

std::shared_ptr<DXSynthVoice> DXSynth::GetOldestVoiceWithNote(int midi_note) {
  int timestamp = -1;
  std::shared_ptr<DXSynthVoice> found_voice = NULL;
  for (auto v : voices_) {
    if (v->CanNoteOff() && (int)v->m_timestamp > timestamp &&
        v->m_midi_note_number == midi_note) {
      found_voice = v;
      timestamp = (int)v->m_timestamp;
    }
  }
  return found_voice;
}

void DXSynth::randomize() {
  // dxsynth_reset(dx);
  // return;
  // printf("Randomizing DXSYNTH!\n");

  strncpy(m_settings.m_settings_name, "-- random UNSAVED--", 256);

  m_settings.m_voice_mode = rand() % 8;
  m_settings.m_portamento_time_ms = rand() % 5000;
  m_settings.m_pitchbend_range = (rand() % 12) + 1;
  // m_settings.m_velocity_to_attack_scaling = rand() % 2;
  m_settings.m_note_number_to_decay_scaling = rand() % 2;
  m_settings.m_reset_to_zero = rand() % 2;
  m_settings.m_legato_mode = rand() % 2;

  m_settings.m_lfo1_intensity = ((float)rand()) / RAND_MAX;
  m_settings.m_lfo1_rate = 0.02 + ((float)rand()) / (RAND_MAX / 20);
  m_settings.m_lfo1_waveform = rand() % MAX_LFO_OSC;
  m_settings.m_lfo1_mod_dest1 = rand() % 3;
  m_settings.m_lfo1_mod_dest2 = rand() % 3;
  m_settings.m_lfo1_mod_dest3 = rand() % 3;
  m_settings.m_lfo1_mod_dest4 = rand() % 3;

  m_settings.m_op1_waveform = rand() % MAX_OSC;
  m_settings.m_op1_ratio = GetRandRatio();
  m_settings.m_op1_detune_cents = (rand() % 20) - 10;
  m_settings.m_eg1_attack_ms = rand() % 300;
  m_settings.m_eg1_decay_ms = rand() % 300;
  m_settings.m_eg1_sustain_lvl = ((float)rand()) / RAND_MAX;
  m_settings.m_eg1_release_ms = rand() % 300;
  // m_settings.m_op1_output_lvl = (rand() % 55) + 35;

  m_settings.m_op2_waveform = rand() % MAX_OSC;
  m_settings.m_op2_ratio = GetRandRatio();
  m_settings.m_op2_detune_cents = (rand() % 20) - 10;
  m_settings.m_eg2_attack_ms = rand() % 300;
  m_settings.m_eg2_decay_ms = rand() % 400;
  m_settings.m_eg2_sustain_lvl = ((float)rand()) / RAND_MAX;
  m_settings.m_eg2_release_ms = rand() % 400;
  m_settings.m_op2_output_lvl = (rand() % 55) + 15;

  m_settings.m_op3_waveform = rand() % MAX_OSC;
  m_settings.m_op3_ratio = GetRandRatio();
  m_settings.m_op3_detune_cents = (rand() % 20) - 10;
  m_settings.m_eg3_attack_ms = rand() % 300;
  m_settings.m_eg3_decay_ms = rand() % 400;
  m_settings.m_eg3_sustain_lvl = ((float)rand()) / RAND_MAX;
  m_settings.m_eg3_release_ms = rand() % 400;
  m_settings.m_op3_output_lvl = (rand() % 55) + 15;

  m_settings.m_op4_waveform = rand() % MAX_OSC;
  m_settings.m_op4_ratio = GetRandRatio();
  m_settings.m_op4_detune_cents = (rand() % 20) - 10;
  m_settings.m_eg4_attack_ms = rand() % 400;
  m_settings.m_eg4_decay_ms = rand() % 500;
  m_settings.m_eg4_sustain_lvl = ((float)rand()) / RAND_MAX;
  m_settings.m_eg4_release_ms = rand() % 500;
  m_settings.m_op4_output_lvl = (rand() % 55) + 15;
  m_settings.m_op4_feedback = rand() % 70;

  Update();
  // dxsynth_print_settings(dx);
}

void DXSynth::Save(std::string preset) {
  if (preset.empty()) {
    printf(
        "Play tha game, pal, need a name to save yer synth settings "
        "with\n");
    return;
  }
  const char *preset_name = preset.c_str();

  printf("Saving '%s' settings for dxsynth to file %s\n", preset_name,
         DX_PRESET_FILENAME);
  FILE *presetzzz = fopen(DX_PRESET_FILENAME, "a+");
  if (presetzzz == NULL) {
    printf("Couldn't save settings!!\n");
    return;
  }

  int settings_count = 0;
  strncpy(m_settings.m_settings_name, preset_name, 256);

  fprintf(presetzzz, "::name=%s", m_settings.m_settings_name);
  settings_count++;

  fprintf(presetzzz, "::m_lfo1_intensity=%f", m_settings.m_lfo1_intensity);
  settings_count++;
  fprintf(presetzzz, "::m_lfo1_rate=%f", m_settings.m_lfo1_rate);
  settings_count++;
  fprintf(presetzzz, "::m_lfo1_waveform=%d", m_settings.m_lfo1_waveform);
  settings_count++;
  fprintf(presetzzz, "::m_lfo1_mod_dest1=%d", m_settings.m_lfo1_mod_dest1);
  settings_count++;
  fprintf(presetzzz, "::m_lfo1_mod_dest2=%d", m_settings.m_lfo1_mod_dest2);
  settings_count++;
  fprintf(presetzzz, "::m_lfo1_mod_dest3=%d", m_settings.m_lfo1_mod_dest3);
  settings_count++;
  fprintf(presetzzz, "::m_lfo1_mod_dest4=%d", m_settings.m_lfo1_mod_dest4);
  settings_count++;

  fprintf(presetzzz, "::m_op1_waveform=%d", m_settings.m_op1_waveform);
  settings_count++;
  fprintf(presetzzz, "::m_op1_ratio=%f", m_settings.m_op1_ratio);
  settings_count++;
  fprintf(presetzzz, "::m_op1_detune_cents=%f", m_settings.m_op1_detune_cents);
  settings_count++;
  fprintf(presetzzz, "::m_eg1_attack_ms=%f", m_settings.m_eg1_attack_ms);
  settings_count++;
  fprintf(presetzzz, "::m_eg1_decay_ms=%f", m_settings.m_eg1_decay_ms);
  settings_count++;
  fprintf(presetzzz, "::m_eg1_sustain_lvl=%f", m_settings.m_eg1_sustain_lvl);
  settings_count++;
  fprintf(presetzzz, "::m_eg1_release_ms=%f", m_settings.m_eg1_release_ms);
  settings_count++;
  fprintf(presetzzz, "::m_op1_output_lvl=%f", m_settings.m_op1_output_lvl);
  settings_count++;

  fprintf(presetzzz, "::m_op2_waveform=%d", m_settings.m_op2_waveform);
  settings_count++;
  fprintf(presetzzz, "::m_op2_ratio=%f", m_settings.m_op2_ratio);
  settings_count++;
  fprintf(presetzzz, "::m_op2_detune_cents=%f", m_settings.m_op2_detune_cents);
  settings_count++;
  fprintf(presetzzz, "::m_eg2_attack_ms=%f", m_settings.m_eg2_attack_ms);
  settings_count++;
  fprintf(presetzzz, "::m_eg2_decay_ms=%f", m_settings.m_eg2_decay_ms);
  settings_count++;
  fprintf(presetzzz, "::m_eg2_sustain_lvl=%f", m_settings.m_eg2_sustain_lvl);
  settings_count++;
  fprintf(presetzzz, "::m_eg2_release_ms=%f", m_settings.m_eg2_release_ms);
  settings_count++;
  fprintf(presetzzz, "::m_op2_output_lvl=%f", m_settings.m_op2_output_lvl);
  settings_count++;

  fprintf(presetzzz, "::m_op3_waveform=%d", m_settings.m_op3_waveform);
  settings_count++;
  fprintf(presetzzz, "::m_op3_ratio=%f", m_settings.m_op3_ratio);
  settings_count++;
  fprintf(presetzzz, "::m_op3_detune_cents=%f", m_settings.m_op3_detune_cents);
  settings_count++;
  fprintf(presetzzz, "::m_eg3_attack_ms=%f", m_settings.m_eg3_attack_ms);
  settings_count++;
  fprintf(presetzzz, "::m_eg3_decay_ms=%f", m_settings.m_eg3_decay_ms);
  settings_count++;
  fprintf(presetzzz, "::m_eg3_sustain_lvl=%f", m_settings.m_eg3_sustain_lvl);
  settings_count++;
  fprintf(presetzzz, "::m_eg3_release_ms=%f", m_settings.m_eg3_release_ms);
  settings_count++;
  fprintf(presetzzz, "::m_op3_output_lvl=%f", m_settings.m_op3_output_lvl);
  settings_count++;

  fprintf(presetzzz, "::m_op4_waveform=%d", m_settings.m_op4_waveform);
  settings_count++;
  fprintf(presetzzz, "::m_op4_ratio=%f", m_settings.m_op4_ratio);
  settings_count++;
  fprintf(presetzzz, "::m_op4_detune_cents=%f", m_settings.m_op4_detune_cents);
  settings_count++;
  fprintf(presetzzz, "::m_eg4_attack_ms=%f", m_settings.m_eg4_attack_ms);
  settings_count++;
  fprintf(presetzzz, "::m_eg4_decay_ms=%f", m_settings.m_eg4_decay_ms);
  settings_count++;
  fprintf(presetzzz, "::m_eg4_sustain_lvl=%f", m_settings.m_eg4_sustain_lvl);
  settings_count++;
  fprintf(presetzzz, "::m_eg4_release_ms=%f", m_settings.m_eg4_release_ms);
  settings_count++;
  fprintf(presetzzz, "::m_op4_output_lvl=%f", m_settings.m_op4_output_lvl);
  settings_count++;
  fprintf(presetzzz, "::m_op4_feedback=%f", m_settings.m_op4_feedback);
  settings_count++;

  fprintf(presetzzz, "::m_portamento_time_ms=%f",
          m_settings.m_portamento_time_ms);
  settings_count++;
  fprintf(presetzzz, "::m_volume_db=%f", m_settings.m_volume_db);
  settings_count++;
  fprintf(presetzzz, "::m_pitchbend_range=%d", m_settings.m_pitchbend_range);
  settings_count++;
  fprintf(presetzzz, "::m_voice_mode=%d", m_settings.m_voice_mode);
  settings_count++;
  fprintf(presetzzz, "::m_velocity_to_attack_scaling=%d",
          m_settings.m_velocity_to_attack_scaling);
  settings_count++;
  fprintf(presetzzz, "::m_note_number_to_decay_scaling=%d",
          m_settings.m_note_number_to_decay_scaling);
  settings_count++;
  fprintf(presetzzz, "::m_reset_to_zero=%d", m_settings.m_reset_to_zero);
  settings_count++;
  fprintf(presetzzz, "::m_legato_mode=%d", m_settings.m_legato_mode);
  settings_count++;

  fprintf(presetzzz, ":::\n");
  fclose(presetzzz);
  printf("Wrote %d settings\n", settings_count++);
}

void DXSynth::Load(std::string preset_name) {
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

  FILE *presetzzz = fopen(DX_PRESET_FILENAME, "r+");
  if (presetzzz == NULL) return;

  char *tok, *last_tok;
  char const *sep = "::";

  while (fgets(line, sizeof(line), presetzzz)) {
    int settings_count = 0;

    for (tok = strtok_r(line, sep, &last_tok); tok;
         tok = strtok_r(NULL, sep, &last_tok)) {
      sscanf(tok, "%[^=]=%s", setting_key, setting_val);
      sscanf(setting_val, "%lf", &scratch_val);
      // printf("key:%s val:%f\n", setting_key, scratch_val);
      if (strcmp(setting_key, "name") == 0) {
        if (strcmp(setting_val, preset_to_load) != 0) break;
        strcpy(m_settings.m_settings_name, setting_val);
        settings_count++;
      } else if (strcmp(setting_key, "m_lfo1_intensity") == 0) {
        m_settings.m_lfo1_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_lfo1_rate") == 0) {
        m_settings.m_lfo1_rate = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_lfo1_waveform") == 0) {
        m_settings.m_lfo1_waveform = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_lfo1_mod_dest1") == 0) {
        m_settings.m_lfo1_mod_dest1 = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_lfo1_mod_dest2") == 0) {
        m_settings.m_lfo1_mod_dest2 = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_lfo1_mod_dest3") == 0) {
        m_settings.m_lfo1_mod_dest3 = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_lfo1_mod_dest4") == 0) {
        m_settings.m_lfo1_mod_dest4 = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op1_waveform") == 0) {
        m_settings.m_op1_waveform = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op1_ratio") == 0) {
        m_settings.m_op1_ratio = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op1_detune_cents") == 0) {
        m_settings.m_op1_detune_cents = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg1_attack_ms") == 0) {
        m_settings.m_eg1_attack_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg1_decay_ms") == 0) {
        m_settings.m_eg1_decay_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg1_sustain_lvl") == 0) {
        m_settings.m_eg1_sustain_lvl = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg1_release_ms") == 0) {
        m_settings.m_eg1_release_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op1_output_lvl") == 0) {
        m_settings.m_op1_output_lvl = scratch_val;
        settings_count++;
      }

      else if (strcmp(setting_key, "m_op2_waveform") == 0) {
        m_settings.m_op2_waveform = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op2_ratio") == 0) {
        m_settings.m_op2_ratio = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op2_detune_cents") == 0) {
        m_settings.m_op2_detune_cents = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg2_attack_ms") == 0) {
        m_settings.m_eg2_attack_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg2_decay_ms") == 0) {
        m_settings.m_eg2_decay_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg2_sustain_lvl") == 0) {
        m_settings.m_eg2_sustain_lvl = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg2_release_ms") == 0) {
        m_settings.m_eg2_release_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op2_output_lvl") == 0) {
        m_settings.m_op2_output_lvl = scratch_val;
        settings_count++;
      }

      else if (strcmp(setting_key, "m_op3_waveform") == 0) {
        m_settings.m_op3_waveform = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op3_ratio") == 0) {
        m_settings.m_op3_ratio = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op3_detune_cents") == 0) {
        m_settings.m_op3_detune_cents = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg3_attack_ms") == 0) {
        m_settings.m_eg3_attack_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg3_decay_ms") == 0) {
        m_settings.m_eg3_decay_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg3_sustain_lvl") == 0) {
        m_settings.m_eg3_sustain_lvl = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg3_release_ms") == 0) {
        m_settings.m_eg3_release_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op3_output_lvl") == 0) {
        m_settings.m_op3_output_lvl = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op4_waveform") == 0) {
        m_settings.m_op4_waveform = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op4_ratio") == 0) {
        m_settings.m_op4_ratio = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op4_detune_cents") == 0) {
        m_settings.m_op4_detune_cents = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg4_attack_ms") == 0) {
        m_settings.m_eg4_attack_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg4_decay_ms") == 0) {
        m_settings.m_eg4_decay_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg4_sustain_lvl") == 0) {
        m_settings.m_eg4_sustain_lvl = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_eg4_release_ms") == 0) {
        m_settings.m_eg4_release_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op4_output_lvl") == 0) {
        m_settings.m_op4_output_lvl = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_op4_feedback") == 0) {
        m_settings.m_op4_feedback = scratch_val;
        settings_count++;
      }

      else if (strcmp(setting_key, "m_portamento_time_ms") == 0) {
        m_settings.m_portamento_time_ms = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_volume_db") == 0) {
        m_settings.m_volume_db = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_pitchbend_range") == 0) {
        m_settings.m_pitchbend_range = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_voice_mode") == 0) {
        m_settings.m_voice_mode = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_velocity_to_attack_scaling") == 0) {
        m_settings.m_velocity_to_attack_scaling = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_note_number_to_decay_scaling") == 0) {
        m_settings.m_note_number_to_decay_scaling = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_reset_to_zero") == 0) {
        m_settings.m_reset_to_zero = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "m_legato_mode") == 0) {
        m_settings.m_legato_mode = scratch_val;
        settings_count++;
      }
    }
    // if (settings_count > 0)
    //    printf("Loaded %d settings\n", settings_count);
    Update();
  }

  fclose(presetzzz);
}

void DXSynth::SetLFO1Intensity(double val) {
  if (val >= 0.0 && val <= 1.0)
    m_settings.m_lfo1_intensity = val;
  else
    printf("Val has to be between 0.0-1.0\n");
}

void DXSynth::SetLFO1Rate(double val) {
  if (val >= 0.02 && val <= 20.0)
    m_settings.m_lfo1_rate = val;
  else
    printf("Val has to be between 0.02 - 20.0\n");
}

void DXSynth::SetLFO1Waveform(unsigned int val) {
  if (val < MAX_LFO_OSC)
    m_settings.m_lfo1_waveform = val;
  else
    printf("Val has to be between [0-%d]\n", MAX_LFO_OSC);
}

void DXSynth::SetLFO1ModDest(unsigned int mod_dest, unsigned int dest) {
  if (dest > 2) {
    printf("Dest has to be [0-2]\n");
    return;
  }
  switch (mod_dest) {
    case (1):
      m_settings.m_lfo1_mod_dest1 = dest;
      break;
    case (2):
      m_settings.m_lfo1_mod_dest2 = dest;
      break;
    case (3):
      m_settings.m_lfo1_mod_dest3 = dest;
      break;
    case (4):
      m_settings.m_lfo1_mod_dest4 = dest;
      break;
    default:
      printf("Huh?! Only got 4 destinations, brah..\n");
  }
}

void DXSynth::SetOpWaveform(unsigned int op, unsigned int val) {
  if (val >= MAX_OSC) {
    printf("WAV has to be [0-%d)\n", MAX_OSC);
    return;
  }
  switch (op) {
    case (1):
      m_settings.m_op1_waveform = val;
      break;
    case (2):
      m_settings.m_op2_waveform = val;
      break;
    case (3):
      m_settings.m_op3_waveform = val;
      break;
    case (4):
      m_settings.m_op4_waveform = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetOpRatio(unsigned int op, double val) {
  if (val < 0.01 || val > 26) {
    printf("val has to be [0.01-26]\n");
    return;
  }
  switch (op) {
    case (1):
      m_settings.m_op1_ratio = val;
      break;
    case (2):
      m_settings.m_op2_ratio = val;
      break;
    case (3):
      m_settings.m_op3_ratio = val;
      break;
    case (4):
      m_settings.m_op4_ratio = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetOpSustain(unsigned int op, bool val) {
  switch (op) {
    case (1):
      m_settings.m_op1_sustain_override = val;
      break;
    case (2):
      m_settings.m_op2_sustain_override = val;
      break;
    case (3):
      m_settings.m_op3_sustain_override = val;
      break;
    case (4):
      m_settings.m_op4_sustain_override = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetOpDetune(unsigned int op, double val) {
  if (val < -100 || val > 100) {
    printf("val has to be [-100-100]\n");
    return;
  }
  switch (op) {
    case (1):
      m_settings.m_op1_detune_cents = val;
      break;
    case (2):
      m_settings.m_op2_detune_cents = val;
      break;
    case (3):
      m_settings.m_op3_detune_cents = val;
      break;
    case (4):
      m_settings.m_op4_detune_cents = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetEGAttackMs(unsigned int eg, double val) {
  if (val < EG_MINTIME_MS || val > EG_MAXTIME_MS) {
    printf("val has to be [%d - %d] - you've given me %f\n", EG_MINTIME_MS,
           EG_MAXTIME_MS, val);
    return;
  }
  switch (eg) {
    case (1):
      m_settings.m_eg1_attack_ms = val;
      break;
    case (2):
      m_settings.m_eg2_attack_ms = val;
      break;
    case (3):
      m_settings.m_eg3_attack_ms = val;
      break;
    case (4):
      m_settings.m_eg4_attack_ms = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetEGDecayMs(unsigned int eg, double val) {
  if (val < EG_MINTIME_MS || val > EG_MAXTIME_MS) {
    printf("val has to be [%d - %d]\n", EG_MINTIME_MS, EG_MAXTIME_MS);
    return;
  }
  switch (eg) {
    case (1):
      m_settings.m_eg1_decay_ms = val;
      break;
    case (2):
      m_settings.m_eg2_decay_ms = val;
      break;
    case (3):
      m_settings.m_eg3_decay_ms = val;
      break;
    case (4):
      m_settings.m_eg4_decay_ms = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetEGReleaseMs(unsigned int eg, double val) {
  if (val < EG_MINTIME_MS || val > EG_MAXTIME_MS) {
    printf("val has to be [%d - %d]\n", EG_MINTIME_MS, EG_MAXTIME_MS);
    return;
  }
  switch (eg) {
    case (1):
      m_settings.m_eg1_release_ms = val;
      break;
    case (2):
      m_settings.m_eg2_release_ms = val;
      break;
    case (3):
      m_settings.m_eg3_release_ms = val;
      break;
    case (4):
      m_settings.m_eg4_release_ms = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetEGSustainLevel(unsigned int eg, double val) {
  if (val < 0 || val > 1) {
    printf("val has to be [0-1]\n");
    return;
  }
  switch (eg) {
    case (1):
      m_settings.m_eg1_sustain_lvl = val;
      break;
    case (2):
      m_settings.m_eg2_sustain_lvl = val;
      break;
    case (3):
      m_settings.m_eg3_sustain_lvl = val;
      break;
    case (4):
      m_settings.m_eg4_sustain_lvl = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetOpOutputLevel(unsigned int op, double val) {
  if (val < 0 || val > 99) {
    printf("val has to be [0-99]\n");
    return;
  }
  switch (op) {
    case (1):
      m_settings.m_op1_output_lvl = val;
      break;
    case (2):
      m_settings.m_op2_output_lvl = val;
      break;
    case (3):
      m_settings.m_op3_output_lvl = val;
      break;
    case (4):
      m_settings.m_op4_output_lvl = val;
      break;
    default:
      printf("Huh?! Only got 4 operators, brah..\n");
  }
}

void DXSynth::SetPortamentoTimeMs(double val) {
  if (val >= 0 && val <= 5000.0)
    m_settings.m_portamento_time_ms = val;
  else
    printf("Val has to be between 0 - 5000.0\n");
}

void DXSynth::SetVolumeDb(double val) {
  if (val >= -96 && val <= 20)
    m_settings.m_volume_db = val;
  else
    printf("Val has to be between -96 and 20\n");
}

void DXSynth::SetPitchbendRange(unsigned int val) {
  if (val <= 12)
    m_settings.m_pitchbend_range = val;
  else
    printf("Val has to be between 0 and 12\n");
}
void DXSynth::SetVoiceMode(unsigned int val) {
  if (val < MAXDX)
    m_settings.m_voice_mode = val;
  else
    printf("Val has to be [0-%d)\n", MAXDX);
}

void DXSynth::SetVelocityToAttackScaling(bool b) {
  m_settings.m_velocity_to_attack_scaling = b;
}
void DXSynth::SetNoteNumberToDecayScaling(bool b) {
  m_settings.m_note_number_to_decay_scaling = b;
}

void DXSynth::SetResetToZero(bool b) { m_settings.m_reset_to_zero = b; }

void DXSynth::SetLegatoMode(bool b) { m_settings.m_legato_mode = b; }

void DXSynth::SetOp4Feedback(double val) {
  if (val >= 0 && val <= 70)
    m_settings.m_op4_feedback = val;
  else
    printf("Op4 feedback val has to be [0-70]\n");
}

void DXSynth::SetActiveMidiOsc(int osc_num) {
  if (osc_num >= 1 && osc_num <= 4) active_midi_osc = osc_num;
}
void DXSynth::SetParam(std::string name, double val) {
  if (name == "vol")
    SetVolume(val);
  else if (name == "pan")
    SetPan(val);
  else if (name == "algo")
    SetVoiceMode(val);
  else if (name == "midi_osc")
    SetActiveMidiOsc(val);
  else if (name == "porta")
    SetPortamentoTimeMs(val);
  else if (name == "pitchrange")
    SetPitchbendRange(val);
  else if (name == "op4fb")
    SetOp4Feedback(val);
  else if (name == "vel2att")
    SetVelocityToAttackScaling(val);
  else if (name == "note2dec")
    SetNoteNumberToDecayScaling(val);
  else if (name == "reset2zero")
    SetResetToZero(val);
  else if (name == "legato")
    SetLegatoMode(val);
  else if (name == "l1_wav")
    SetLFO1Waveform(val);
  else if (name == "l1_int")
    SetLFO1Intensity(val);
  else if (name == "l1_rate")
    SetLFO1Rate(val);

  else if (name == "l1_dest1")
    SetLFO1ModDest(1, val);
  else if (name == "l1_dest2")
    SetLFO1ModDest(2, val);
  else if (name == "l1_dest3")
    SetLFO1ModDest(3, val);
  else if (name == "l1_dest4")
    SetLFO1ModDest(4, val);

  ///// OP11111111111111111
  else if (name == "o1wav")
    SetOpWaveform(1, val);
  else if (name == "o1rat")
    SetOpRatio(1, val);
  else if (name == "o1det")
    SetOpDetune(1, val);
  else if (name == "o1sus")
    SetOpSustain(1, val);
  else if (name == "op1freq")
    SetOpFreq(1, val);

  else if (name == "e1att")
    SetEGAttackMs(1, val);
  else if (name == "e1dec")
    SetEGDecayMs(1, val);
  else if (name == "e1sus")
    SetEGSustainLevel(1, val);
  else if (name == "e1rel")
    SetEGReleaseMs(1, val);

  ///// OP2222222222222222222222
  else if (name == "o2wav")
    SetOpWaveform(2, val);
  else if (name == "o2rat")
    SetOpRatio(2, val);
  else if (name == "o2det")
    SetOpDetune(2, val);
  else if (name == "o2sus")
    SetOpSustain(2, val);
  else if (name == "op2freq")
    SetOpFreq(2, val);

  else if (name == "e2att")
    SetEGAttackMs(2, val);
  else if (name == "e2dec")
    SetEGDecayMs(2, val);
  else if (name == "e2sus")
    SetEGSustainLevel(2, val);
  else if (name == "e2rel")
    SetEGReleaseMs(2, val);

  ///// 33333333333333333
  else if (name == "o3wav")
    SetOpWaveform(3, val);
  else if (name == "o3rat")
    SetOpRatio(3, val);
  else if (name == "o3det")
    SetOpDetune(3, val);
  else if (name == "o3sus")
    SetOpSustain(3, val);
  else if (name == "op3freq")
    SetOpFreq(3, val);

  else if (name == "e3att")
    SetEGAttackMs(3, val);
  else if (name == "e3dec")
    SetEGDecayMs(3, val);
  else if (name == "e3sus")
    SetEGSustainLevel(3, val);
  else if (name == "e3rel")
    SetEGReleaseMs(3, val);

  ///// 44444444444444444
  else if (name == "o4wav")
    SetOpWaveform(4, val);
  else if (name == "o4rat")
    SetOpRatio(4, val);
  else if (name == "o4det")
    SetOpDetune(4, val);
  else if (name == "o4sus")
    SetOpSustain(4, val);
  else if (name == "op4freq")
    SetOpFreq(4, val);

  else if (name == "e4att")
    SetEGAttackMs(4, val);
  else if (name == "e4dec")
    SetEGDecayMs(4, val);
  else if (name == "e4sus")
    SetEGSustainLevel(4, val);
  else if (name == "e4rel")
    SetEGReleaseMs(4, val);

  /// OUTZZZ
  else if (name == "op1out")
    SetOpOutputLevel(1, val);
  else if (name == "op2out")
    SetOpOutputLevel(2, val);
  else if (name == "op3out")
    SetOpOutputLevel(3, val);
  else if (name == "op4out")
    SetOpOutputLevel(4, val);

  Update();
}

void DXSynth::ListPresets() {
  // TODO - implement this properly!
  synth_list_presets(DXSYNTH_TYPE);
}

};  // namespace SBAudio
