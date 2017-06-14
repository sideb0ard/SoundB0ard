#include "minisynth_voice.h"
#include <stdio.h>
#include <stdlib.h>

minisynth_voice *new_minisynth_voice(void)
{
    minisynth_voice *msv =
        (minisynth_voice *)calloc(1, sizeof(minisynth_voice));
    if (msv == NULL)
        return NULL;
    minisynth_voice_init(msv);

    return msv;
}

void minisynth_voice_init(minisynth_voice *msv)
{
    // initializes 4 x envelope generators and
    // 2 x lfos, and dca in base class
    voice_init(&msv->m_voice);

    // initialize all my oscillators
    osc_new_settings(&msv->m_osc1.osc);
    qb_set_soundgenerator_interface(&msv->m_osc1);
    osc_new_settings(&msv->m_osc2.osc);
    qb_set_soundgenerator_interface(&msv->m_osc2);
    osc_new_settings(&msv->m_osc3.osc);
    qb_set_soundgenerator_interface(&msv->m_osc3);
    osc_new_settings(&msv->m_osc4.osc);
    qb_set_soundgenerator_interface(&msv->m_osc4);

    // attach oscillators to my base class
    msv->m_voice.m_osc1 = (oscillator *)&msv->m_osc1;
    msv->m_voice.m_osc2 = (oscillator *)&msv->m_osc2;
    msv->m_voice.m_osc3 = (oscillator *)&msv->m_osc3;
    msv->m_voice.m_osc4 = (oscillator *)&msv->m_osc4;

    // initialize my filter
    filter_moog_init(&msv->m_moog_ladder_filter);

    // attach to base class
    msv->m_voice.m_filter1 = (filter *)&msv->m_moog_ladder_filter;
    msv->m_voice.m_filter2 = NULL;

    msv->m_moog_ladder_filter.f.m_aux_control = 0.0;

    // voice mode 0
    msv->m_osc1.osc.m_waveform = SAW1;
    msv->m_osc2.osc.m_waveform = SAW1;
    msv->m_osc3.osc.m_waveform = SAW1;
    msv->m_osc4.osc.m_waveform = NOISE;

    eg_set_eg_mode(&msv->m_voice.m_eg1, ANALOG);
    msv->m_voice.m_eg1.m_output_eg = true;

    dca_initialize(&msv->m_voice.m_dca);
    msv->m_voice.m_dca.m_mod_source_eg = DEST_DCA_EG;
}

void minisynth_voice_initialize_modmatrix(minisynth_voice *msv,
                                          modmatrix *matrix)
{
    voice_initialize_modmatrix(&msv->m_voice, matrix);

    if (!get_matrix_core(matrix))
        return;

    matrixrow *row = NULL;

    // LFO1 -> ALL OSC1 FO
    row = create_matrix_row(
        SOURCE_LFO1, DEST_ALL_OSC_FO,
        &msv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &msv->m_voice.m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE,
        true);
    add_matrix_row(matrix, row);

    // EG1 -> FILTER1 FC
    row = create_matrix_row(
        SOURCE_BIASED_EG1, DEST_ALL_FILTER_FC,
        &msv->m_voice.m_global_voice_params->eg1_filter1_mod_intensity,
        &msv->m_voice.m_global_voice_params->filter_mod_range, TRANSFORM_NONE,
        true);
    add_matrix_row(matrix, row);

    // EG1 -> DCA EG
    row = create_matrix_row(
        SOURCE_EG1, DEST_DCA_EG,
        &msv->m_voice.m_global_voice_params->eg1_dca_amp_mod_intensity,
        &msv->m_voice.m_default_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(matrix, row);

    // EG1 -> ALL OSC
    row = create_matrix_row(
        SOURCE_BIASED_EG1, DEST_ALL_OSC_FO,
        &msv->m_voice.m_global_voice_params->eg1_osc_mod_intensity,
        &msv->m_voice.m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE,
        true);
    add_matrix_row(matrix, row);

    // LFO1 -> FILTER1 FC
    row = create_matrix_row(
        SOURCE_LFO1, DEST_ALL_FILTER_FC,
        &msv->m_voice.m_global_voice_params->lfo1_filter1_mod_intensity,
        &msv->m_voice.m_global_voice_params->filter_mod_range, TRANSFORM_NONE,
        true);
    add_matrix_row(matrix, row);

    // LFO1 -> PULSE WIDTH
    row = create_matrix_row(SOURCE_LFO1, DEST_ALL_OSC_PULSEWIDTH,
                            &msv->m_voice.m_default_mod_intensity,
                            &msv->m_voice.m_default_mod_range, TRANSFORM_NONE,
                            true);
    add_matrix_row(matrix, row);

    // LFO1 (-1 -> +1) -> DCA Amp Mod (0->1)
    row = create_matrix_row(
        SOURCE_LFO1, DEST_DCA_AMP,
        &msv->m_voice.m_global_voice_params->lfo1_dca_amp_mod_intensity,
        &msv->m_voice.m_global_voice_params->amp_mod_range,
        TRANSFORM_BIPOLAR_TO_UNIPOLAR, true);
    add_matrix_row(matrix, row);

    // LFO1 (-1 -> +1) -> DCA Pan Mod (0->1)
    row = create_matrix_row(
        SOURCE_LFO1, DEST_DCA_PAN,
        &msv->m_voice.m_global_voice_params->lfo1_dca_pan_mod_intensity,
        &msv->m_voice.m_default_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(matrix, row);
}

void minisynth_voice_init_global_parameters(minisynth_voice *msv,
                                            global_synth_params *sp)
{
    voice_init_global_parameters(&msv->m_voice, sp);

    msv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity = 1.0;
    msv->m_voice.m_global_voice_params->lfo1_filter1_mod_intensity = 1.0;
    msv->m_voice.m_global_voice_params->lfo1_filter2_mod_intensity = 1.0;
    msv->m_voice.m_global_voice_params->lfo1_dca_pan_mod_intensity = 1.0;
    msv->m_voice.m_global_voice_params->lfo1_dca_amp_mod_intensity = 1.0;

    msv->m_voice.m_global_voice_params->eg1_osc_mod_intensity = 1.0;
    msv->m_voice.m_global_voice_params->eg1_filter1_mod_intensity = 1.0;
    msv->m_voice.m_global_voice_params->eg1_filter2_mod_intensity = 1.0;
    msv->m_voice.m_global_voice_params->eg1_dca_amp_mod_intensity = 1.0;
}

void minisynth_voice_prepare_for_play(minisynth_voice *msv)
{
    voice_prepare_for_play(&msv->m_voice);
    minisynth_voice_reset(msv);
}

void minisynth_voice_update(minisynth_voice *msv)
{
    if (!msv->m_voice.m_global_voice_params)
        return;
    unsigned int current_voice_mode = msv->m_voice.m_voice_mode;

    voice_update(&msv->m_voice);
    if (msv->m_voice.m_voice_mode != current_voice_mode) {
        msv->m_voice.m_voice_mode =
            msv->m_voice.m_global_voice_params->voice_mode;
        msv->m_osc3.osc.m_octave = -1.0;
        msv->m_voice.m_global_synth_params->osc4_params.waveform = NOISE;

        switch (msv->m_voice.m_voice_mode) {
        case Saw3:
            msv->m_voice.m_global_synth_params->osc1_params.waveform = SAW1;
            msv->m_voice.m_global_synth_params->osc2_params.waveform = SAW1;
            msv->m_voice.m_global_synth_params->osc3_params.waveform = SAW1;
            break;
        case Sqr3:
            msv->m_voice.m_global_synth_params->osc1_params.waveform = SQUARE;
            msv->m_voice.m_global_synth_params->osc2_params.waveform = SQUARE;
            msv->m_voice.m_global_synth_params->osc3_params.waveform = SQUARE;
            break;
        case Saw2Sqr:
            msv->m_voice.m_global_synth_params->osc1_params.waveform = SAW1;
            msv->m_voice.m_global_synth_params->osc2_params.waveform = SQUARE;
            msv->m_voice.m_global_synth_params->osc3_params.waveform = SAW1;
            break;
        case Tri2Saw:
            msv->m_voice.m_global_synth_params->osc1_params.waveform = TRI;
            msv->m_voice.m_global_synth_params->osc2_params.waveform = SAW1;
            msv->m_voice.m_global_synth_params->osc3_params.waveform = TRI;
            break;
        case Tri2Sqr:
            msv->m_voice.m_global_synth_params->osc1_params.waveform = TRI;
            msv->m_voice.m_global_synth_params->osc2_params.waveform = SQUARE;
            msv->m_voice.m_global_synth_params->osc3_params.waveform = TRI;
            break;
        default:
            msv->m_voice.m_global_synth_params->osc1_params.waveform = SAW1;
            msv->m_voice.m_global_synth_params->osc2_params.waveform = SAW1;
            msv->m_voice.m_global_synth_params->osc3_params.waveform = SAW1;
            break;
        }
        msv->m_osc1.osc.reset_oscillator((oscillator *)&msv->m_osc1);
        msv->m_osc2.osc.reset_oscillator((oscillator *)&msv->m_osc2);
        msv->m_osc3.osc.reset_oscillator((oscillator *)&msv->m_osc3);
        msv->m_osc4.osc.reset_oscillator((oscillator *)&msv->m_osc4);
    }
}

void minisynth_voice_reset(minisynth_voice *msv)
{
    voice_reset(&msv->m_voice);
    msv->m_voice.m_portamento_inc = 0.0;
    msv->m_osc1.osc.m_waveform = SAW1;
}

bool minisynth_voice_gennext(minisynth_voice *msv, double *left_output,
                             double *right_output)
{
    if (!voice_gennext(&msv->m_voice, left_output, right_output)) {
        return false;
    }

    minisynth_voice_update(msv);

    //// layer 0 //////////////////////////////
    do_modulation_matrix(&msv->m_voice.m_v_modmatrix, 0);

    ////// update layer 1 modulators
    eg_update(&msv->m_voice.m_eg1);
    osc_update((oscillator *)&msv->m_voice.m_lfo1);

    ////// gen next val layer 1 mods
    eg_do_envelope(&msv->m_voice.m_eg1, NULL);
    lfo_do_oscillate((oscillator *)&msv->m_voice.m_lfo1, NULL);

    ////// layer 1 //////////////////////////////
    do_modulation_matrix(&msv->m_voice.m_v_modmatrix, 1);

    dca_update(&msv->m_voice.m_dca);
    moog_update((filter *)&msv->m_moog_ladder_filter);

    osc_update((oscillator *)&msv->m_osc1);
    osc_update((oscillator *)&msv->m_osc2);
    osc_update((oscillator *)&msv->m_osc3);
    osc_update((oscillator *)&msv->m_osc4);

    double osc_mix = 0.333 * qb_do_oscillate((oscillator *)&msv->m_osc1, NULL) +
                     0.333 * qb_do_oscillate((oscillator *)&msv->m_osc2, NULL) +
                     0.333 * qb_do_oscillate((oscillator *)&msv->m_osc3, NULL) +
                     0.333 * qb_do_oscillate((oscillator *)&msv->m_osc4, NULL);

    double filter_out =
        moog_gennext((filter *)&msv->m_moog_ladder_filter, osc_mix);

    dca_gennext(&msv->m_voice.m_dca, filter_out, filter_out, left_output,
                right_output);

    return true;
}

void minisynth_voice_set_filter_mod(minisynth_voice *ms, double mod)
{
    filter_set_fc_mod(&ms->m_moog_ladder_filter.f, mod);
}

void minisynth_voice_free_self(minisynth_voice *ms) { free(ms); }
