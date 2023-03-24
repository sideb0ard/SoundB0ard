#pragma once

#include <math.h>
#include <stdbool.h>

#include <map>

#include "defjams.h"

struct GlobalOscillatorParams {
  // --- common
  double osc_fo;
  double fo_ratio;
  double amplitude;            // 0->1 from GUI
  double pulse_width_control;  // from GUI
  int octave;                  // octave tweak
  int semitones;               // semitones tweak
  int cents;                   // cents tweak
  unsigned int waveform;       // to store type
  // --- LFOs
  unsigned int lfo_mode;  // to store MODE
  unsigned int loop_mode;
};

struct GlobalFilterParams {
  double fc_control;
  double q_control;
  double aux_control;
  double saturation;
  unsigned int filter_type;
  unsigned int nlp;
};

struct GlobalEgParams {
  double attack_time_msec;   // att: is a time duration
  double decay_time_msec;    // dcy: is a time to decay 1->0
  double release_time_msec;  // rel: is a time to decay 1->0
  double sustain_level;
  double shutdown_time_msec;  // shutdown is a time
  bool reset_to_zero;
  bool legato_mode;
  bool sustain_override;
};

struct GlobalDCAParams {
  double amplitude_db;  // the user's control setting in dB
  double pan_control;
};

struct GlobalVoiceParams {
  unsigned int voice_mode;
  bool hard_sync;
  double hs_ratio;  // hard sync
  double portamento_time_msec;

  double osc_fo_pitchbend_mod_range;
  double osc_fo_mod_range;
  double osc_hard_sync_mod_range;
  double filter_mod_range;
  double filter_q_mod_range;
  double amp_mod_range;

  double filter_keytrack_intensity;

  double lfo1_osc_mod_intensity;
  double lfo1_hs_mod_intensity;
  double lfo1_filter1_mod_intensity;
  double lfo1_filter1_q_mod_intensity;
  double lfo1_filter2_mod_intensity;
  double lfo1_filter2_q_mod_intensity;
  double lfo1_dca_amp_mod_intensity;
  double lfo1_dca_pan_mod_intensity;

  double lfo2_osc_mod_intensity;
  double lfo2_hs_mod_intensity;
  double lfo2_filter1_mod_intensity;
  double lfo2_filter1_q_mod_intensity;
  double lfo2_filter2_mod_intensity;
  double lfo2_filter2_q_mod_intensity;
  double lfo2_dca_amp_mod_intensity;
  double lfo2_dca_pan_mod_intensity;

  double eg1_osc_mod_intensity;
  double eg1_filter1_mod_intensity;
  double eg1_filter2_mod_intensity;
  double eg1_dca_amp_mod_intensity;

  double eg2_osc_mod_intensity;
  double eg2_filter1_mod_intensity;
  double eg2_filter2_mod_intensity;
  double eg2_dca_amp_mod_intensity;

  double eg3_osc_mod_intensity;
  double eg3_filter1_mod_intensity;
  double eg3_filter2_mod_intensity;
  double eg3_dca_amp_mod_intensity;

  double eg4_osc_mod_intensity;
  double eg4_filter1_mod_intensity;
  double eg4_filter2_mod_intensity;
  double eg4_dca_amp_mod_intensity;

  // vector synth stuff
  double orbit_x_amp;
  double orbit_y_amp;
  double amplitude_a;
  double amplitude_b;
  double amplitude_c;
  double amplitude_d;
  unsigned int vector_path_mode;

  // DX synth
  double op1_feedback;
  double op2_feedback;
  double op3_feedback;
  double op4_feedback;
};

struct GlobalSynthParams {
  GlobalVoiceParams voice_params;
  GlobalOscillatorParams osc1_params;
  GlobalOscillatorParams osc2_params;
  GlobalOscillatorParams osc3_params;
  GlobalOscillatorParams osc4_params;
  GlobalOscillatorParams lfo1_params;
  GlobalOscillatorParams lfo2_params;
  GlobalFilterParams filter1_params;
  GlobalFilterParams filter2_params;
  GlobalEgParams eg1_params;
  GlobalEgParams eg2_params;
  GlobalEgParams eg3_params;
  GlobalEgParams eg4_params;
  GlobalDCAParams dca_params;
};

double midi_to_pan_value(unsigned int midi_val);
double mma_midi_to_atten_dB(unsigned int midi_val);
double midi_to_bipolar(unsigned int midi_val);
double calculate_dx_amp(double dx_level);

std::map<std::string, double> GetPreset(int id, std::string preset_name);
