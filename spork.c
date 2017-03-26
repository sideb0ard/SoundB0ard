#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "spork.h"
#include "utils.h"

spork *new_spork()
{
    spork *s = (spork *)calloc(1, sizeof(spork));
    osc_new_settings(&s->osc);
    s->osc.m_note_on = true;

    s->sg.gennext = &spork_gennext;
    s->sg.status = &spork_status;
    s->sg.getvol = &spork_getvol;
    s->sg.setvol = &spork_setvol;
    s->sg.type = LFO_TYPE;

    return s;
}

double spork_gennext(void *sg)
{
    spork *l = (spork*) sg;
    oscillator *self = &l->osc;

    osc_update(self, "SpoooooooOOOrk");

    // output
    double out = 0.0;
    double qp_out = 0.0;

    // always first
    bool wrap = osc_check_wrap_modulo(self);

    // one shot LFO?
    if (self->m_lfo_mode == LFOSHOT && wrap) {
        self->m_note_on = false;
        return 0.0;
    }

    // for QP output
    // advance modulo by 0.25 = 90 degrees
    double quad_modulo = self->m_modulo + 0.25;

    //// check and wrap
    if (quad_modulo >= 1.0) {
        quad_modulo -= 1.0;
    }

    // decode and calculate
    // printf("WAVEFORM %d\n", self->m_waveform);
    switch (self->m_waveform) {
    case sine: {
        // calculate angle
        double angle = self->m_modulo * 2.0 * M_PI - M_PI;

        // call the parabolicSine approximator
        out = parabolic_sine(-angle, true);

        // use second modulo for quad phase
        angle = quad_modulo * 2.0 * M_PI - M_PI;
        qp_out = parabolic_sine(-angle, true);

        break;
    }

    case usaw:
    case dsaw: {
        // --- one shot is unipolar for saw
        if (self->m_lfo_mode != LFOSHOT) {
            // unipolar to bipolar
            out = unipolar_to_bipolar(self->m_modulo);
            qp_out = unipolar_to_bipolar(quad_modulo);
        }
        else {
            out = self->m_modulo - 1.0;
            qp_out = quad_modulo - 1.0;
        }

        // invert for downsaw
        if (self->m_waveform == dsaw) {
            out *= -1.0;
            qp_out *= -1.0;
        }

        break;
    }

    case square: {
        // check pulse width and output either +1 or -1
        out = self->m_modulo > self->m_pulse_width / 100.0 ? -1.0 : +1.0;
        qp_out = quad_modulo > self->m_pulse_width / 100.0 ? -1.0 : +1.0;

        break;
    }

    case tri: {
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
    case expo: {
        // calculate the output directly
        out = concave_inverted_transform(self->m_modulo);
        qp_out = concave_inverted_transform(quad_modulo);

        break;
    }

    case rsh:
    case qrsh: {
        // this is the very first run
        if (self->m_rsh_counter < 0) {
            if (self->m_waveform == rsh)
                self->m_rsh_value = do_white_noise();
            else
                self->m_rsh_value = do_pn_sequence(&self->m_pn_register);

            self->m_rsh_counter = 1.0;
        }
        // hold time exceeded?
        else if (self->m_rsh_counter > SAMPLE_RATE / self->m_fo) {
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

    ////// self->m_amplitude & self->m_amp_mod is calculated in update() on base
    ////// class
    //if (self->m_v_modmatrix) {
    //    // write our outputs into their destinations
    //    // printf("LFO WRITING TO m_mod_dest_output1 OUT:%f m_AMP: %f m_AMP_MOD:
    //    // %f FINALVAL: %f\n", out, self->m_amplitude, self->m_amp_mod, out *
    //    // self->m_amplitude * self->m_amp_mod);
    //    self->m_v_modmatrix->m_sources[self->m_mod_dest_output1] =
    //        out * self->m_amplitude * self->m_amp_mod;

    //    // add quad phase/stereo output
    //    self->m_v_modmatrix->m_sources[self->m_mod_dest_output2] =
    //        qp_out * self->m_amplitude * self->m_amp_mod;
    //}


    //// self->m_amplitude & self->m_amp_mod is calculated in update() on base
    //// class
    return out * self->m_amplitude * self->m_amp_mod;

}

void spork_status(void *self, wchar_t *ss) {}
double spork_getvol(void *self) { return 0.; }
void spork_setvol(void *self, double v) {}
