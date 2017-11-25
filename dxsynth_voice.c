#include <dxsynth_voice.h>
#include <stdlib.h>

#define IMAX 4.0

dxsynth_voice *new_dxsynth_voice(void)
{
    dxsynth_voice *dxv = (dxsynth_voice *)calloc(1, sizeof(dxsynth_voice));
    if (dxv == NULL)
        return NULL;
    dxsynth_voice_init(dxv);

    return dxv;
}

void dxsynth_voice_init(dxsynth_voice *dxv)
{
    voice_init(&dxv->m_voice);

    // initialize all oscillators
    osc_new_settings(&dxv->m_op1.osc);
    qb_set_soundgenerator_interface(&dxv->m_op1);
    osc_new_settings(&dxv->m_op2.osc);
    qb_set_soundgenerator_interface(&dxv->m_op2);
    osc_new_settings(&dxv->m_op3.osc);
    qb_set_soundgenerator_interface(&dxv->m_op3);
    osc_new_settings(&dxv->m_op4.osc);
    qb_set_soundgenerator_interface(&dxv->m_op4);
    // or use wavetable versions
    // wt_initialize(&dxv->m_op1);
    // wt_initialize(&dxv->m_op2);
    // wt_initialize(&dxv->m_op3);
    // wt_initialize(&dxv->m_op4);

    // attach oscillators to base class
    dxv->m_voice.m_osc1 = (oscillator *)&dxv->m_op1;
    dxv->m_voice.m_osc2 = (oscillator *)&dxv->m_op2;
    dxv->m_voice.m_osc3 = (oscillator *)&dxv->m_op3;
    dxv->m_voice.m_osc4 = (oscillator *)&dxv->m_op4;

    dxv->m_op1_feedback = 0.0;
    dxv->m_op2_feedback = 0.0;
    dxv->m_op3_feedback = 0.0;
    dxv->m_op4_feedback = 0.0;
}

void dxsynth_voice_init_global_parameters(dxsynth_voice *dx,
                                          global_synth_params *sp)
{
    voice_init_global_parameters(&dx->m_voice, sp);
    dx->m_voice.m_global_voice_params->op1_feedback = 1.0;
    dx->m_voice.m_global_voice_params->op2_feedback = 1.0;
    dx->m_voice.m_global_voice_params->op3_feedback = 1.0;
    dx->m_voice.m_global_voice_params->op4_feedback = 1.0;
    dx->m_voice.m_global_voice_params->lfo1_osc_mod_intensity = 1.0;
}

void dxsynth_voice_initialize_modmatrix(dxsynth_voice *dxv, modmatrix *matrix)
{
    voice_initialize_modmatrix(&dxv->m_voice, matrix);

    if (!get_matrix_core(matrix))
        return;

    matrixrow *row = NULL;

    // LFO1 -> DEST_OSC1_OUTPUT_AMP
    row = create_matrix_row(
        SOURCE_LFO1, DEST_OSC1_OUTPUT_AMP,
        &dxv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &dxv->m_voice.m_default_mod_range, TRANSFORM_BIPOLAR_TO_UNIPOLAR,
        false);
    add_matrix_row(matrix, row);

    // LFO1 -> DEST_OSC1_FO (VIBRATO)
    row = create_matrix_row(
        SOURCE_LFO1, DEST_OSC1_FO,
        &dxv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &dxv->m_voice.m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE,
        false);
    add_matrix_row(matrix, row);

    // LFO1 -> DEST_OSC2_OUTPUT_AMP
    row = create_matrix_row(
        SOURCE_LFO1, DEST_OSC2_OUTPUT_AMP,
        &dxv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &dxv->m_voice.m_default_mod_range, TRANSFORM_BIPOLAR_TO_UNIPOLAR,
        false);
    add_matrix_row(matrix, row);

    // LFO1 -> DEST_OSC2_FO (VIBRATO)
    row = create_matrix_row(
        SOURCE_LFO1, DEST_OSC2_FO,
        &dxv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &dxv->m_voice.m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE,
        false);
    add_matrix_row(matrix, row);

    // LFO1 -> DEST_OSC3_OUTPUT_AMP
    row = create_matrix_row(
        SOURCE_LFO1, DEST_OSC3_OUTPUT_AMP,
        &dxv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &dxv->m_voice.m_default_mod_range, TRANSFORM_BIPOLAR_TO_UNIPOLAR,
        false);
    add_matrix_row(matrix, row);

    // LFO1 -> DEST_OSC3_FO (VIBRATO)
    row = create_matrix_row(
        SOURCE_LFO1, DEST_OSC3_FO,
        &dxv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &dxv->m_voice.m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE,
        false);
    add_matrix_row(matrix, row);

    // LFO1 -> DEST_OSC4_OUTPUT_AMP
    row = create_matrix_row(
        SOURCE_LFO1, DEST_OSC4_OUTPUT_AMP,
        &dxv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &dxv->m_voice.m_default_mod_range, TRANSFORM_BIPOLAR_TO_UNIPOLAR,
        false);
    add_matrix_row(matrix, row);

    // LFO1 -> DEST_OSC4_FO (VIBRATO)
    row = create_matrix_row(
        SOURCE_LFO1, DEST_OSC4_FO,
        &dxv->m_voice.m_global_voice_params->lfo1_osc_mod_intensity,
        &dxv->m_voice.m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE,
        false);
    add_matrix_row(matrix, row);
}

void dxsynth_voice_set_lfo1_destination(dxsynth_voice *dx, unsigned int op,
                                        unsigned int dest)
{
    switch (op)
    {
    case (0):
    {
        if (dest == DX_LFO_DEST_AMP_MOD &&
            dx->m_op1.osc.m_mod_source_amp != DEST_OSC1_OUTPUT_AMP)
        {
            dx->m_op1.osc.m_mod_source_amp = DEST_OSC1_OUTPUT_AMP;
            dx->m_op1.osc.m_mod_source_fo = DEST_NONE;
        }
        else if (dest == DX_LFO_DEST_VIBRATO &&
                 dx->m_op1.osc.m_mod_source_fo != DEST_OSC1_FO)
        {
            dx->m_op1.osc.m_mod_source_fo = DEST_OSC1_FO;
            dx->m_op1.osc.m_mod_source_amp = DEST_NONE;
        }
        else if (dest == DX_LFO_DEST_NONE &&
                 (dx->m_op1.osc.m_mod_source_amp != DEST_NONE ||
                  dx->m_op1.osc.m_mod_source_fo != DEST_NONE))
        {
            dx->m_op1.osc.m_mod_source_amp = DEST_NONE;
            dx->m_op1.osc.m_mod_source_fo = DEST_NONE;
        }
        break;
    }
    case (1):
    {
        if (dest == DX_LFO_DEST_AMP_MOD &&
            dx->m_op2.osc.m_mod_source_amp != DEST_OSC2_OUTPUT_AMP)
        {
            dx->m_op2.osc.m_mod_source_amp = DEST_OSC2_OUTPUT_AMP;
            dx->m_op2.osc.m_mod_source_fo = DEST_NONE;
        }
        else if (dest == DX_LFO_DEST_VIBRATO &&
                 dx->m_op2.osc.m_mod_source_fo != DEST_OSC2_FO)
        {
            dx->m_op2.osc.m_mod_source_fo = DEST_OSC2_FO;
            dx->m_op2.osc.m_mod_source_amp = DEST_NONE;
        }
        else if (dest == DX_LFO_DEST_NONE &&
                 (dx->m_op2.osc.m_mod_source_amp != DEST_NONE ||
                  dx->m_op2.osc.m_mod_source_fo != DEST_NONE))
        {
            dx->m_op2.osc.m_mod_source_amp = DEST_NONE;
            dx->m_op2.osc.m_mod_source_fo = DEST_NONE;
        }
        break;
    }
    case (2):
    {
        if (dest == DX_LFO_DEST_AMP_MOD &&
            dx->m_op3.osc.m_mod_source_amp != DEST_OSC3_OUTPUT_AMP)
        {
            dx->m_op3.osc.m_mod_source_amp = DEST_OSC3_OUTPUT_AMP;
            dx->m_op3.osc.m_mod_source_fo = DEST_NONE;
        }
        else if (dest == DX_LFO_DEST_VIBRATO &&
                 dx->m_op3.osc.m_mod_source_fo != DEST_OSC3_FO)
        {
            dx->m_op3.osc.m_mod_source_fo = DEST_OSC3_FO;
            dx->m_op3.osc.m_mod_source_amp = DEST_NONE;
        }
        else if (dest == DX_LFO_DEST_NONE &&
                 (dx->m_op3.osc.m_mod_source_amp != DEST_NONE ||
                  dx->m_op3.osc.m_mod_source_fo != DEST_NONE))
        {
            dx->m_op3.osc.m_mod_source_amp = DEST_NONE;
            dx->m_op3.osc.m_mod_source_fo = DEST_NONE;
        }
        break;
    }
    case (3):
    {
        if (dest == DX_LFO_DEST_AMP_MOD &&
            dx->m_op4.osc.m_mod_source_amp != DEST_OSC4_OUTPUT_AMP)
        {
            dx->m_op4.osc.m_mod_source_amp = DEST_OSC4_OUTPUT_AMP;
            dx->m_op4.osc.m_mod_source_fo = DEST_NONE;
        }
        else if (dest == DX_LFO_DEST_VIBRATO &&
                 dx->m_op4.osc.m_mod_source_fo != DEST_OSC4_FO)
        {
            dx->m_op4.osc.m_mod_source_fo = DEST_OSC4_FO;
            dx->m_op4.osc.m_mod_source_amp = DEST_NONE;
        }
        else if (dest == DX_LFO_DEST_NONE &&
                 (dx->m_op4.osc.m_mod_source_amp != DEST_NONE ||
                  dx->m_op4.osc.m_mod_source_fo != DEST_NONE))
        {
            dx->m_op4.osc.m_mod_source_amp = DEST_NONE;
            dx->m_op4.osc.m_mod_source_fo = DEST_NONE;
        }
        break;
    }
    default:
        break;
    }
}

void dxsynth_voice_prepare_for_play(dxsynth_voice *dxv)
{
    voice_prepare_for_play(&dxv->m_voice);
    dxsynth_voice_reset(dxv);
}

void dxsynth_voice_update(dxsynth_voice *dxv)
{
    if (!dxv->m_voice.m_global_voice_params)
        return;

    voice_update(&dxv->m_voice);

    dxv->m_op1_feedback = dxv->m_voice.m_global_voice_params->op1_feedback;
    dxv->m_op2_feedback = dxv->m_voice.m_global_voice_params->op2_feedback;
    dxv->m_op3_feedback = dxv->m_voice.m_global_voice_params->op3_feedback;
    dxv->m_op4_feedback = dxv->m_voice.m_global_voice_params->op4_feedback;
}

void dxsynth_voice_reset(dxsynth_voice *dxv)
{
    voice_reset(&dxv->m_voice);
    dxv->m_voice.m_portamento_inc = 0.0;
}

inline bool dxsynth_voice_can_note_off(dxsynth_voice *dxv)
{
    bool ret = false;
    if (!dxv->m_voice.m_note_on)
        return ret;
    else
    {
        switch (dxv->m_voice.m_voice_mode + 1)
        {
        case 1:
        case 2:
        case 3:
        case 4:
        {
            if (eg_can_note_off(&dxv->m_voice.m_eg1))
                ret = true;
            break;
        }
        case 5:
        {
            if (eg_can_note_off(&dxv->m_voice.m_eg1) &&
                eg_can_note_off(&dxv->m_voice.m_eg2))
                ret = true;
            break;
        }
        case 6:
        case 7:
        {
            if (eg_can_note_off(&dxv->m_voice.m_eg1) &&
                eg_can_note_off(&dxv->m_voice.m_eg2) &&
                eg_can_note_off(&dxv->m_voice.m_eg3))
                ret = true;
            break;
        }
        case 8:
        {
            if (eg_can_note_off(&dxv->m_voice.m_eg1) &&
                eg_can_note_off(&dxv->m_voice.m_eg2) &&
                eg_can_note_off(&dxv->m_voice.m_eg3) &&
                eg_can_note_off(&dxv->m_voice.m_eg4))
                ret = true;
            break;
        }
        }
    }
    return ret;
}

bool dxsynth_voice_is_voice_done(dxsynth_voice *dxv)
{
    bool ret = false;
    switch (dxv->m_voice.m_voice_mode + 1)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    {
        if (eg_get_state(&dxv->m_voice.m_eg1) == OFFF)
            ret = true;
        break;
    }
    case 5:
    {
        if (eg_get_state(&dxv->m_voice.m_eg1) == OFFF &&
            eg_get_state(&dxv->m_voice.m_eg2) == OFFF)
            ret = true;
        break;
    }
    case 6:
    case 7:
    {
        if (eg_get_state(&dxv->m_voice.m_eg1) == OFFF &&
            eg_get_state(&dxv->m_voice.m_eg2) == OFFF &&
            eg_get_state(&dxv->m_voice.m_eg3) == OFFF)
            ret = true;
        break;
    }
    case 8:
    {
        if (eg_get_state(&dxv->m_voice.m_eg1) == OFFF &&
            eg_get_state(&dxv->m_voice.m_eg2) == OFFF &&
            eg_get_state(&dxv->m_voice.m_eg3) == OFFF &&
            eg_get_state(&dxv->m_voice.m_eg4) == OFFF)
            ret = true;
        break;
    }
    }
    return ret;
}

void dxsynth_voice_set_output_egs(dxsynth_voice *dxv)
{
    dxv->m_voice.m_eg1.m_output_eg = false;
    dxv->m_voice.m_eg2.m_output_eg = false;
    dxv->m_voice.m_eg3.m_output_eg = false;
    dxv->m_voice.m_eg4.m_output_eg = false;

    switch (dxv->m_voice.m_voice_mode + 1)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    {
        dxv->m_voice.m_eg1.m_output_eg = true;
        break;
    }
    case 5:
    {
        dxv->m_voice.m_eg1.m_output_eg = true;
        dxv->m_voice.m_eg2.m_output_eg = true;
        break;
    }
    case 6:
    case 7:
    {
        dxv->m_voice.m_eg1.m_output_eg = true;
        dxv->m_voice.m_eg2.m_output_eg = true;
        dxv->m_voice.m_eg3.m_output_eg = true;
        break;
    }
    case 8:
    {
        dxv->m_voice.m_eg1.m_output_eg = true;
        dxv->m_voice.m_eg2.m_output_eg = true;
        dxv->m_voice.m_eg3.m_output_eg = true;
        dxv->m_voice.m_eg4.m_output_eg = true;
        break;
    }
    }
}

inline bool dxsynth_voice_gennext(dxsynth_voice *dxv, double *left_output,
                                  double *right_output)
{
    if (!voice_gennext(&dxv->m_voice, left_output, right_output))
    {
        return false;
    }

    double out = 0.0;
    double out1, out2, out3, out4 = 0.0;
    double eg1, eg2, eg3, eg4 = 0.0;

    dxsynth_voice_set_output_egs(dxv);

    do_modulation_matrix(&dxv->m_voice.m_v_modmatrix, 0);

    eg_update(&dxv->m_voice.m_eg1);
    eg_update(&dxv->m_voice.m_eg2);
    eg_update(&dxv->m_voice.m_eg3);
    eg_update(&dxv->m_voice.m_eg4);

    eg1 = eg_do_envelope(&dxv->m_voice.m_eg1, NULL);
    eg2 = eg_do_envelope(&dxv->m_voice.m_eg2, NULL);
    eg3 = eg_do_envelope(&dxv->m_voice.m_eg3, NULL);
    eg4 = eg_do_envelope(&dxv->m_voice.m_eg4, NULL);

    osc_update((oscillator *)&dxv->m_voice.m_lfo1);
    lfo_do_oscillate((oscillator *)&dxv->m_voice.m_lfo1, NULL);

    ////// layer 1 //////////////////////////////
    do_modulation_matrix(&dxv->m_voice.m_v_modmatrix, 1);

    dxsynth_voice_update(dxv);
    osc_update((oscillator *)&dxv->m_op1);
    osc_update((oscillator *)&dxv->m_op2);
    osc_update((oscillator *)&dxv->m_op3);
    osc_update((oscillator *)&dxv->m_op4);

    dca_update(&dxv->m_voice.m_dca);

    switch (dxv->m_voice.m_voice_mode + 1)
    {
    case (1):
    {
        out4 = IMAX * eg4 * qb_do_oscillate((oscillator *)&dxv->m_op4, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op4,
                          out4 * dxv->m_op4_feedback);
        osc_update((oscillator *)&dxv->m_op4);

        osc_set_phase_mod((oscillator *)&dxv->m_op3, out4);
        osc_update((oscillator *)&dxv->m_op3);

        out3 = IMAX * eg3 * qb_do_oscillate((oscillator *)&dxv->m_op3, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op2, out3);
        osc_update((oscillator *)&dxv->m_op2);

        out2 = IMAX * eg2 * qb_do_oscillate((oscillator *)&dxv->m_op2, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op1, out2);
        osc_update((oscillator *)&dxv->m_op1);

        out1 = IMAX * eg1 * qb_do_oscillate((oscillator *)&dxv->m_op1, NULL);

        out = out1;

        break;
    }
    case (2):
    {
        out4 = IMAX * eg4 * qb_do_oscillate((oscillator *)&dxv->m_op4, NULL);
        osc_set_phase_mod((oscillator *)&dxv->m_op4,
                          out4 * dxv->m_op4_feedback);
        osc_update((oscillator *)&dxv->m_op4);

        out3 = IMAX * eg3 * qb_do_oscillate((oscillator *)&dxv->m_op3, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op2, out3 + out4);
        osc_update((oscillator *)&dxv->m_op2);

        out2 = IMAX * eg2 * qb_do_oscillate((oscillator *)&dxv->m_op2, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op1, out2);
        osc_update((oscillator *)&dxv->m_op1);

        out1 = IMAX * eg1 * qb_do_oscillate((oscillator *)&dxv->m_op1, NULL);

        out = out1;

        break;
    }
    case (3):
    {
        out4 = IMAX * eg4 * qb_do_oscillate((oscillator *)&dxv->m_op4, NULL);
        osc_set_phase_mod((oscillator *)&dxv->m_op4,
                          out4 * dxv->m_op4_feedback);
        osc_update((oscillator *)&dxv->m_op4);

        out3 = IMAX * eg3 * qb_do_oscillate((oscillator *)&dxv->m_op3, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op2, out3);
        osc_update((oscillator *)&dxv->m_op2);

        out2 = IMAX * eg2 * qb_do_oscillate((oscillator *)&dxv->m_op2, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op1, out2 + out4);
        osc_update((oscillator *)&dxv->m_op1);

        out1 = IMAX * eg1 * qb_do_oscillate((oscillator *)&dxv->m_op1, NULL);

        out = out1;

        break;
    }
    case (4):
    {
        out4 = IMAX * eg4 * qb_do_oscillate((oscillator *)&dxv->m_op4, NULL);
        osc_set_phase_mod((oscillator *)&dxv->m_op4,
                          out4 * dxv->m_op4_feedback);
        osc_update((oscillator *)&dxv->m_op4);

        osc_set_phase_mod((oscillator *)&dxv->m_op3, out4);
        osc_update((oscillator *)&dxv->m_op2);

        out3 = IMAX * eg3 * qb_do_oscillate((oscillator *)&dxv->m_op3, NULL);

        out2 = IMAX * eg2 * qb_do_oscillate((oscillator *)&dxv->m_op2, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op1, out2 + out3);
        osc_update((oscillator *)&dxv->m_op1);

        out1 = IMAX * eg1 * qb_do_oscillate((oscillator *)&dxv->m_op1, NULL);

        out = out1;

        break;
    }
    case (5):
    {
        out4 = IMAX * eg4 * qb_do_oscillate((oscillator *)&dxv->m_op4, NULL);
        osc_set_phase_mod((oscillator *)&dxv->m_op4,
                          out4 * dxv->m_op4_feedback);
        osc_update((oscillator *)&dxv->m_op4);

        out3 = IMAX * eg3 * qb_do_oscillate((oscillator *)&dxv->m_op3, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op2, out4);
        osc_update((oscillator *)&dxv->m_op2);

        out2 = IMAX * eg2 * qb_do_oscillate((oscillator *)&dxv->m_op2, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op1, out3);
        osc_update((oscillator *)&dxv->m_op1);

        out1 = IMAX * eg1 * qb_do_oscillate((oscillator *)&dxv->m_op1, NULL);

        out = out1;

        break;
    }
    case (6):
    {
        out4 = IMAX * eg4 * qb_do_oscillate((oscillator *)&dxv->m_op4, NULL);
        osc_set_phase_mod((oscillator *)&dxv->m_op4,
                          out4 * dxv->m_op4_feedback);
        osc_update((oscillator *)&dxv->m_op4);

        osc_set_phase_mod((oscillator *)&dxv->m_op3, out4);
        osc_update((oscillator *)&dxv->m_op2);

        out3 = IMAX * eg3 * qb_do_oscillate((oscillator *)&dxv->m_op3, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op2, out4);
        osc_update((oscillator *)&dxv->m_op2);

        out2 = IMAX * eg2 * qb_do_oscillate((oscillator *)&dxv->m_op2, NULL);

        osc_set_phase_mod((oscillator *)&dxv->m_op1, out4);
        osc_update((oscillator *)&dxv->m_op1);

        out1 = IMAX * eg1 * qb_do_oscillate((oscillator *)&dxv->m_op1, NULL);

        out = 0.33 * out1 + 0.33 * out2 + 0.33 * out3;

        break;
    }
    case (7):
    {
        out4 = IMAX * eg4 * qb_do_oscillate((oscillator *)&dxv->m_op4, NULL);
        osc_set_phase_mod((oscillator *)&dxv->m_op4,
                          out4 * dxv->m_op4_feedback);
        osc_update((oscillator *)&dxv->m_op4);

        osc_set_phase_mod((oscillator *)&dxv->m_op3, out4);
        osc_update((oscillator *)&dxv->m_op2);

        out3 = IMAX * eg3 * qb_do_oscillate((oscillator *)&dxv->m_op3, NULL);

        out2 = IMAX * eg2 * qb_do_oscillate((oscillator *)&dxv->m_op2, NULL);

        out1 = IMAX * eg1 * qb_do_oscillate((oscillator *)&dxv->m_op1, NULL);

        out = 0.33 * out1 + 0.33 * out2 + 0.33 * out3;

        break;
    }
    case (8):
    {
        out4 = IMAX * eg4 * qb_do_oscillate((oscillator *)&dxv->m_op4, NULL);
        osc_set_phase_mod((oscillator *)&dxv->m_op4,
                          out4 * dxv->m_op4_feedback);
        osc_update((oscillator *)&dxv->m_op4);

        out3 = IMAX * eg3 * qb_do_oscillate((oscillator *)&dxv->m_op3, NULL);
        out2 = IMAX * eg2 * qb_do_oscillate((oscillator *)&dxv->m_op2, NULL);
        out1 = IMAX * eg1 * qb_do_oscillate((oscillator *)&dxv->m_op1, NULL);

        out = 0.25 * out1 + 0.25 * out2 + 0.25 * out3 + 0.25 * out4;

        break;
    }
    default:
        break;
    }

    dca_gennext(&dxv->m_voice.m_dca, out, out, left_output, right_output);

    return true;
}

void dxsynth_voice_free_self(dxsynth_voice *dxv) { free(dxv); }
