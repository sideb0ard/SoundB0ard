#include <math.h>
#include <stdlib.h>

#include "minisynth.h"

minisynth *new_minisynth(void)
{
    minisynth *ms = calloc(1, sizeof(minisynth));
    if (ms == NULL)
        return NULL; // barf

    for (int i = 0; i < MAX_VOICES; i++) {
        ms->m_voices[i] = new_minisynth_voice();
        if (!ms->m_voices[i])
            return NULL; // would be bad

        voice_init_global_parameters(&ms->m_voices[i]->m_voice,
                                     &ms->m_global_synth_params);
    }

    // use first voice to setup global
    minisynth_voice_initialize_modmatrix(ms->m_voices[0], &ms->g_modmatrix);

    for (int i = 0; i < MAX_VOICES; i++) {
        voice_set_modmatrix_core(&ms->m_voices[i]->m_voice,
                                 get_matrix_core(&ms->g_modmatrix));
    }

    return ms;
}

// sound generator interface //////////////
// void minisynth_gennext(void* self, double* frame_vals, int framesPerBuffer);
void minisynth_status(void *self, wchar_t *status_string);
void minisynth_setvol(void *self, double v);
double minisynth_getvol(void *self);
////////////////////////////////////

bool minisynth_prepare_for_play(minisynth *ms)
{
    for (int i = 0; i < MAX_VOICES; i++) {
        if (ms->m_voices[i]) {
            voice_prepare_for_play(&ms->m_voices[i]->m_voice);
        }
    }

    // TODO add delay
    minisynth_update(ms);
    ms->m_last_note_frequency = -1.0;

    return true;
}

void minisynth_update(minisynth *ms)
{
    ms->m_global_synth_params.voice_params.voice_mode = ms->m_voice_mode;
    ms->m_global_synth_params.voice_params.portamento_time_msec =
        ms->m_portamento_time_msec;

    ms->m_global_synth_params.voice_params.osc_fo_pitchbend_mod_range =
        ms->m_pitchbend_range;

    // --- intensities
    ms->m_global_synth_params.voice_params.filter_keytrack_intensity =
        ms->m_filter_keytrack_intensity;
    ms->m_global_synth_params.voice_params.lfo1_filter1_mod_intensity =
        ms->m_lfo1_filter_fc_intensity;
    ms->m_global_synth_params.voice_params.lfo1_osc_mod_intensity =
        ms->m_lfo1_osc_pitch_intensity;
    ms->m_global_synth_params.voice_params.lfo1_dca_amp_mod_intensity =
        ms->m_lfo1_amp_intensity;
    ms->m_global_synth_params.voice_params.lfo1_dca_pan_mod_intensity =
        ms->m_lfo1_pan_intensity;

    ms->m_global_synth_params.voice_params.eg1_osc_mod_intensity =
        ms->m_eg1_osc_intensity;
    ms->m_global_synth_params.voice_params.eg1_filter1_mod_intensity =
        ms->m_eg1_filter_intensity;
    ms->m_global_synth_params.voice_params.eg1_dca_amp_mod_intensity =
        ms->m_eg1_dca_intensity;

    // --- oscillators:
    double noise_amplitude = ms->m_noise_osc_db == -96.0
                                 ? 0.0
                                 : pow(10.0, ms->m_noise_osc_db / 20.0);
    double sub_amplitude =
        ms->m_sub_osc_db == -96.0 ? 0.0 : pow(10.0, ms->m_sub_osc_db / 20.0);

    // --- osc3 is sub osc
    ms->m_global_synth_params.osc3_params.amplitude = sub_amplitude;

    // --- osc4 is noise osc
    ms->m_global_synth_params.osc4_params.amplitude = noise_amplitude;

    // --- pulse width
    ms->m_global_synth_params.osc1_params.pulse_width_control =
        ms->m_pulse_width_pct;
    ms->m_global_synth_params.osc2_params.pulse_width_control =
        ms->m_pulse_width_pct;
    ms->m_global_synth_params.osc3_params.pulse_width_control =
        ms->m_pulse_width_pct;

    // --- octave
    ms->m_global_synth_params.osc1_params.octave = ms->m_octave;
    ms->m_global_synth_params.osc2_params.octave = ms->m_octave;
    ms->m_global_synth_params.osc3_params.octave =
        ms->m_octave - 1; // sub-oscillator

    // --- detuning for minisynth
    ms->m_global_synth_params.osc1_params.cents = ms->m_detune_cents;
    ms->m_global_synth_params.osc2_params.cents = -ms->m_detune_cents;
    // no detune on 3rd oscillator

    // --- filter:
    ms->m_global_synth_params.filter1_params.fc_control = ms->m_fc_control;
    ms->m_global_synth_params.filter1_params.q_control = ms->m_q_control;

    // --- lfo1:
    ms->m_global_synth_params.lfo1_params.waveform = ms->m_lfo1_waveform;
    ms->m_global_synth_params.lfo1_params.amplitude = ms->m_lfo1_amplitude;
    ms->m_global_synth_params.lfo1_params.osc_fo = ms->m_lfo1_rate;

    // --- eg1:
    ms->m_global_synth_params.eg1_params.attack_time_msec =
        ms->m_attack_time_msec;
    ms->m_global_synth_params.eg1_params.decay_time_msec =
        ms->m_decay_release_time_msec;

    ms->m_global_synth_params.eg1_params.sustain_level = ms->m_sustain_level;
    ms->m_global_synth_params.eg1_params.release_time_msec =
        ms->m_decay_release_time_msec;
    ms->m_global_synth_params.eg1_params.reset_to_zero =
        (bool)ms->m_reset_to_zero;
    ms->m_global_synth_params.eg1_params.legato_mode = (bool)ms->m_legato_mode;

    // --- dca:
    ms->m_global_synth_params.dca_params.amplitude_db = ms->m_volume_db;

    // --- enable/disable mod matrix stuff
    if (ms->m_velocity_to_attack_scaling == 1)
        enable_matrix_row(&ms->g_modmatrix, SOURCE_VELOCITY,
                          DEST_ALL_EG_ATTACK_SCALING, true); // enable
    else
        enable_matrix_row(&ms->g_modmatrix, SOURCE_VELOCITY,
                          DEST_ALL_EG_ATTACK_SCALING, false);

    if (ms->m_note_number_to_decay_scaling == 1)
        enable_matrix_row(&ms->g_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_EG_DECAY_SCALING, true); // enable
    else
        enable_matrix_row(&ms->g_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_EG_DECAY_SCALING, false);

    if (ms->m_filter_keytrack == 1)
        enable_matrix_row(&ms->g_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_FILTER_KEYTRACK, true); // enable
    else
        enable_matrix_row(&ms->g_modmatrix, SOURCE_MIDI_NOTE_NUM,
                          DEST_ALL_FILTER_KEYTRACK, false);

    // TODO! delay
    // // --- update master FX delay
    // m_DelayFX.setDelayTime_mSec(m_dDelayTime_mSec);
    // m_DelayFX.setFeedback_Pct(m_dFeedback_Pct);
    // m_DelayFX.setDelayRatio(m_dDelayRatio);
    // m_DelayFX.setWetMix(m_dWetMix);
    // m_DelayFX.setMode(m_uDelayMode);
    // m_DelayFX.update();
}

void minisynth_midi_note_on(minisynth *self, unsigned int midinote,
                            unsigned int velocity);
bool minisynth_midi_note_off(minisynth *self, unsigned int midinote,
                             unsigned int velocity, bool all_notes_off);
void minisynth_midi_pitchbend(minisynth *self, unsigned int data1,
                              unsigned int data2);
void minisynth_midi_control(minisynth *self, unsigned int data1,
                            unsigned int data2);
void minisynth_start_note(minisynth *self, int index, unsigned int midinote,
                          unsigned int velocity);
void minisynth_steal_note(minisynth *self, int index,
                          unsigned int pending_midinote,
                          unsigned int pending_velocity);

void change_octave(void *self, int direction);
void minisynth_change_osc_wave_form(minisynth *self, unsigned int voice_no,
                                    int oscil, bool all_voices);
void p_minisynth_change_osc_wave_form(minisynth *self, unsigned int voice_no,
                                      int oscil);
void minisynth_set_sustain(minisynth *self, int sustain_val);
void minisynth_set_multi_melody_mode(minisynth *self, bool melody_mode);
void minisynth_set_melody_loop_num(minisynth *self, int melody_num,
                                   int loop_num);
void minisynth_add_melody(minisynth *self);
void minisynth_switch_melody(minisynth *self, unsigned int melody_num);
void minisynth_reset_melody(minisynth *self, unsigned int melody_num);
void minisynth_reset_melody_all(minisynth *self);
void minisynth_add_note(minisynth *self, int midi_num);
void minisynth_melody_to_string(minisynth *self, int melody_num,
                                wchar_t scratch[33]);

double minisynth_gennext(void *self)
{
    minisynth *ms = (minisynth*) self;

    double accum_out_left = 0.0;
    double accum_out_right = 0.0;

    float mix = 0.25;

    double out_left = 0.0;
    double out_right = 0.0;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        if (ms->m_voices[i])
            minisynth_voice_gennext(ms->m_voices[i], &out_left, &out_right);

        accum_out_left +=  mix * out_left;
        accum_out_right +=  mix * out_right;
    }

    // TODO delay

    return accum_out_left;
}

