#include "minisynth_voice.h"
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

MiniSynthVoice::MiniSynthVoice()
{
    // attach oscillators to base class
    m_osc1 = &m_op1;
    m_osc2 = &m_op2;
    m_osc3 = &m_op3;
    m_osc4 = &m_op4;

    // attach to base class
    m_filter1 = &m_filter;
    m_filter2 = NULL;

    m_filter.m_aux_control = 0.0;

    // voice mode 0
    m_osc1->m_waveform = SAW1;
    m_osc2->m_waveform = SAW1;
    m_osc3->m_waveform = SAW1;
    m_osc4->m_waveform = NOISE;

    m_eg1.SetEgMode(ANALOG);
    m_eg1.m_output_eg = true;

    m_dca.m_mod_source_eg = DEST_DCA_EG;
}

void MiniSynthVoice::InitializeModMatrix(ModulationMatrix *matrix)
{
    Voice::InitializeModMatrix(matrix);

    std::shared_ptr<ModMatrixRow> row = NULL;

    // LFO1 -> ALL OSC1 FO
    row = CreateMatrixRow(SOURCE_LFO1, DEST_ALL_OSC_FO,
                          &m_global_voice_params->lfo1_osc_mod_intensity,
                          &m_global_voice_params->osc_fo_mod_range,
                          TRANSFORM_NONE, true);
    matrix->AddMatrixRow(row);

    // LFO1 -> FILTER1 FC
    row = CreateMatrixRow(SOURCE_LFO1, DEST_ALL_FILTER_FC,
                          &m_global_voice_params->lfo1_filter1_mod_intensity,
                          &m_global_voice_params->filter_mod_range,
                          TRANSFORM_NONE, true);
    matrix->AddMatrixRow(row);

    // LFO1 -> FILTER1 Q
    row = CreateMatrixRow(SOURCE_LFO1, DEST_ALL_FILTER_Q,
                          &m_global_voice_params->lfo1_filter1_q_mod_intensity,
                          &m_global_voice_params->filter_q_mod_range,
                          TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // LFO1 (-1 -> +1) -> DCA Amp Mod (0->1)
    row = CreateMatrixRow(SOURCE_LFO1, DEST_DCA_AMP,
                          &m_global_voice_params->lfo1_dca_amp_mod_intensity,
                          &m_global_voice_params->amp_mod_range,
                          TRANSFORM_BIPOLAR_TO_UNIPOLAR, true);
    matrix->AddMatrixRow(row);

    // LFO1 (-1 -> +1) -> DCA Pan Mod (0->1)
    row = CreateMatrixRow(SOURCE_LFO1, DEST_DCA_PAN,
                          &m_global_voice_params->lfo1_dca_pan_mod_intensity,
                          &m_default_mod_range, TRANSFORM_NONE, true);
    matrix->AddMatrixRow(row);

    // LFO1 -> PULSE WIDTH
    row = CreateMatrixRow(SOURCE_LFO1, DEST_ALL_OSC_PULSEWIDTH,
                          &m_default_mod_intensity, &m_default_mod_range,
                          TRANSFORM_NONE, true);
    matrix->AddMatrixRow(row);

    // LFO2 -> ALL OSC FO
    row = CreateMatrixRow(SOURCE_LFO2, DEST_ALL_OSC_FO,
                          &m_global_voice_params->lfo2_osc_mod_intensity,
                          &m_global_voice_params->osc_fo_mod_range,
                          TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // LFO2 -> FILTER1 FC
    row = CreateMatrixRow(SOURCE_LFO2, DEST_ALL_FILTER_FC,
                          &m_global_voice_params->lfo2_filter1_mod_intensity,
                          &m_global_voice_params->filter_mod_range,
                          TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // LFO2 -> FILTER1 Q
    row = CreateMatrixRow(SOURCE_LFO2, DEST_ALL_FILTER_Q,
                          &m_global_voice_params->lfo2_filter1_q_mod_intensity,
                          &m_global_voice_params->filter_q_mod_range,
                          TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // LFO2 (-1 -> +1) -> DCA Amp Mod (0->1)
    row = CreateMatrixRow(SOURCE_LFO2, DEST_DCA_AMP,
                          &m_global_voice_params->lfo2_dca_amp_mod_intensity,
                          &m_global_voice_params->amp_mod_range,
                          TRANSFORM_BIPOLAR_TO_UNIPOLAR, false);
    matrix->AddMatrixRow(row);

    row = CreateMatrixRow(SOURCE_LFO2, DEST_DCA_PAN,
                          &m_global_voice_params->lfo2_dca_pan_mod_intensity,
                          &m_default_mod_range, TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // LFO2 -> PULSE WIDTH
    row = CreateMatrixRow(SOURCE_LFO2, DEST_ALL_OSC_PULSEWIDTH,
                          &m_default_mod_intensity, &m_default_mod_range,
                          TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // EG1 -> FILTER1 FC
    row = CreateMatrixRow(SOURCE_BIASED_EG1, DEST_ALL_FILTER_FC,
                          &m_global_voice_params->eg1_filter1_mod_intensity,
                          &m_global_voice_params->filter_mod_range,
                          TRANSFORM_NONE, true);
    matrix->AddMatrixRow(row);

    // EG1 -> DCA EG
    row = CreateMatrixRow(SOURCE_EG1, DEST_DCA_EG,
                          &m_global_voice_params->eg1_dca_amp_mod_intensity,
                          &m_default_mod_range, TRANSFORM_NONE, true);
    matrix->AddMatrixRow(row);

    // EG1 -> ALL OSC
    row = CreateMatrixRow(SOURCE_BIASED_EG1, DEST_ALL_OSC_FO,
                          &m_global_voice_params->eg1_osc_mod_intensity,
                          &m_global_voice_params->osc_fo_mod_range,
                          TRANSFORM_NONE, true);
    matrix->AddMatrixRow(row);

    // EG2 -> FILTER1 FC
    row = CreateMatrixRow(SOURCE_BIASED_EG2, DEST_ALL_FILTER_FC,
                          &m_global_voice_params->eg2_filter1_mod_intensity,
                          &m_global_voice_params->filter_mod_range,
                          TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // EG2 -> DCA EG
    row = CreateMatrixRow(SOURCE_EG2, DEST_DCA_EG,
                          &m_global_voice_params->eg2_dca_amp_mod_intensity,
                          &m_default_mod_range, TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // EG2 -> ALL OSC
    row = CreateMatrixRow(SOURCE_BIASED_EG2, DEST_ALL_OSC_FO,
                          &m_global_voice_params->eg2_osc_mod_intensity,
                          &m_global_voice_params->osc_fo_mod_range,
                          TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);
}

void MiniSynthVoice::InitGlobalParameters(GlobalSynthParams *sp)
{
    Voice::InitGlobalParameters(sp);

    m_global_voice_params->lfo1_osc_mod_intensity = 1.0;
    m_global_voice_params->lfo1_filter1_mod_intensity = 1.0;
    m_global_voice_params->lfo1_filter2_mod_intensity = 1.0;
    m_global_voice_params->lfo1_dca_pan_mod_intensity = 1.0;
    m_global_voice_params->lfo1_dca_amp_mod_intensity = 1.0;

    m_global_voice_params->lfo2_osc_mod_intensity = 1.0;
    m_global_voice_params->lfo2_filter1_mod_intensity = 1.0;
    m_global_voice_params->lfo2_filter2_mod_intensity = 1.0;
    m_global_voice_params->lfo2_dca_pan_mod_intensity = 1.0;
    m_global_voice_params->lfo2_dca_amp_mod_intensity = 1.0;

    m_global_voice_params->eg1_osc_mod_intensity = 1.0;
    m_global_voice_params->eg1_filter1_mod_intensity = 1.0;
    m_global_voice_params->eg1_filter2_mod_intensity = 1.0;
    m_global_voice_params->eg1_dca_amp_mod_intensity = 1.0;

    m_global_voice_params->eg2_osc_mod_intensity = 1.0;
    m_global_voice_params->eg2_filter1_mod_intensity = 1.0;
    m_global_voice_params->eg2_filter2_mod_intensity = 1.0;
    m_global_voice_params->eg2_dca_amp_mod_intensity = 1.0;
}

void MiniSynthVoice::PrepareForPlay()
{
    Voice::PrepareForPlay();
    Reset();
}

void MiniSynthVoice::Update()
{
    if (!m_global_voice_params)
        return;

    // save this as base class will override
    unsigned int current_voice_mode = m_voice_mode;

    Voice::Update();

    if (m_voice_mode != current_voice_mode)
    {
        m_voice_mode = m_global_voice_params->voice_mode;
        m_osc3->m_octave = -1.0;
        m_global_synth_params->osc4_params.waveform = NOISE;

        switch (m_voice_mode)
        {
        case Saw3:
            m_global_synth_params->osc1_params.waveform = SAW1;
            m_global_synth_params->osc2_params.waveform = SAW1;
            m_global_synth_params->osc3_params.waveform = SAW1;
            break;
        case Sqr3:
            m_global_synth_params->osc1_params.waveform = SQUARE;
            m_global_synth_params->osc2_params.waveform = SQUARE;
            m_global_synth_params->osc3_params.waveform = SQUARE;
            break;
        case Saw2Sqr:
            m_global_synth_params->osc1_params.waveform = SAW1;
            m_global_synth_params->osc2_params.waveform = SQUARE;
            m_global_synth_params->osc3_params.waveform = SAW1;
            break;
        case Tri2Saw:
            m_global_synth_params->osc1_params.waveform = TRI;
            m_global_synth_params->osc2_params.waveform = SAW1;
            m_global_synth_params->osc3_params.waveform = TRI;
            break;
        case Tri2Sqr:
            m_global_synth_params->osc1_params.waveform = TRI;
            m_global_synth_params->osc2_params.waveform = SQUARE;
            m_global_synth_params->osc3_params.waveform = TRI;
            break;
        case Sin2Sqr:
            m_global_synth_params->osc1_params.waveform = SINE;
            m_global_synth_params->osc2_params.waveform = SQUARE;
            m_global_synth_params->osc3_params.waveform = SINE;
            break;
        default:
            m_global_synth_params->osc1_params.waveform = SAW1;
            m_global_synth_params->osc2_params.waveform = SAW1;
            m_global_synth_params->osc3_params.waveform = SAW1;
            break;
        }
        m_osc1->Reset();
        m_osc2->Reset();
        m_osc3->Reset();
        m_osc4->Reset();
    }
}

void MiniSynthVoice::Reset()
{
    Voice::Reset();
    m_portamento_inc = 0.0;
    m_osc1->m_waveform = SAW1;
    m_osc1->m_waveform = SAW1;
    m_osc1->m_waveform = SAW1;
    m_osc1->m_waveform = NOISE;
}

bool MiniSynthVoice::DoVoice(double *left_output, double *right_output)
{
    if (!Voice::DoVoice(left_output, right_output))
    {
        return false;
    }

    //// layer 0 //////////////////////////////
    modmatrix.DoModMatrix(0);

    ////// update layer 1 modulators
    m_eg1.Update();
    m_eg2.Update();
    m_lfo1.Update();
    m_lfo2.Update();

    ////// gen next val layer 1 mods
    m_eg1.DoEnvelope(NULL);
    m_eg2.DoEnvelope(NULL);
    m_lfo1.DoOscillate(NULL);
    m_lfo2.DoOscillate(NULL);

    ////// layer 1 //////////////////////////////
    modmatrix.DoModMatrix(1);

    Update();
    m_dca.Update();
    m_filter.Update();

    m_osc1->Update();
    m_osc2->Update();
    m_osc3->Update();
    m_osc4->Update();

    double osc_mix = 0.;
    if (hard_sync)
    {
        m_osc1->DoOscillate(NULL);
        if (m_osc1->just_wrapped)
            m_osc2->StartOscillator();
        osc_mix = 0.666 * m_osc2->DoOscillate(NULL) +
                  0.333 * m_osc3->DoOscillate(NULL) + m_osc4->DoOscillate(NULL);
    }
    else
    {
        osc_mix = 0.333 * m_osc1->DoOscillate(NULL) +
                  0.333 * m_osc2->DoOscillate(NULL) +
                  0.333 * m_osc3->DoOscillate(NULL) + m_osc4->DoOscillate(NULL);
    }

    double filter_out = m_filter.DoFilter(osc_mix);

    m_dca.DoDCA(filter_out, filter_out, left_output, right_output);

    return true;
}

void MiniSynthVoice::SetFilterMod(double mod) { m_filter.SetFcMod(mod); }
