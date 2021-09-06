#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "dca.h"
#include "defjams.h"
#include "utils.h"

#include <iostream>

extern const char *s_dest_enum_to_name[];

void DCA::SetMidiVelocity(unsigned int vel) { m_midi_velocity = vel; }

void DCA::SetPanControl(double pan) { m_pan_control = pan; }

void DCA::Reset()
{
    m_eg_mod = 1.0;
    m_amp_mod_db = 0.0;
}

void DCA::SetAmplitudeDb(double amp)
{
    m_amplitude_db = amp;
    m_amplitude_control = pow((double)10.0, amp / (double)20.0);
}

void DCA::SetAmpModDb(double mod) { m_amp_mod_db = mod; }

void DCA::SetEgMod(double mod) { m_eg_mod = mod; }

void DCA::SetPanMod(double mod) { m_pan_mod = mod; }

void DCA::Update()
{
    if (global_dca_params)
    {
        SetAmplitudeDb(global_dca_params->amplitude_db);
        m_pan_control = global_dca_params->pan_control;
    }

    if (modmatrix)
    {
        if (m_mod_source_eg != DEST_NONE)
        {
            m_eg_mod = modmatrix->destinations[m_mod_source_eg];
        }

        if (m_mod_source_amp_db != DEST_NONE)
            m_amp_mod_db = modmatrix->destinations[m_mod_source_amp_db];

        if (m_mod_source_velocity != DEST_NONE)
        {
            m_midi_velocity = modmatrix->destinations[m_mod_source_velocity];
            // std::cout << "GOT VELoCITY! " << m_mod_source_velocity << "
            // "
            //          << m_midi_velocity << std::endl;
        }

        if (m_mod_source_pan != DEST_NONE)
            m_pan_mod = modmatrix->destinations[m_mod_source_pan];
    }

    if (m_eg_mod >= 0)
        m_gain = m_eg_mod;
    else
        m_gain = m_eg_mod + 1.0;

    m_gain *= pow(10.0, m_amp_mod_db / (double)20.0);
    m_gain *= mma_midi_to_atten(m_midi_velocity);
}

void DCA::DoDCA(double left_input, double right_input, double *left_output,
                double *right_output)
{
    double pan_total = m_pan_control + m_pan_mod;

    pan_total = fmin(pan_total, 1.0);
    pan_total = fmax(pan_total, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(pan_total, &pan_left, &pan_right);

    *left_output = pan_left * m_amplitude_control * left_input * m_gain;
    *right_output = pan_right * m_amplitude_control * right_input * m_gain;
}

void DCA::InitGlobalParameters(GlobalDCAParams *params)
{
    global_dca_params = params;
    params->amplitude_db = m_amplitude_db;
    params->pan_control = m_pan_control;
}
