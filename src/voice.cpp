#include "voice.h"
#include "modmatrix.h"
#include "oscillator.h"
#include "utils.h"
#include <stdlib.h>

#include <iostream>

void Voice::InitializeModMatrix(ModulationMatrix *matrix)
{
    std::shared_ptr<ModMatrixRow> row;

    // VELOCITY -> DCA VEL
    row = CreateMatrixRow(SOURCE_VELOCITY, DEST_DCA_VELOCITY,
                          &m_default_mod_intensity, &m_default_mod_range,
                          TRANSFORM_NONE, true);
    matrix->AddMatrixRow(row);

    // PITCHBEND -> OSC
    row = CreateMatrixRow(SOURCE_PITCHBEND, DEST_ALL_OSC_FO,
                          &m_default_mod_intensity,
                          &m_global_voice_params->osc_fo_pitchbend_mod_range,
                          TRANSFORM_NONE, false);
    matrix->AddMatrixRow(row);

    // MIDI Volume CC07
    row = CreateMatrixRow(SOURCE_MIDI_VOLUME_CC07, DEST_DCA_AMP,
                          &m_default_mod_intensity,
                          &m_global_voice_params->amp_mod_range,
                          TRANSFORM_INVERT_MIDI_NORMALIZE, false);
    matrix->AddMatrixRow(row);

    // MIDI Pan CC10
    row = CreateMatrixRow(SOURCE_MIDI_PAN_CC10, DEST_DCA_PAN,
                          &m_default_mod_intensity, &m_default_mod_range,
                          TRANSFORM_MIDI_TO_PAN, false);
    matrix->AddMatrixRow(row);

    // // MIDI Sustain Pedal
    row = CreateMatrixRow(SOURCE_SUSTAIN_PEDAL, DEST_ALL_EG_SUSTAIN_OVERRIDE,
                          &m_default_mod_intensity, &m_default_mod_range,
                          TRANSFORM_MIDI_SWITCH, true);
    matrix->AddMatrixRow(row);

    // NOTE NUMBER -> FILTER Fc CONTROL
    row = CreateMatrixRow(
        SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK,
        &m_global_voice_params->filter_keytrack_intensity, &m_default_mod_range,
        TRANSFORM_NOTE_NUMBER_TO_FREQUENCY, false); /* DISABLED BY DEFAULT */
    matrix->AddMatrixRow(row);

    // VELOCITY -> EG ATTACK SOURCE_VELOCITY
    // 0 velocity -> scalar = 1, normal attack time
    // 128 velocity -> scalar = 0, fastest (0) attack time:
    row = CreateMatrixRow(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING,
                          &m_default_mod_intensity, &m_default_mod_range,
                          TRANSFORM_MIDI_NORMALIZE, false);
    matrix->AddMatrixRow(row);

    // NOTE NUMBER -> EG DECAY SCALING
    row = CreateMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING,
                          &m_default_mod_intensity, &m_default_mod_range,
                          TRANSFORM_MIDI_NORMALIZE, false);
    matrix->AddMatrixRow(row);
}

void Voice::PrepareForPlay()
{
    modmatrix.ClearSources();
    modmatrix.sources[SOURCE_MIDI_VOLUME_CC07] = 127;
    modmatrix.sources[SOURCE_MIDI_PAN_CC10] = 64;

    m_lfo1.modmatrix = &modmatrix;
    m_lfo1.m_mod_dest_output1 = SOURCE_LFO1;
    m_lfo1.m_mod_dest_output2 = SOURCE_LFO1Q;

    m_lfo2.modmatrix = &modmatrix;
    m_lfo2.m_mod_dest_output1 = SOURCE_LFO2;
    m_lfo2.m_mod_dest_output2 = SOURCE_LFO2Q;

    m_eg1.modmatrix = &modmatrix;
    m_eg1.m_mod_dest_eg_output = SOURCE_EG1;
    m_eg1.m_mod_dest_eg_biased_output = SOURCE_BIASED_EG1;
    m_eg1.m_mod_source_eg_attack_scaling = DEST_EG1_ATTACK_SCALING;
    m_eg1.m_mod_source_eg_decay_scaling = DEST_EG1_DECAY_SCALING;
    m_eg1.m_mod_source_sustain_override = DEST_EG1_SUSTAIN_OVERRIDE;

    m_eg2.modmatrix = &modmatrix;
    m_eg2.m_mod_dest_eg_output = SOURCE_EG2;
    m_eg2.m_mod_dest_eg_biased_output = SOURCE_BIASED_EG2;
    m_eg2.m_mod_source_eg_attack_scaling = DEST_EG2_ATTACK_SCALING;
    m_eg2.m_mod_source_eg_decay_scaling = DEST_EG2_DECAY_SCALING;
    m_eg2.m_mod_source_sustain_override = DEST_EG2_SUSTAIN_OVERRIDE;

    m_eg3.modmatrix = &modmatrix;
    m_eg3.m_mod_dest_eg_output = SOURCE_EG3;
    m_eg3.m_mod_dest_eg_biased_output = SOURCE_BIASED_EG3;
    m_eg3.m_mod_source_eg_attack_scaling = DEST_EG3_ATTACK_SCALING;
    m_eg3.m_mod_source_eg_decay_scaling = DEST_EG3_DECAY_SCALING;
    m_eg3.m_mod_source_sustain_override = DEST_EG3_SUSTAIN_OVERRIDE;

    m_eg4.modmatrix = &modmatrix;
    m_eg4.m_mod_dest_eg_output = SOURCE_EG4;
    m_eg4.m_mod_dest_eg_biased_output = SOURCE_BIASED_EG4;
    m_eg4.m_mod_source_eg_attack_scaling = DEST_EG4_ATTACK_SCALING;
    m_eg4.m_mod_source_eg_decay_scaling = DEST_EG4_DECAY_SCALING;
    // m_eg4.m_mod_source_sustain_override = DEST_EG4_SUSTAIN_OVERRIDE;

    m_dca.modmatrix = &modmatrix;
    // m_dca.m_mod_source_eg = DEST_DCA_EG; // enabled in minisynth
    m_dca.m_mod_source_eg = DEST_NONE;
    m_dca.m_mod_source_amp_db = DEST_DCA_AMP;
    m_dca.m_mod_source_velocity = DEST_DCA_VELOCITY;
    m_dca.m_mod_source_pan = DEST_DCA_PAN;

    if (m_osc1)
    {
        m_osc1->modmatrix = &modmatrix;
        m_osc1->m_mod_source_fo = DEST_OSC1_FO;
        m_osc1->m_mod_source_pulse_width = DEST_OSC1_PULSEWIDTH;
        m_osc1->m_mod_source_amp = DEST_OSC1_OUTPUT_AMP;
    }

    if (m_osc2)
    {
        m_osc2->modmatrix = &modmatrix;
        m_osc2->m_mod_source_fo = DEST_OSC2_FO;
        m_osc2->m_mod_source_pulse_width = DEST_OSC2_PULSEWIDTH;
        m_osc2->m_mod_source_amp = DEST_OSC2_OUTPUT_AMP;
    }

    if (m_osc3)
    {
        m_osc3->modmatrix = &modmatrix;
        m_osc3->m_mod_source_fo = DEST_OSC3_FO;
        m_osc3->m_mod_source_pulse_width = DEST_OSC3_PULSEWIDTH;
        m_osc3->m_mod_source_amp = DEST_OSC3_OUTPUT_AMP;
    }

    if (m_osc4)
    {
        m_osc4->modmatrix = &modmatrix;
        m_osc4->m_mod_source_fo = DEST_OSC4_FO;
        m_osc4->m_mod_source_pulse_width = DEST_OSC4_PULSEWIDTH;
        m_osc4->m_mod_source_amp = DEST_OSC4_OUTPUT_AMP;
    }

    if (m_filter1)
    {
        m_filter1->modmatrix = &modmatrix;
        m_filter1->m_mod_source_fc = DEST_FILTER1_FC;
        m_filter1->m_mod_source_fc_control = DEST_ALL_FILTER_KEYTRACK;
    }

    if (m_filter2)
    {
        m_filter2->modmatrix = &modmatrix;
        m_filter2->m_mod_source_fc = DEST_FILTER2_FC;
        m_filter2->m_mod_source_fc_control = DEST_ALL_FILTER_KEYTRACK;
    }
}

void Voice::InitGlobalParameters(GlobalSynthParams *sp)
{
    m_global_synth_params = sp;

    m_global_voice_params = &sp->voice_params;
    m_global_voice_params->voice_mode = m_voice_mode;
    m_global_voice_params->hs_ratio = m_hs_ratio;
    m_global_voice_params->portamento_time_msec = m_portamento_time_msec;

    m_global_voice_params->osc_fo_pitchbend_mod_range = OSC_PITCHBEND_MOD_RANGE;
    m_global_voice_params->osc_fo_mod_range = OSC_FO_MOD_RANGE;
    m_global_voice_params->osc_hard_sync_mod_range = OSC_HARD_SYNC_RATIO_RANGE;
    m_global_voice_params->filter_mod_range = FILTER_FC_MOD_RANGE;
    m_global_voice_params->filter_q_mod_range = FILTER_Q_MOD_RANGE;
    m_global_voice_params->amp_mod_range = AMP_MOD_RANGE;

    // --- Intensity variables
    m_global_voice_params->filter_keytrack_intensity = 1.0;

    // --- init sub-components
    if (m_osc1)
        m_osc1->InitGlobalParameters(&m_global_synth_params->osc1_params);
    if (m_osc2)
        m_osc2->InitGlobalParameters(&m_global_synth_params->osc2_params);
    if (m_osc3)
        m_osc3->InitGlobalParameters(&m_global_synth_params->osc3_params);
    if (m_osc4)
        m_osc4->InitGlobalParameters(&m_global_synth_params->osc4_params);

    if (m_filter1)
        m_filter1->InitGlobalParameters(&m_global_synth_params->filter1_params);
    if (m_filter2)
        m_filter2->InitGlobalParameters(&m_global_synth_params->filter2_params);

    m_eg1.InitGlobalParameters(&m_global_synth_params->eg1_params);
    m_eg2.InitGlobalParameters(&m_global_synth_params->eg2_params);
    m_eg3.InitGlobalParameters(&m_global_synth_params->eg3_params);
    m_eg4.InitGlobalParameters(&m_global_synth_params->eg4_params);

    m_lfo1.InitGlobalParameters(&m_global_synth_params->lfo1_params);
    m_lfo2.InitGlobalParameters(&m_global_synth_params->lfo2_params);

    m_dca.InitGlobalParameters(&m_global_synth_params->dca_params);
}

void Voice::SetModMatrixCore(std::vector<std::shared_ptr<ModMatrixRow>> &core)
{
    modmatrix.SetModMatrixCore(core);
}

bool Voice::IsActiveVoice()
{
    if (m_note_on && m_eg1.IsActive())
        return true;
    return false;
}

bool Voice::CanNoteOff()
{
    if (m_note_on && m_eg1.CanNoteOff())
        return true;
    return false;
}

bool Voice::IsVoiceDone()
{
    if (m_eg1.GetState() == OFFF)
        return true;
    return false;
}

bool Voice::InLegatoMode() { return m_eg1.m_legato_mode; }

void Voice::Update()
{
    m_voice_mode = m_global_voice_params->voice_mode;
    hard_sync = m_global_voice_params->hard_sync;
    if (m_portamento_time_msec != m_global_voice_params->portamento_time_msec)
    {
        m_portamento_time_msec = m_global_voice_params->portamento_time_msec;
        if (m_portamento_time_msec == 0.0)
            m_portamento_inc = 0.0;
        else
            m_portamento_inc = 1000.0 / m_portamento_time_msec / SAMPLE_RATE;
    }

    m_hs_ratio = m_global_voice_params->hs_ratio;
}

void Voice::Reset()
{
    if (m_osc1)
        m_osc1->Reset();
    if (m_osc2)
        m_osc2->Reset();
    if (m_osc3)
        m_osc3->Reset();
    if (m_osc4)
        m_osc4->Reset();

    if (m_filter1)
        m_filter1->Reset();
    if (m_filter2)
        m_filter2->Reset();

    m_eg1.Reset();
    m_eg2.Reset();
    m_eg3.Reset();
    m_eg4.Reset();

    m_lfo1.Reset();
    m_lfo2.Reset();

    m_dca.Reset();
}

void Voice::NoteOn(int midi_note, int midi_velocity, double frequency,
                   double last_note_frequency)
{
    m_osc_pitch = frequency;

    if (!m_note_on && !m_note_pending)
    {
        m_midi_note_number = midi_note;
        m_midi_velocity = midi_velocity;
        modmatrix.sources[SOURCE_VELOCITY] = (double)m_midi_velocity;
        modmatrix.sources[SOURCE_MIDI_NOTE_NUM] = (double)m_midi_note_number;
        if (m_portamento_inc > 0.0 && last_note_frequency >= 0)
        {
            m_modulo_portamento = 0.0;
            m_portamento_semitones =
                semitones_between_frequencies(last_note_frequency, frequency);
            m_portamento_start = last_note_frequency;

            if (m_osc1)
                m_osc1->m_osc_fo = m_portamento_start;
            if (m_osc2)
                m_osc2->m_osc_fo = m_portamento_start;
            if (m_osc3)
                m_osc3->m_osc_fo = m_portamento_start;
            if (m_osc4)
                m_osc4->m_osc_fo = m_portamento_start;
        }
        else
        {
            if (m_osc1)
                m_osc1->m_osc_fo = m_osc_pitch;
            if (m_osc2)
                m_osc2->m_osc_fo = m_osc_pitch;
            if (m_osc3)
                m_osc3->m_osc_fo = m_osc_pitch;
            if (m_osc4)
                m_osc4->m_osc_fo = m_osc_pitch;
        }

        // needed for sample based oscillators
        if (m_osc1)
            m_osc1->m_midi_note_number = m_midi_note_number;
        if (m_osc2)
            m_osc2->m_midi_note_number = m_midi_note_number;
        if (m_osc3)
            m_osc3->m_midi_note_number = m_midi_note_number;
        if (m_osc4)
            m_osc4->m_midi_note_number = m_midi_note_number;

        if (m_osc1)
            m_osc1->StartOscillator();
        if (m_osc2)
            m_osc2->StartOscillator();
        if (m_osc3)
            m_osc3->StartOscillator();
        if (m_osc4)
            m_osc4->StartOscillator();

        m_eg1.StartEg();
        m_eg2.StartEg();
        m_eg3.StartEg();
        m_eg4.StartEg();

        m_lfo1.StartOscillator();
        m_lfo2.StartOscillator();

        m_note_on = true;
        m_timestamp = 0;

        return; // ESCAPE HATCH, ALL DONE
    }

    // if we get here, we need to steal a note
    //
    // this checks if we're already stealing this note - then no-op, just rturn
    if (m_note_pending && m_midi_note_number_pending == midi_note)
        return;

    m_midi_note_number_pending = midi_note;
    m_midi_velocity_pending = midi_velocity;
    m_osc_pitch_pending = frequency;

    m_note_pending = true;

    if (m_portamento_inc > 0.0 && last_note_frequency > 0)
    {
        if (m_modulo_portamento > 0.0)
        {
            double portamento_pitch_mult = pitch_shift_multiplier(
                m_modulo_portamento * m_portamento_semitones);
            m_portamento_start = m_portamento_start * portamento_pitch_mult;
        }
        else
        {
            m_portamento_start = last_note_frequency;
        }
        m_modulo_portamento = 0.0;
        m_portamento_semitones =
            semitones_between_frequencies(m_portamento_start, frequency);
    }

    m_eg1.Shutdown();
    m_eg2.Shutdown();
    m_eg3.Shutdown();
    m_eg4.Shutdown();
}

void Voice::NoteOff(int midi_note)
{
    if (m_note_on && CanNoteOff())
    {
        if (m_note_pending && (midi_note == m_midi_note_number_pending))
        {
            m_note_pending = false;
            return;
        }
        if (midi_note != m_midi_note_number && midi_note != -1)
        {
            return;
        }

        m_eg1.NoteOff();
        m_eg2.NoteOff();
        m_eg3.NoteOff();
        m_eg4.NoteOff();
    }
}

bool Voice::DoVoice(double *left_output, double *right_output)
{
    *left_output = 0.0;
    *right_output = 0.0;

    if (!m_note_on)
        return false;

    if (IsVoiceDone() || m_note_pending)
    {
        if (IsVoiceDone() && !m_note_pending)
        {
            if (m_osc1)
                m_osc1->StopOscillator();
            if (m_osc2)
                m_osc2->StopOscillator();
            if (m_osc3)
                m_osc3->StopOscillator();
            if (m_osc4)
                m_osc4->StopOscillator();

            if (m_osc1)
                m_osc1->Reset();
            if (m_osc2)
                m_osc2->Reset();
            if (m_osc3)
                m_osc3->Reset();
            if (m_osc4)
                m_osc4->Reset();

            m_lfo1.StopOscillator();
            m_lfo2.StopOscillator();

            m_eg1.Reset();
            m_eg2.Reset();
            m_eg3.Reset();
            m_eg4.Reset();

            m_note_on = false;

            return false;
        }
        else if (m_note_pending && (IsVoiceDone() || InLegatoMode()))
        {
            m_midi_note_number = m_midi_note_number_pending;
            m_midi_velocity = m_midi_velocity_pending;
            m_osc_pitch = m_osc_pitch_pending;
            m_timestamp = 0;

            modmatrix.sources[SOURCE_MIDI_NOTE_NUM] =
                (double)m_midi_note_number;

            if (!InLegatoMode())
                modmatrix.sources[SOURCE_VELOCITY] = (double)m_midi_velocity;

            double pitch =
                m_portamento_inc > 0.0 ? m_portamento_start : m_osc_pitch;

            if (m_osc1)
                m_osc1->m_osc_fo = pitch;
            if (m_osc2)
                m_osc2->m_osc_fo = pitch;
            if (m_osc3)
                m_osc3->m_osc_fo = pitch;
            if (m_osc4)
                m_osc4->m_osc_fo = pitch;

            if (m_osc1)
                m_osc1->m_midi_note_number = m_midi_note_number;
            if (m_osc2)
                m_osc2->m_midi_note_number = m_midi_note_number;
            if (m_osc3)
                m_osc3->m_midi_note_number = m_midi_note_number;
            if (m_osc4)
                m_osc4->m_midi_note_number = m_midi_note_number;

            if (!m_legato_mode)
            {
                if (m_osc1)
                    m_osc1->Reset();
                if (m_osc2)
                    m_osc2->Reset();
                if (m_osc3)
                    m_osc3->Reset();
                if (m_osc4)
                    m_osc4->Reset();
            }

            m_eg1.StartEg();
            m_eg2.StartEg();
            m_eg3.StartEg();
            m_eg4.StartEg();

            m_lfo1.StartOscillator();
            m_lfo2.StartOscillator();

            m_note_pending = false;
        }
    }

    // portamento block
    if (m_portamento_inc > 0.0 && m_osc1->m_osc_fo != m_osc_pitch)
    {
        if (m_modulo_portamento >= 1.0)
        {
            m_modulo_portamento = 0.0;

            if (m_osc1)
                m_osc1->m_osc_fo = m_osc_pitch;
            if (m_osc2)
                m_osc2->m_osc_fo = m_osc_pitch;
            if (m_osc3)
                m_osc3->m_osc_fo = m_osc_pitch;
            if (m_osc4)
                m_osc4->m_osc_fo = m_osc_pitch;
        }
        else
        {
            double portamento_pitch =
                m_portamento_start *
                pitch_shift_multiplier(m_modulo_portamento *
                                       m_portamento_semitones);
            if (m_osc1)
                m_osc1->m_osc_fo = portamento_pitch;
            if (m_osc2)
                m_osc2->m_osc_fo = portamento_pitch;
            if (m_osc3)
                m_osc3->m_osc_fo = portamento_pitch;
            if (m_osc4)
                m_osc4->m_osc_fo = portamento_pitch;

            m_modulo_portamento += m_portamento_inc;
        }
    }
    return true;
}

void Voice::SetSustainOverride(bool b)
{
    m_eg1.SetSustainOverride(b);
    m_eg2.SetSustainOverride(b);
    m_eg3.SetSustainOverride(b);
    m_eg4.SetSustainOverride(b);
}
