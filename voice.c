#include "voice.h"
#include "modmatrix.h"
#include "oscillator.h"
#include "utils.h"
#include <stdlib.h>

void voice_init(voice *v)
{
    v->m_portamento_time_msec = 0.0;
    v->m_note_on = false;
    v->m_timestamp = 0;
    v->m_note_pending = false;
    v->m_voice_mode = 0; // this will vary in meaning depending on synth
    v->m_hs_ratio = 1.0;
    v->m_osc_pitch = OSC_FO_DEFAULT;
    v->m_osc_pitch_pending = OSC_FO_DEFAULT;
    v->m_portamento_start = OSC_FO_DEFAULT;
    v->m_modulo_portamento = OSC_FO_DEFAULT;
    v->m_portamento_inc = 0.0;
    v->m_portamento_semitones = 0.0;
    v->m_legato_mode = LEGATO;
    v->m_default_mod_intensity = 1.0;
    v->m_default_mod_range = 1.0;

    v->m_osc1 = NULL;
    v->m_osc2 = NULL;
    v->m_osc3 = NULL;
    v->m_osc4 = NULL;

    v->m_filter1 = NULL;
    v->m_filter2 = NULL;

    envelope_generator_init(&v->m_eg1);
    envelope_generator_init(&v->m_eg2);
    envelope_generator_init(&v->m_eg3);
    envelope_generator_init(&v->m_eg4);

    osc_new_settings((oscillator *)&v->m_lfo1);
    lfo_set_soundgenerator_interface(&v->m_lfo1);
    osc_new_settings((oscillator *)&v->m_lfo2);
    lfo_set_soundgenerator_interface(&v->m_lfo2);

    dca_initialize(&v->m_dca);

    v->m_global_synth_params = NULL;
    v->m_global_voice_params = NULL;
}

void voice_initialize_modmatrix(voice *v, modmatrix *matrix)
{
    matrixrow *row = NULL;

    // VELOCITY -> DCA VEL
    row = create_matrix_row(SOURCE_VELOCITY, DEST_DCA_VELOCITY,
                            &v->m_default_mod_intensity,
                            &v->m_default_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(matrix, row);

    // PITCHBEND -> OSC
    row = create_matrix_row(
        SOURCE_PITCHBEND, DEST_ALL_OSC_FO, &v->m_default_mod_intensity,
        &v->m_global_voice_params->osc_fo_pitchbend_mod_range, TRANSFORM_NONE,
        false);
    add_matrix_row(matrix, row);

    // MIDI Volume CC07
    row = create_matrix_row(SOURCE_MIDI_VOLUME_CC07, DEST_DCA_AMP,
                            &v->m_default_mod_intensity,
                            &v->m_global_voice_params->amp_mod_range,
                            TRANSFORM_INVERT_MIDI_NORMALIZE, true);
    add_matrix_row(matrix, row);

    // MIDI Pan CC10
    row = create_matrix_row(
        SOURCE_MIDI_PAN_CC10, DEST_DCA_PAN, &v->m_default_mod_intensity,
        &v->m_default_mod_range, TRANSFORM_MIDI_TO_PAN, true);
    add_matrix_row(matrix, row);

    // // MIDI Sustain Pedal
    row =
        create_matrix_row(SOURCE_SUSTAIN_PEDAL, DEST_ALL_EG_SUSTAIN_OVERRIDE,
                          &v->m_default_mod_intensity, &v->m_default_mod_range,
                          TRANSFORM_MIDI_SWITCH, true);
    add_matrix_row(matrix, row);

    // NOTE NUMBER -> FILTER Fc CONTROL
    row = create_matrix_row(
        SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK,
        &v->m_global_voice_params->filter_keytrack_intensity,
        &v->m_default_mod_range, TRANSFORM_NOTE_NUMBER_TO_FREQUENCY,
        false); /* DISABLED BY DEFAULT */
    add_matrix_row(matrix, row);

    // VELOCITY -> EG ATTACK SOURCE_VELOCITY
    // 0 velocity -> scalar = 1, normal attack time
    // 128 velocity -> scalar = 0, fastest (0) attack time:
    row =
        create_matrix_row(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING,
                          &v->m_default_mod_intensity, &v->m_default_mod_range,
                          TRANSFORM_MIDI_NORMALIZE, false);
    add_matrix_row(matrix, row);

    // NOTE NUMBER -> EG DECAY SCALING
    row =
        create_matrix_row(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING,
                          &v->m_default_mod_intensity, &v->m_default_mod_range,
                          TRANSFORM_MIDI_NORMALIZE, false);
    add_matrix_row(matrix, row);
}

void voice_prepare_for_play(voice *v)
{
    matrix_clear_sources(&v->m_v_modmatrix);
    v->m_v_modmatrix.m_sources[SOURCE_MIDI_VOLUME_CC07] = 127;
    v->m_v_modmatrix.m_sources[SOURCE_MIDI_PAN_CC10] = 64;

    v->m_lfo1.osc.m_v_modmatrix = &v->m_v_modmatrix;
    v->m_lfo1.osc.m_mod_dest_output1 = SOURCE_LFO1;
    v->m_lfo1.osc.m_mod_dest_output2 = SOURCE_LFO1Q;

    v->m_lfo2.osc.m_v_modmatrix = &v->m_v_modmatrix;
    v->m_lfo2.osc.m_mod_dest_output1 = SOURCE_LFO2;
    v->m_lfo2.osc.m_mod_dest_output2 = SOURCE_LFO2Q;

    v->m_eg1.m_v_modmatrix = &v->m_v_modmatrix;
    v->m_eg1.m_mod_dest_eg_output = SOURCE_EG1;
    v->m_eg1.m_mod_dest_eg_biased_output = SOURCE_BIASED_EG1;
    v->m_eg1.m_mod_source_eg_attack_scaling = DEST_EG1_ATTACK_SCALING;
    v->m_eg1.m_mod_source_eg_decay_scaling = DEST_EG1_DECAY_SCALING;
    v->m_eg1.m_mod_source_sustain_override = DEST_EG1_SUSTAIN_OVERRIDE;

    v->m_eg2.m_v_modmatrix = &v->m_v_modmatrix;
    v->m_eg2.m_mod_dest_eg_output = SOURCE_EG2;
    v->m_eg2.m_mod_dest_eg_biased_output = SOURCE_BIASED_EG2;
    v->m_eg2.m_mod_source_eg_attack_scaling = DEST_EG2_ATTACK_SCALING;
    v->m_eg2.m_mod_source_eg_decay_scaling = DEST_EG2_DECAY_SCALING;
    v->m_eg2.m_mod_source_sustain_override = DEST_EG2_SUSTAIN_OVERRIDE;

    v->m_eg3.m_v_modmatrix = &v->m_v_modmatrix;
    v->m_eg3.m_mod_dest_eg_output = SOURCE_EG3;
    v->m_eg3.m_mod_dest_eg_biased_output = SOURCE_BIASED_EG3;
    v->m_eg3.m_mod_source_eg_attack_scaling = DEST_EG3_ATTACK_SCALING;
    v->m_eg3.m_mod_source_eg_decay_scaling = DEST_EG3_DECAY_SCALING;
    v->m_eg3.m_mod_source_sustain_override = DEST_EG3_SUSTAIN_OVERRIDE;

    v->m_eg4.m_v_modmatrix = &v->m_v_modmatrix;
    v->m_eg4.m_mod_dest_eg_output = SOURCE_EG4;
    v->m_eg4.m_mod_dest_eg_biased_output = SOURCE_BIASED_EG4;
    v->m_eg4.m_mod_source_eg_attack_scaling = DEST_EG4_ATTACK_SCALING;
    v->m_eg4.m_mod_source_eg_decay_scaling = DEST_EG4_DECAY_SCALING;
    // v->m_eg4.m_mod_source_sustain_override = DEST_EG4_SUSTAIN_OVERRIDE;

    v->m_dca.m_v_modmatrix = &v->m_v_modmatrix;
    v->m_dca.m_mod_source_eg = DEST_DCA_EG;
    v->m_dca.m_mod_source_amp_db = DEST_DCA_AMP;
    v->m_dca.m_mod_source_velocity = DEST_DCA_VELOCITY;
    v->m_dca.m_mod_source_pan = DEST_DCA_PAN;

    if (v->m_osc1)
    {
        v->m_osc1->m_v_modmatrix = &v->m_v_modmatrix;
        v->m_osc1->m_mod_source_fo = DEST_OSC1_FO;
        v->m_osc1->m_mod_source_pulse_width = DEST_OSC1_PULSEWIDTH;
        v->m_osc1->m_mod_source_amp = DEST_OSC1_OUTPUT_AMP;
    }

    if (v->m_osc2)
    {
        v->m_osc2->m_v_modmatrix = &v->m_v_modmatrix;
        v->m_osc2->m_mod_source_fo = DEST_OSC2_FO;
        v->m_osc2->m_mod_source_pulse_width = DEST_OSC2_PULSEWIDTH;
        v->m_osc2->m_mod_source_amp = DEST_OSC2_OUTPUT_AMP;
    }

    if (v->m_osc3)
    {
        v->m_osc3->m_v_modmatrix = &v->m_v_modmatrix;
        v->m_osc3->m_mod_source_fo = DEST_OSC3_FO;
        v->m_osc3->m_mod_source_pulse_width = DEST_OSC3_PULSEWIDTH;
        v->m_osc3->m_mod_source_amp = DEST_OSC3_OUTPUT_AMP;
    }

    if (v->m_osc4)
    {
        v->m_osc4->m_v_modmatrix = &v->m_v_modmatrix;
        v->m_osc4->m_mod_source_fo = DEST_OSC4_FO;
        v->m_osc4->m_mod_source_pulse_width = DEST_OSC4_PULSEWIDTH;
        v->m_osc4->m_mod_source_amp = DEST_OSC4_OUTPUT_AMP;
    }

    if (v->m_filter1)
    {
        v->m_filter1->m_v_modmatrix = &v->m_v_modmatrix;
        v->m_filter1->m_mod_source_fc = DEST_FILTER1_FC;
        v->m_filter1->m_mod_source_fc_control = DEST_ALL_FILTER_KEYTRACK;
    }

    if (v->m_filter2)
    {
        v->m_filter2->m_v_modmatrix = &v->m_v_modmatrix;
        v->m_filter2->m_mod_source_fc = DEST_FILTER2_FC;
        v->m_filter2->m_mod_source_fc_control = DEST_ALL_FILTER_KEYTRACK;
    }
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
                                   &v->m_global_synth_params->osc2_params);
    if (v->m_osc3)
        osc_init_global_parameters(v->m_osc3,
                                   &v->m_global_synth_params->osc3_params);
    if (v->m_osc4)
        osc_init_global_parameters(v->m_osc4,
                                   &v->m_global_synth_params->osc4_params);

    if (v->m_filter1)
        filter_init_global_parameters(
            v->m_filter1, &v->m_global_synth_params->filter1_params);
    if (v->m_filter2)
        filter_init_global_parameters(
            v->m_filter2, &v->m_global_synth_params->filter2_params);

    eg_init_global_parameters(&v->m_eg1, &v->m_global_synth_params->eg1_params);
    eg_init_global_parameters(&v->m_eg2, &v->m_global_synth_params->eg2_params);
    eg_init_global_parameters(&v->m_eg3, &v->m_global_synth_params->eg3_params);
    eg_init_global_parameters(&v->m_eg4, &v->m_global_synth_params->eg4_params);

    osc_init_global_parameters((oscillator *)&v->m_lfo1,
                               &v->m_global_synth_params->lfo1_params);
    osc_init_global_parameters((oscillator *)&v->m_lfo2,
                               &v->m_global_synth_params->lfo2_params);

    dca_init_global_parameters(&v->m_dca,
                               &v->m_global_synth_params->dca_params);
}

void voice_set_modmatrix_core(voice *v, matrixrow **modmatrix)
{
    set_matrix_core(&v->m_v_modmatrix, modmatrix);
}

bool voice_is_active_voice(voice *v)
{
    if (v->m_note_on && eg_is_active(&v->m_eg1))
        return true;
    return false;
}

bool voice_can_note_off(voice *v)
{
    if (v->m_note_on && eg_can_note_off(&v->m_eg1))
        return true;
    return false;
}

bool voice_is_voice_done(voice *v)
{
    if (eg_get_state(&v->m_eg1) == OFFF)
        return true;
    return false;
}

bool voice_in_legato_mode(voice *v) { return v->m_eg1.m_legato_mode; }

void voice_update(voice *v)
{
    v->m_voice_mode = v->m_global_voice_params->voice_mode;
    if (v->m_portamento_time_msec !=
        v->m_global_voice_params->portamento_time_msec)
    {
        v->m_portamento_time_msec =
            v->m_global_voice_params->portamento_time_msec;
        if (v->m_portamento_time_msec == 0.0)
            v->m_portamento_inc = 0.0;
        else
            v->m_portamento_inc =
                1000.0 / v->m_portamento_time_msec / SAMPLE_RATE;
    }

    v->m_hs_ratio = v->m_global_voice_params->hs_ratio;
}

void voice_reset(voice *v)
{
    if (v->m_osc1)
        v->m_osc1->reset_oscillator(v->m_osc1);
    if (v->m_osc2)
        v->m_osc2->reset_oscillator(v->m_osc2);
    if (v->m_osc3)
        v->m_osc3->reset_oscillator(v->m_osc3);
    if (v->m_osc4)
        v->m_osc4->reset_oscillator(v->m_osc4);

    if (v->m_filter1)
        filter_reset(v->m_filter1);
    if (v->m_filter2)
        filter_reset(v->m_filter2);

    eg_reset(&v->m_eg1);
    eg_reset(&v->m_eg2);
    eg_reset(&v->m_eg3);
    eg_reset(&v->m_eg4);

    lfo_reset_oscillator((oscillator *)&v->m_lfo1);
    lfo_reset_oscillator((oscillator *)&v->m_lfo2);

    dca_reset(&v->m_dca);
}

void voice_note_on(voice *v, unsigned int midi_note, unsigned int midi_velocity,
                   double frequency, double last_note_frequency)
{
    v->m_osc_pitch = frequency;

    if (!v->m_note_on && !v->m_note_pending)
    {
        v->m_midi_note_number = midi_note;
        v->m_midi_velocity = midi_velocity;
        v->m_v_modmatrix.m_sources[SOURCE_VELOCITY] =
            (double)v->m_midi_velocity;
        v->m_v_modmatrix.m_sources[SOURCE_MIDI_NOTE_NUM] =
            (double)v->m_midi_note_number;
        if (v->m_portamento_inc > 0.0 && last_note_frequency >= 0)
        {
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
        else
        {
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
        if (v->m_osc4)
            v->m_osc4->start_oscillator(v->m_osc4);

        eg_start_eg(&v->m_eg1);
        eg_start_eg(&v->m_eg2);
        eg_start_eg(&v->m_eg3);
        eg_start_eg(&v->m_eg4);

        v->m_lfo1.osc.start_oscillator((oscillator *)&v->m_lfo1);
        v->m_lfo2.osc.start_oscillator((oscillator *)&v->m_lfo2);

        v->m_note_on = true;
        v->m_timestamp = 0;

        return; // ESCAPE HATCH, ALL DONE
    }

    // if we get here, we need to steal a note
    //
    // this checks if we're already stealing this note - then no-op, just rturn
    if (v->m_note_pending && v->m_midi_note_number_pending == midi_note)
        return;

    v->m_midi_note_number_pending = midi_note;
    v->m_midi_velocity_pending = midi_velocity;
    v->m_osc_pitch_pending = frequency;

    v->m_note_pending = true;

    if (v->m_portamento_inc > 0.0 && last_note_frequency > 0)
    {
        if (v->m_modulo_portamento > 0.0)
        {
            double portamento_pitch_mult = pitch_shift_multiplier(
                v->m_modulo_portamento * v->m_portamento_semitones);
            v->m_portamento_start =
                v->m_portamento_start * portamento_pitch_mult;
        }
        else
        {
            v->m_portamento_start = last_note_frequency;
        }
        v->m_modulo_portamento = 0.0;
        v->m_portamento_semitones =
            semitones_between_frequencies(v->m_portamento_start, frequency);
    }

    eg_shutdown(&v->m_eg1);
    eg_shutdown(&v->m_eg2);
    eg_shutdown(&v->m_eg3);
    eg_shutdown(&v->m_eg4);
}

void voice_note_off(voice *v, unsigned int midi_note)
{
    if (v->m_note_on && voice_can_note_off(v))
    {
        if (v->m_note_pending && (midi_note == v->m_midi_note_number_pending))
        {
            v->m_note_pending = false;
            return;
        }
        if (midi_note != v->m_midi_note_number)
        {
            return;
        }

        eg_note_off(&v->m_eg1);
        eg_note_off(&v->m_eg2);
        eg_note_off(&v->m_eg3);
        eg_note_off(&v->m_eg4);
    }
}

bool voice_gennext(voice *v, double *left_output, double *right_output)
{
    *left_output = 0.0;
    *right_output = 0.0;

    if (!v->m_note_on)
        return false;

    if (voice_is_voice_done(v) || v->m_note_pending)
    {
        if (voice_is_voice_done(v) && !v->m_note_pending)
        {
            if (v->m_osc1)
                v->m_osc1->stop_oscillator(v->m_osc1);
            if (v->m_osc2)
                v->m_osc2->stop_oscillator(v->m_osc2);
            if (v->m_osc3)
                v->m_osc3->stop_oscillator(v->m_osc3);
            if (v->m_osc4)
                v->m_osc4->stop_oscillator(v->m_osc4);

            if (v->m_osc1)
                v->m_osc1->reset_oscillator(v->m_osc1);
            if (v->m_osc2)
                v->m_osc2->reset_oscillator(v->m_osc2);
            if (v->m_osc3)
                v->m_osc3->reset_oscillator(v->m_osc3);
            if (v->m_osc4)
                v->m_osc4->reset_oscillator(v->m_osc4);

            lfo_stop_oscillator((oscillator *)&v->m_lfo1);
            lfo_stop_oscillator((oscillator *)&v->m_lfo2);

            eg_reset(&v->m_eg1);
            eg_reset(&v->m_eg2);
            eg_reset(&v->m_eg3);
            eg_reset(&v->m_eg4);

            v->m_note_on = false;

            return false;
        }
        else if (v->m_note_pending &&
                 (voice_is_voice_done(v) || voice_in_legato_mode(v)))
        {
            v->m_midi_note_number = v->m_midi_note_number_pending;
            v->m_midi_velocity = v->m_midi_velocity_pending;
            v->m_osc_pitch = v->m_osc_pitch_pending;
            v->m_timestamp = 0;

            v->m_v_modmatrix.m_sources[SOURCE_MIDI_NOTE_NUM] =
                (double)v->m_midi_note_number;

            if (!voice_in_legato_mode(v))
                v->m_v_modmatrix.m_sources[SOURCE_VELOCITY] =
                    (double)v->m_midi_velocity;

            double pitch = v->m_portamento_inc > 0.0 ? v->m_portamento_start
                                                     : v->m_osc_pitch;

            if (v->m_osc1)
                v->m_osc1->m_osc_fo = pitch;
            if (v->m_osc2)
                v->m_osc2->m_osc_fo = pitch;
            if (v->m_osc3)
                v->m_osc3->m_osc_fo = pitch;
            if (v->m_osc4)
                v->m_osc4->m_osc_fo = pitch;

            if (v->m_osc1)
                v->m_osc1->m_midi_note_number = v->m_midi_note_number;
            if (v->m_osc2)
                v->m_osc2->m_midi_note_number = v->m_midi_note_number;
            if (v->m_osc3)
                v->m_osc3->m_midi_note_number = v->m_midi_note_number;
            if (v->m_osc4)
                v->m_osc4->m_midi_note_number = v->m_midi_note_number;

            if (!v->m_legato_mode)
            {
                if (v->m_osc1)
                    v->m_osc1->reset_oscillator(v->m_osc1);
                if (v->m_osc2)
                    v->m_osc2->reset_oscillator(v->m_osc2);
                if (v->m_osc3)
                    v->m_osc3->reset_oscillator(v->m_osc3);
                if (v->m_osc4)
                    v->m_osc4->reset_oscillator(v->m_osc4);
            }

            eg_start_eg(&v->m_eg1);
            eg_start_eg(&v->m_eg2);
            eg_start_eg(&v->m_eg3);
            eg_start_eg(&v->m_eg4);

            v->m_lfo1.osc.start_oscillator((oscillator *)&v->m_lfo1);
            v->m_lfo2.osc.start_oscillator((oscillator *)&v->m_lfo2);

            v->m_note_pending = false;
        }
    }

    // portamento block
    if (v->m_portamento_inc > 0.0 && v->m_osc1->m_osc_fo != v->m_osc_pitch)
    {
        if (v->m_modulo_portamento >= 1.0)
        {
            v->m_modulo_portamento = 0.0;

            if (v->m_osc1)
                v->m_osc1->m_osc_fo = v->m_osc_pitch;
            if (v->m_osc2)
                v->m_osc2->m_osc_fo = v->m_osc_pitch;
            if (v->m_osc3)
                v->m_osc3->m_osc_fo = v->m_osc_pitch;
            if (v->m_osc4)
                v->m_osc4->m_osc_fo = v->m_osc_pitch;
        }
        else
        {
            double portamento_pitch =
                v->m_portamento_start *
                pitch_shift_multiplier(v->m_modulo_portamento *
                                       v->m_portamento_semitones);
            if (v->m_osc1)
                v->m_osc1->m_osc_fo = portamento_pitch;
            if (v->m_osc2)
                v->m_osc2->m_osc_fo = portamento_pitch;
            if (v->m_osc3)
                v->m_osc3->m_osc_fo = portamento_pitch;
            if (v->m_osc4)
                v->m_osc4->m_osc_fo = portamento_pitch;

            v->m_modulo_portamento += v->m_portamento_inc;
        }
    }
    return true;
}

void voice_set_sustain_override(voice *v, bool b)
{
    eg_set_sustain_override(&v->m_eg1, b);
    eg_set_sustain_override(&v->m_eg2, b);
    eg_set_sustain_override(&v->m_eg3, b);
    eg_set_sustain_override(&v->m_eg4, b);
}
