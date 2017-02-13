#ifndef SBSHELL_SYNTH_FUNCTION_H_
#define SBSHELL_SYNTH_FUNCTION_H_

// OSCILLATOR ///////////////////////////
typedef struct {
    // --- common
    double osc_fo;
    double fo_ratio;
    double amplitude;           // 0->1 from GUI
    double pulse_width_control; // from GUI
    int octave;                 // octave tweak
    int semitones;              // semitones tweak
    int cents;                  // cents tweak
    unsigned int waveform;      // to store type
    // --- LFOs
    unsigned int lfo_mode; // to store MODE
    // --- CSampleOscillators
    unsigned int loop_Mode;
} global_oscillator_params;

// FILTER ///////////////////////////////////
typedef struct {
    double fc_control;
    double q_control;
    double aux_control;
    double saturation;
    unsigned int filter_type;
    unsigned int nlp;
} global_filter_params;

// ENVELOPE GENERATOR //////////////////////
typedef struct {
    double attack_time_msec;  // att: is a time duration
    double decay_time_msec;   // dcy: is a time to decay 1->0
    double release_time_msec; // rel: is a time to decay 1->0
    double sustain_level;
    double shutdown_time_msec; // shutdown is a time
    bool reset_to_zero;
    bool legato_mode;
} global_eg_params;

// DCA GENERATOR //////////////////////
typedef struct {
    double amplitude_db; // the user's control setting in dB
    double pan_control;
} global_dca_params;

// VOICE PARAMS
typedef struct {
    unsigned int voice_mode;
    double hs_ratio; // hard sync
    double portamento_time_msec;

    double osc_fo_pitchbend_mod_range;
    double osc_fo_mod_range;
    double osc_hard_sync_mod_range;
    double filter_mod_range;
    double amp_mod_range;

    double filter_keytrack_intensity;

    double lfo1_osc_mod_intensity;
    double lfo1_hs_mod_intensity;
    double lfo1_filter1_mod_intensity;
    double lfo1_filter2_mod_intensity;
    double lfo1_dca_amp_mod_intensity;
    double lfo1_dca_pan_mod_intensity;

    double lfo2_osc_mod_intensity;
    double lfo2_hs_mod_intensity;
    double lfo2_filter1_mod_intensity;
    double lfo2_filter2_mod_intensity;
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

} global_voice_params;

// SYNTH  /////////////////////////////
typedef struct {
    global_voice_params voice_params;
    global_oscillator_params osc1_params;
    global_oscillator_params osc2_params;
    global_oscillator_params osc3_params;
    global_oscillator_params osc4_params;
    global_oscillator_params lfo1_params;
    global_oscillator_params lfo2_params;
    global_filter_params filter1_params;
    global_filter_params filter2_params;
    global_eg_params eg1_params;
    global_eg_params eg2_params;
    global_eg_params eg3_params;
    global_eg_params eg4_params;
    global_dca_params dca_params;
} global_synth_params;

#endif // SBSHELL_SYNTH_FUNCTION_H_
