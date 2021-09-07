#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "defjams.h"
#include "lfo.h"
#include "oscillator.h"
#include "utils.h"

LFO::LFO() { m_lfo_mode = LFOSYNC; }

double LFO::DoOscillate(double *quad_phase_output)
{
    if (!m_note_on)
    {
        if (quad_phase_output)
        {
            *quad_phase_output = 0.0;
        }
        return 0.0;
    }

    // output
    double out = 0.0;
    double qp_out = 0.0;

    // always first
    bool wrap = CheckWrapModulo();

    // one shot LFO?
    if (m_lfo_mode == LFOSHOT && wrap)
    {
        m_note_on = false;

        if (quad_phase_output)
        {
            *quad_phase_output = 0.0;
        }

        return 0.0;
    }

    // for Quad Phase output
    // advance modulo by 0.25 = 90 degrees
    double quad_modulo = m_modulo + 0.25;

    //// check and wrap
    if (quad_modulo >= 1.0)
    {
        quad_modulo -= 1.0;
    }

    switch (m_waveform)
    {
    case sine:
    {
        double angle = m_modulo * 2.0 * M_PI - M_PI;
        out = parabolic_sine(-angle, true);
        angle = quad_modulo * 2.0 * M_PI - M_PI;
        qp_out = parabolic_sine(-angle, true);
        break;
    }

    case usaw:
    case dsaw:
    {
        // --- one shot is unipolar for saw
        if (m_lfo_mode != LFOSHOT)
        {
            // unipolar to bipolar
            out = unipolar_to_bipolar(m_modulo);
            qp_out = unipolar_to_bipolar(quad_modulo);
        }
        else
        {
            out = m_modulo - 1.0;
            qp_out = quad_modulo - 1.0;
        }

        // invert for downsaw
        if (m_waveform == dsaw)
        {
            out *= -1.0;
            qp_out *= -1.0;
        }

        break;
    }

    case square:
    {
        // check pulse width and output either +1 or -1
        out = m_modulo > m_pulse_width / 100.0 ? -1.0 : +1.0;
        qp_out = quad_modulo > m_pulse_width / 100.0 ? -1.0 : +1.0;

        break;
    }

    case tri:
    {
        if (m_modulo < 0.5) // out  = 0 -> +1
            out = 2.0 * m_modulo;
        else // out = +1 -> 0
            out = 1.0 - 2.0 * (m_modulo - 0.5);

        if (m_lfo_mode != LFOSHOT)
            // unipolar to bipolar
            out = unipolar_to_bipolar(out);

        if (quad_modulo < 0.5) // out  = 0 -> +1
            qp_out = 2.0 * quad_modulo;
        else // out = +1 -> 0
            qp_out = 1.0 - 2.0 * (quad_modulo - 0.5);

        // unipolar to bipolar
        qp_out = unipolar_to_bipolar(qp_out);

        break;
    }

    // --- expo is unipolar!
    case expo:
    {
        // calculate the output directly
        out = concave_inverted_transform(m_modulo);
        qp_out = concave_inverted_transform(quad_modulo);

        break;
    }

    case rsh:
    case qrsh:
    {
        // this is the very first run
        if (m_rsh_counter < 0)
        {
            if (m_waveform == rsh)
                m_rsh_value = do_white_noise();
            else
                m_rsh_value = do_pn_sequence(&m_pn_register);

            m_rsh_counter = 1.0;
        }
        // hold time exceeded?
        else if (m_rsh_counter > SAMPLE_RATE / m_fo)
        {
            m_rsh_counter -= SAMPLE_RATE / m_fo;

            if (m_waveform == rsh)
                m_rsh_value = do_white_noise();
            else
                m_rsh_value = do_pn_sequence(&m_pn_register);
        }

        // inc the counter
        m_rsh_counter += 1.0;

        // output held value
        out = m_rsh_value;

        // not meaningful for this output
        qp_out = m_rsh_value;
        break;
    }

    default:
        break;
    }

    // ok to inc modulo now
    IncModulo();

    if (modmatrix)
    {
        // write our outputs into their destinations
        modmatrix->sources[m_mod_dest_output1] = out * m_amplitude * m_amp_mod;

        // add quad phase/stereo output
        modmatrix->sources[m_mod_dest_output2] =
            qp_out * m_amplitude * m_amp_mod;
    }

    if (quad_phase_output)
        *quad_phase_output = qp_out * m_amplitude * m_amp_mod;

    return out * m_amplitude * m_amp_mod;
}

void LFO::StartOscillator()
{
    if (m_lfo_mode == LFOSYNC || m_lfo_mode == LFOSHOT)
        Reset();

    m_note_on = true;
}

void LFO::StopOscillator() { m_note_on = false; }
