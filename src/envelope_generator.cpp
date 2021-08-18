#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "defjams.h"
#include "envelope_generator.h"

unsigned int EnvelopeGenerator::GetState() { return m_state; }

bool EnvelopeGenerator::IsActive()
{
    if (m_state != RELEASE && m_state != OFFF && !m_release_pending)
        return true;
    return false;
}

bool EnvelopeGenerator::CanNoteOff()
{
    if (m_state != RELEASE && m_state != SHUTDOWN && m_state != OFFF &&
        !m_release_pending)
        return true;
    return false;
}

void EnvelopeGenerator::Reset()
{
    m_attack_time_scalar = 1.0;
    m_decay_time_scalar = 1.0;
    m_state = OFFF;
    m_release_pending = false;
    SetEgMode(m_eg_mode);

    CalculateReleaseTime();

    if (m_reset_to_zero)
    {
        m_envelope_output = 0.0;
    }
}

void EnvelopeGenerator::SetEgMode(unsigned int mode)
{
    m_eg_mode = mode;
    if (m_eg_mode == ANALOG)
    {
        m_attack_tco = exp(-1.5); // fast attack
        m_decay_tco = exp(-4.95);
        m_release_tco = m_decay_tco;
    }
    else
    {
        m_attack_tco = pow(10.0, -96.0 / 20.0);
        m_decay_tco = m_attack_tco;
        m_release_tco = m_decay_tco;
    }

    CalculateAttackTime();
    CalculateDecayTime();
    CalculateReleaseTime();
}

void EnvelopeGenerator::CalculateAttackTime()
{
    double d_samples =
        SAMPLE_RATE * ((m_attack_time_scalar * m_attack_time_msec) / 1000.0);
    m_attack_coeff = exp(-log((1.0 + m_attack_tco) / m_attack_tco) / d_samples);
    m_attack_offset = (1.0 + m_attack_tco) * (1.0 - m_attack_coeff);
}

void EnvelopeGenerator::CalculateDecayTime()
{
    double d_samples =
        SAMPLE_RATE * ((m_decay_time_scalar * m_decay_time_msec) / 1000.0);
    m_decay_coeff = exp(-log((1.0 + m_decay_tco) / m_decay_tco) / d_samples);
    m_decay_offset = (m_sustain_level - m_decay_tco) * (1.0 - m_decay_coeff);
}

void EnvelopeGenerator::CalculateReleaseTime()
{
    double d_samples = SAMPLE_RATE * (m_release_time_msec / 1000.0);
    m_release_coeff =
        exp(-log((1.0 + m_release_tco) / m_release_tco) / d_samples);
    m_release_offset = -m_release_tco * (1.0 - m_release_coeff);
}

void EnvelopeGenerator::SetAttackTimeMsec(double time)
{
    m_attack_time_msec = time;
    CalculateAttackTime();
}

void EnvelopeGenerator::SetDecayTimeMsec(double time)
{
    m_decay_time_msec = time;
    CalculateDecayTime();
}

void EnvelopeGenerator::SetReleaseTimeMsec(double time)
{
    m_release_time_msec = time;
    CalculateReleaseTime();
}

void EnvelopeGenerator::SetShutdownTimeMsec(double time)
{
    m_shutdown_time_msec = time;
}

void EnvelopeGenerator::SetSustainLevel(double level)
{
    m_sustain_level = level;
    CalculateDecayTime();
    if (m_state != RELEASE)
        CalculateReleaseTime();
}

void EnvelopeGenerator::SetSustainOverride(bool b)
{
    m_sustain_override = b;
    if (m_release_pending && !m_sustain_override)
    {
        m_release_pending = false;
        NoteOff();
    }
}

void EnvelopeGenerator::StartEg()
{
    if (m_legato_mode && m_state != OFFF && m_state != RELEASE)
    {
        return;
    }

    Reset();
    m_state = ATTACK;
}

void EnvelopeGenerator::Release()
{
    if (m_state == SUSTAIN)
        m_state = RELEASE;
}

void EnvelopeGenerator::StopEg() { m_state = OFFF; }

void EnvelopeGenerator::InitGlobalParameters(GlobalEgParams *params)
{
    global_eg_params = params;

    global_eg_params->attack_time_msec = m_attack_time_msec;
    global_eg_params->decay_time_msec = m_decay_time_msec;
    global_eg_params->release_time_msec = m_release_time_msec;
    global_eg_params->sustain_level = m_sustain_level;
    global_eg_params->shutdown_time_msec = m_shutdown_time_msec;
    global_eg_params->reset_to_zero = m_reset_to_zero;
    global_eg_params->legato_mode = m_legato_mode;
    global_eg_params->sustain_override = m_sustain_override;
}

void EnvelopeGenerator::Update()
{
    if (global_eg_params)
    {
        if (m_sustain_override != global_eg_params->sustain_override)
        {
            SetSustainOverride(global_eg_params->sustain_override);
        }

        if (m_attack_time_msec != global_eg_params->attack_time_msec)
        {
            SetAttackTimeMsec(global_eg_params->attack_time_msec);
        }

        if (m_decay_time_msec != global_eg_params->decay_time_msec)
            SetDecayTimeMsec(global_eg_params->attack_time_msec);

        if (m_release_time_msec != global_eg_params->release_time_msec)
            SetReleaseTimeMsec(global_eg_params->release_time_msec);

        if (m_sustain_level != global_eg_params->sustain_level)
            SetSustainLevel(global_eg_params->sustain_level);

        m_shutdown_time_msec = global_eg_params->shutdown_time_msec;
        m_reset_to_zero = global_eg_params->reset_to_zero;
        m_legato_mode = global_eg_params->legato_mode;
    }

    if (!modmatrix)
    {
        return;
    }

    // --- with mod matrix, when value is 0 there is NO modulation, so here
    if (m_mod_source_eg_attack_scaling != DEST_NONE &&
        m_attack_time_scalar == 1.0)
    {
        double scale = modmatrix->destinations[m_mod_source_eg_attack_scaling];
        if (m_attack_time_scalar != 1.0 - scale)
        {
            m_attack_time_scalar = 1.0 - scale;
            CalculateAttackTime();
        }
    }

    // --- for vel->attack and note#->decay scaling modulation
    //     NOTE: make sure this is only called ONCE during a new note event!
    if (m_mod_source_eg_decay_scaling != DEST_NONE &&
        m_decay_time_scalar == 1.0)
    {
        double scale = modmatrix->destinations[m_mod_source_eg_decay_scaling];
        if (m_decay_time_scalar != 1.0 - scale)
        {
            m_decay_time_scalar = 1.0 - scale;
            CalculateDecayTime();
        }
    }
}

double EnvelopeGenerator::DoEnvelope(double *p_biased_output)
{
    switch (m_state)
    {
    case OFFF:
    {
        // std::cout << "OFF!!\n";
        if (m_reset_to_zero)
            m_envelope_output = 0.0;
        break;
    }
    case ATTACK:
    {
        // std::cout << "ATTACK!!\n";
        m_envelope_output =
            m_attack_offset + m_envelope_output * m_attack_coeff;
        if (m_envelope_output >= 1.0 ||
            m_attack_time_scalar * m_attack_time_msec <= 0.0)
        {
            m_envelope_output = 1.0;
            m_state = DECAY;

            break;
        }
        break;
    }
    case DECAY:
    {
        // std::cout << "DECAY!!\n";
        m_envelope_output = m_decay_offset + m_envelope_output * m_decay_coeff;
        if (m_envelope_output <= m_sustain_level ||
            m_decay_time_scalar * m_decay_time_msec <= 0.0)
        {
            m_envelope_output = m_sustain_level;
            if (ramp_mode)
                NoteOff();
            else
                m_state = SUSTAIN;
            break;
        }
        break;
    }
    case SUSTAIN:
    {
        // std::cout << "SUSTAIN!!\n";
        m_envelope_output = m_sustain_level;
        break;
    }
    case RELEASE:
    {
        // std::cout << "REEEEEEElease!!\n";
        if (m_sustain_override)
        {
            m_envelope_output = m_sustain_level;
            break;
        }
        else
        {
            // std::cout << "ELSE:offset:" << m_release_offset
            //          << " env_out:" << m_envelope_output
            //          << " * m_release_coeff:" << m_release_coeff <<
            //          std::endl;
            m_envelope_output =
                m_release_offset + m_envelope_output * m_release_coeff;
        }

        if (m_envelope_output <= 0.0 || m_release_time_msec <= 0.0)
        {
            // std::cout << "*** WOW LESS THAN **!\n";
            m_envelope_output = 0.0;
            m_state = OFFF;
            break;
        }
        break;
    }
    case SHUTDOWN:
    {
        /// std::cout << "SHUTDOooooWN!!\n";
        if (m_reset_to_zero)
        {
            m_envelope_output += m_inc_shutdown;
            if (m_envelope_output <= 0)
            {
                m_envelope_output = 0.0;
                m_state = OFFF;
                break;
            }
        }
        else
        {
            m_state = OFFF;
        }
        break;
    }
    }

    if (modmatrix)
    {
        /// std::cout << "MODMATRIX!\n";
        modmatrix->sources[m_mod_dest_eg_output] = m_envelope_output;
        modmatrix->sources[m_mod_dest_eg_biased_output] =
            m_envelope_output - m_sustain_level;
    }

    if (p_biased_output)
        *p_biased_output = m_envelope_output - m_sustain_level;

    // std::cout << "ENV OUTPUT:" << m_envelope_output << std::endl;
    return m_envelope_output;
}

void EnvelopeGenerator::NoteOff()
{
    if (m_sustain_override)
    {
        m_release_pending = true;
        return;
    }

    if (m_envelope_output > 0)
    {
        m_state = RELEASE;
    }
    else
    {
        m_state = OFFF;
    }
}

void EnvelopeGenerator::Shutdown()
{
    // std::cout << "EG SHUTDOWN\n";
    if (m_legato_mode)
        return;
    m_inc_shutdown =
        -(1000.0 * m_envelope_output) / m_shutdown_time_msec / SAMPLE_RATE;
    m_state = SHUTDOWN;
    m_sustain_override = false;
    m_release_pending = false;
}
