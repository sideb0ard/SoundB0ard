#include "voice.h"
#include "modmatrix.h"
#include "oscillator.h"
#include "utils.h"
#include <stdlib.h>

voice *new_voice()
{
    voice *v = (voice *)calloc(1, sizeof(voice));
    if (!v) {
        printf("youch");
        return NULL;
    }

    return v;
}

void voice_init_global_parameters(voice *v, global_synth_params *sp)
{
    v->m_global_synth_params = sp;

    v->m_global_voice_params = &sp->voice_params;
    v->m_global_voice_params->voice_mode = v->m_voice_mode;
    v->m_global_voice_params->hs_ratio = v->m_hs_ratio;
    v->m_global_voice_params->portamento_time_msec = v->m_portamento_time_msec;

    v->m_global_voice_params->osc_fo_pitchbend_mod_range =
        OSC_PITCHBEND_MOD_RANGE;
    v->m_global_voice_params->osc_fo_mod_range = OSC_FO_MOD_RANGE;
    v->m_global_voice_params->osc_hard_sync_mod_range =
        OSC_HARD_SYNC_RATIO_RANGE;
    v->m_global_voice_params->filter_mod_range = FILTER_FC_MOD_RANGE;
    v->m_global_voice_params->amp_mod_range = AMP_MOD_RANGE;

    // --- Intensity variables
    v->m_global_voice_params->filter_keytrack_intensity = 1.0;

    // --- init sub-components
    if (v->m_osc1)
        osc_init_global_parameters(v->m_osc1,
                                   &v->m_global_synth_params->osc1_params);
    if (v->m_osc2)
        osc_init_global_parameters(v->m_osc2,
                                   &v->m_global_synth_params->osc1_params);
    if (v->m_osc3)
        osc_init_global_parameters(v->m_osc3,
                                   &v->m_global_synth_params->osc1_params);
    if (v->m_osc4)
        osc_init_global_parameters(v->m_osc4,
                                   &v->m_global_synth_params->osc1_params);

    if (v->m_filter1)
        filter_init_global_parameters(
            v->m_filter1, &v->m_global_synth_params->filter1_params);
    if (v->m_filter2)
        filter_init_global_parameters(
            v->m_filter2, &v->m_global_synth_params->filter2_params);

    eg_init_global_parameters(v->m_eg1, &v->m_global_synth_params->eg1_params);
    eg_init_global_parameters(v->m_eg2, &v->m_global_synth_params->eg2_params);
    eg_init_global_parameters(v->m_eg3, &v->m_global_synth_params->eg3_params);
    eg_init_global_parameters(v->m_eg4, &v->m_global_synth_params->eg4_params);

    osc_init_global_parameters((oscillator *)&v->m_lfo1,
                               &v->m_global_synth_params->lfo1_params);
    osc_init_global_parameters((oscillator *)&v->m_lfo2,
                               &v->m_global_synth_params->lfo2_params);

    dca_init_global_parameters(&v->m_dca,
                               &v->m_global_synth_params->dca_params);
}
void voice_set_modmatrix_core(voice *v, matrixrow **modmatrix)
{
    set_matrix_core(&v->g_modmatrix, modmatrix);
}

// void voice_initialize_modmatrix(voice *v, modmatrix *matrix)
//{
//    // NO-OP - voice specific
//}

bool voice_is_active_voice(voice *v)
{
    if (v->m_note_on && eg_is_active(v->m_eg1))
        return true;
    return false;
}

bool voice_can_note_off(voice *v)
{
    if (v->m_note_on && eg_can_note_off(v->m_eg1))
        return true;
    return false;
}

bool voice_is_voice_done(voice *v)
{
    if (eg_get_state(v->m_eg1) == OFFF)
        return true;
    return false;
}

bool voice_in_legato_mode(voice *v) { return v->m_eg1->m_legato_mode; }

void voice_note_on(voice *v, unsigned int midi_note, unsigned int midi_velocity,
                   double frequency, double last_note_frequency)
{
    v->m_osc_pitch = frequency;

    if (!v->m_note_on && !v->m_note_pending) {
        v->m_midi_note_number = midi_note;
        v->m_midi_velocity = midi_velocity;
        v->g_modmatrix.m_sources[SOURCE_VELOCITY] = (double)v->m_midi_velocity;
        v->g_modmatrix.m_sources[SOURCE_MIDI_NOTE_NUM] =
            (double)v->m_midi_note_number;
        if (v->m_portamento_inc > 0.0 && last_note_frequency >= 0) {
            v->m_modulo_portamento = 0.0;
            v->m_portamento_semitones =
                semitones_between_frequencies(last_note_frequency, frequency);
            v->m_portamento_start = last_note_frequency;

            if (v->m_osc1)
                v->m_osc1->m_osc_fo = v->m_portamento_start;
            if (v->m_osc2)
                v->m_osc2->m_osc_fo = v->m_portamento_start;
            if (v->m_osc3)
                v->m_osc3->m_osc_fo = v->m_portamento_start;
            if (v->m_osc4)
                v->m_osc4->m_osc_fo = v->m_portamento_start;
        }
        else {
            if (v->m_osc1)
                v->m_osc1->m_osc_fo = v->m_osc_pitch;
            if (v->m_osc2)
                v->m_osc2->m_osc_fo = v->m_osc_pitch;
            if (v->m_osc3)
                v->m_osc3->m_osc_fo = v->m_osc_pitch;
            if (v->m_osc4)
                v->m_osc4->m_osc_fo = v->m_osc_pitch;
        }

        if (v->m_osc1)
            v->m_osc1->m_midi_note_number = v->m_midi_note_number;
        if (v->m_osc2)
            v->m_osc2->m_midi_note_number = v->m_midi_note_number;
        if (v->m_osc3)
            v->m_osc3->m_midi_note_number = v->m_midi_note_number;
        if (v->m_osc4)
            v->m_osc4->m_midi_note_number = v->m_midi_note_number;

        if (v->m_osc1)
            v->m_osc1->start_oscillator(v->m_osc1);
        if (v->m_osc2)
            v->m_osc2->start_oscillator(v->m_osc2);
        if (v->m_osc3)
            v->m_osc3->start_oscillator(v->m_osc3);
        if (v->m_osc3)
            v->m_osc4->start_oscillator(v->m_osc4);

        eg_start_eg(v->m_eg1);
        eg_start_eg(v->m_eg2);
        eg_start_eg(v->m_eg3);
        eg_start_eg(v->m_eg4);

        v->m_lfo1.osc.start_oscillator((oscillator *)&v->m_lfo1);
        v->m_lfo2.osc.start_oscillator((oscillator *)&v->m_lfo2);

        v->m_note_on = true;
        v->m_timestamp = 0;
        return;
    }
}
void voice_note_off(voice *v, unsigned int midi_note);
bool voice_gennext(voice *v, double *left_output, double *right_output);
