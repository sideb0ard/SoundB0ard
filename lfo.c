#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "lfo.h"
#include "oscillator.h"
#include "utils.h"

lfo *lfo_new()
{
    lfo *l = (lfo *)calloc(1, sizeof(lfo));
    if (l == NULL)
    {
        printf("Nae mems for LFO, mate. Sort it oot\n");
        return NULL;
    }

    osc_new_settings(&l->osc);
    lfo_set_soundgenerator_interface(l);

    l->osc.m_fo = DEFAULT_LFO_RATE;

    return l;
}

void lfo_set_soundgenerator_interface(lfo *l)
{
    l->osc.do_oscillate = &lfo_do_oscillate;
    l->osc.do_oscillate = &lfo_do_oscillate;
    l->osc.start_oscillator = &lfo_start_oscillator;
    l->osc.stop_oscillator = &lfo_stop_oscillator;
    l->osc.reset_oscillator = &lfo_reset_oscillator;
    l->osc.update_oscillator = &osc_update; // base clase impl

    l->osc.m_lfo_mode = LFOSYNC;
}

double lfo_do_oscillate(oscillator *self, double *quad_phase_output)
{
    if (!self->m_note_on)
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
    bool wrap = osc_check_wrap_modulo(self);

    // one shot LFO?
    if (self->m_lfo_mode == LFOSHOT && wrap)
    {
        self->m_note_on = false;

        if (quad_phase_output)
        {
            *quad_phase_output = 0.0;
        }

        return 0.0;
    }

    // for QP output
    // advance modulo by 0.25 = 90 degrees
    double quad_modulo = self->m_modulo + 0.25;

    //// check and wrap
    if (quad_modulo >= 1.0)
    {
        quad_modulo -= 1.0;
    }

    // decode and calculate
    // printf("WAVEFORM %d\n", self->m_waveform);
    switch (self->m_waveform)
    {
    case sine:
    {
        // printf("SINE?\n");
        // calculate angle
        double angle = self->m_modulo * 2.0 * M_PI - M_PI;
        // printf("ANGLEE! %f\n", angle);

        // call the parabolicSine approximator
        out = parabolic_sine(-angle, true);
        // printf("OUT! %f\n", out);

        // use second modulo for quad phase
        angle = quad_modulo * 2.0 * M_PI - M_PI;
        qp_out = parabolic_sine(-angle, true);

        break;
    }

    case usaw:
    case dsaw:
    {
        // --- one shot is unipolar for saw
        if (self->m_lfo_mode != LFOSHOT)
        {
            // unipolar to bipolar
            out = unipolar_to_bipolar(self->m_modulo);
            qp_out = unipolar_to_bipolar(quad_modulo);
        }
        else
        {
            out = self->m_modulo - 1.0;
            qp_out = quad_modulo - 1.0;
        }

        // invert for downsaw
        if (self->m_waveform == dsaw)
        {
            out *= -1.0;
            qp_out *= -1.0;
        }

        break;
    }

    case square:
    {
        // check pulse width and output either +1 or -1
        out = self->m_modulo > self->m_pulse_width / 100.0 ? -1.0 : +1.0;
        qp_out = quad_modulo > self->m_pulse_width / 100.0 ? -1.0 : +1.0;

        break;
    }

    case tri:
    {
        if (self->m_modulo < 0.5) // out  = 0 -> +1
            out = 2.0 * self->m_modulo;
        else // out = +1 -> 0
            out = 1.0 - 2.0 * (self->m_modulo - 0.5);

        if (self->m_lfo_mode != LFOSHOT)
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
        out = concave_inverted_transform(self->m_modulo);
        qp_out = concave_inverted_transform(quad_modulo);

        break;
    }

    case rsh:
    case qrsh:
    {
        // this is the very first run
        if (self->m_rsh_counter < 0)
        {
            if (self->m_waveform == rsh)
                self->m_rsh_value = do_white_noise();
            else
                self->m_rsh_value = do_pn_sequence(&self->m_pn_register);

            self->m_rsh_counter = 1.0;
        }
        // hold time exceeded?
        else if (self->m_rsh_counter > SAMPLE_RATE / self->m_fo)
        {
            self->m_rsh_counter -= SAMPLE_RATE / self->m_fo;

            if (self->m_waveform == rsh)
                self->m_rsh_value = do_white_noise();
            else
                self->m_rsh_value = do_pn_sequence(&self->m_pn_register);
        }

        // inc the counter
        self->m_rsh_counter += 1.0;

        // output held value
        out = self->m_rsh_value;

        // not meaningful for this output
        qp_out = self->m_rsh_value;
        break;
    }

    default:
        break;
    }

    // ok to inc modulo now
    osc_inc_modulo(self);

    //// self->m_amplitude & self->m_amp_mod is calculated in update() on base
    //// class
    if (self->m_v_modmatrix)
    {
        // write our outputs into their destinations
        // printf("LFO WRITING TO m_mod_dest_output1 OUT:%f m_AMP: %f
        // m_AMP_MOD:
        // %f FINALVAL: %f\n", out, self->m_amplitude, self->m_amp_mod, out
        // *
        // self->m_amplitude * self->m_amp_mod);
        self->m_v_modmatrix->m_sources[self->m_mod_dest_output1] =
            out * self->m_amplitude * self->m_amp_mod;

        // add quad phase/stereo output
        self->m_v_modmatrix->m_sources[self->m_mod_dest_output2] =
            qp_out * self->m_amplitude * self->m_amp_mod;
    }

    if (quad_phase_output)
        *quad_phase_output = qp_out * self->m_amplitude * self->m_amp_mod;

    // self->m_amplitude & self->m_amp_mod is calculated in update() on base
    // class
    return out * self->m_amplitude * self->m_amp_mod;
}

void lfo_start_oscillator(oscillator *self)
{
    if (self->m_lfo_mode == LFOSYNC || self->m_lfo_mode == LFOSHOT)
        osc_reset(self);

    self->m_note_on = true;
}

void lfo_stop_oscillator(oscillator *self) { self->m_note_on = false; }

void lfo_reset_oscillator(oscillator *self) { osc_reset(self); }
