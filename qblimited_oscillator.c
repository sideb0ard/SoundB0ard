#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "lookuptables.h"
#include "qblimited_oscillator.h"
#include "utils.h"

char *s_waveform_names[] = {"SINE",   "SAW1",  "SAW2",   "SAW3",   "TRI",
                            "SQUARE", "NOISE", "PNOISE", "MAX_OSC"};

qb_osc *qb_osc_new()
{
    qb_osc *qb = (qb_osc *)calloc(1, sizeof(qb_osc));
    if (qb == NULL) {
        printf("Dinghie, mate\n");
        return NULL;
    }

    osc_new_settings(&qb->osc);
    qb_set_soundgenerator_interface(qb);

    return qb;
}

void qb_set_soundgenerator_interface(qb_osc *qb)
{
    qb->osc.do_oscillate = &qb_do_oscillate;
    qb->osc.start_oscillator = &qb_start_oscillator;
    qb->osc.stop_oscillator = &qb_stop_oscillator;
    qb->osc.reset_oscillator = &qb_reset_oscillator;
    qb->osc.update_oscillator = &osc_update; // from base class
}
double qb_do_sawtooth(oscillator *self, double modulo, double dInc)
{
    double dTrivialSaw = 0.0;
    double out = 0.0;

    if (self->m_waveform == SAW1) // SAW1 = normal sawtooth (ramp)
        dTrivialSaw = unipolar_to_bipolar(modulo);
    else if (self->m_waveform == SAW2) // SAW2 = one sided wave shaper
        dTrivialSaw = 2.0 * (tanh(1.5 * modulo) / tanh(1.5)) - 1.0;
    else if (self->m_waveform == SAW3) // SAW3 = double sided wave shaper
    {
        dTrivialSaw = unipolar_to_bipolar(modulo);
        dTrivialSaw = tanh(1.5 * dTrivialSaw) / tanh(1.5);
    }

    // --- NOTE: Fs/8 = Nyquist/4
    if (self->m_fo <= SAMPLE_RATE / 8.0) {
        out = dTrivialSaw + do_blep_n(&dBLEPTable_8_BLKHAR[0], /* BLEP table */
                                      4096,       /* BLEP table length */
                                      modulo,     /* current phase value */
                                      fabs(dInc), /* abs(dInc) is for FM
                                                     synthesis with negative
                                                     frequencies */
                                      1.0,    /* sawtooth edge height = 1.0 */
                                      false,  /* falling edge */
                                      4,      /* 1 point per side */
                                      false); /* no interpolation */
    }
    else // to prevent overlapM_PIng BLEPs, default back to 2-point for f >
         // Nyquist/4
    {
        out = dTrivialSaw + do_blep_n(&dBLEPTable[0], /* BLEP table */
                                      4096,           /* BLEP table length */
                                      modulo,         /* current phase value */
                                      fabs(dInc),     /* abs(dInc) is for FM
                                                         synthesis with negative
                                                         frequencies */
                                      1.0,   /* sawtooth edge height = 1.0 */
                                      false, /* falling edge */
                                      1,     /* 1 point per side */
                                      true); /* no interpolation */
    }

    // --- or do PolyBLEP
    // out = dTrivialSaw + doPolyBLEP_2(modulo,
    //								  abs(dInc),/*
    // abs(dInc)
    // is
    // for
    // FM
    // synthesis
    // with
    // negative
    // frequencies */
    //								  1.0,
    ///*
    // sawtooth
    // edge
    //=
    // 1.0
    //*/
    //								  false);
    ///*
    // falling
    // edge
    //*/

    return out;
}

// square with polyBLEP
double qb_do_square(oscillator *self, double modulo, double dInc)
{
    // --- sum-of-saws method
    //
    // --- pretend to be SAW1 type
    self->m_waveform = SAW1;

    // --- get first sawtooth output
    double dSaw1 = qb_do_sawtooth(self, modulo, dInc);

    // --- phase shift on second oscillator
    if (dInc > 0)
        modulo += self->m_pulse_width / 100.0;
    else
        modulo -= self->m_pulse_width / 100.0;

    // --- for positive frequencies
    if (dInc > 0 && modulo >= 1.0)
        modulo -= 1.0;

    // --- for negative frequencies
    if (dInc < 0 && modulo <= 0.0)
        modulo += 1.0;

    // --- subtract saw method
    double dSaw2 = qb_do_sawtooth(self, modulo, dInc);

    // --- subtract = 180 out of phase
    double out = 0.5 * dSaw1 - 0.5 * dSaw2;

    // --- apply DC correction
    double dCorr = 1.0 / (self->m_pulse_width / 100.0);

    // --- modfiy for less than 50%
    if ((self->m_pulse_width / 100.0) < 0.5)
        dCorr = 1.0 / (1.0 - (self->m_pulse_width / 100.0));

    // --- apply correction
    out *= dCorr;

    // --- reset back to SQUARE
    self->m_waveform = SQUARE;

    return out;
}

// DPW
double qb_do_triangle(double modulo, double inc, double fo,
                      double square_modulator, double *z_register)
{
    // double out = 0.0;
    // bool bDone = false;

    // bipolar conversion and squaring
    double bipolar = unipolar_to_bipolar(modulo);
    double sq = bipolar * bipolar;

    // inversion
    double inv = 1.0 - sq;

    // modulation with square modulo
    double sq_mod = inv * square_modulator;

    // original
    double differentiated_sq_mod = sq_mod - *z_register;
    *z_register = sq_mod;

    // c = fs/[4fo(1-2fo/fs)]
    double c = SAMPLE_RATE / (4.0 * 2.0 * fo * (1 - inc));

    return differentiated_sq_mod * c;
}

// the rendering function
double qb_do_oscillate(oscillator *self, double *aux_output)
{
    if (!self->m_note_on)
        return 0.0;

    double out = 0.0;

    // always first
    bool bWrap = osc_check_wrap_modulo(self);

    // added for PHASE MODULATION
    double calc_modulo = self->m_modulo + self->m_phase_mod;
    check_wrap_index(&calc_modulo);

    switch (self->m_waveform) {
    case SINE: {
        // calculate angle
        double angle = calc_modulo * 2.0 * (double)M_PI - (double)M_PI;

        // call the parabolic_sine approximator
        out = parabolic_sine(-1.0 * angle, true);

        break;
    }

    case SAW1:
    case SAW2:
    case SAW3: {
        // do first waveform
        out = qb_do_sawtooth(self, calc_modulo, self->m_inc);

        break;
    }

    case SQUARE: {
        out = qb_do_square(self, calc_modulo, self->m_inc);

        break;
    }

    case TRI: {
        // do first waveform
        if (bWrap)
            self->m_dpw_square_modulator *= -1.0;

        out = qb_do_triangle(calc_modulo, self->m_inc, self->m_fo,
                             self->m_dpw_square_modulator, &self->m_dpw_z1);

        break;
    }

    case NOISE: {
        // use helper function
        out = do_white_noise();

        break;
    }

    case PNOISE: {
        // use helper function
        out = do_pn_sequence(&self->m_pn_register);

        break;
    }
    default:
        break;
    }

    // --- ok to inc modulo now
    osc_inc_modulo(self);
    if (self->m_waveform == TRI)
        osc_inc_modulo(self);

    if (self->m_v_modmatrix) {
        self->m_v_modmatrix->m_sources[self->m_mod_dest_output1] =
            out * self->m_amplitude * self->m_amp_mod;
        self->m_v_modmatrix->m_sources[self->m_mod_dest_output2] =
            out * self->m_amplitude * self->m_amp_mod;
    }

    // --- self->m_amp_mod is set in update()
    if (aux_output)
        *aux_output = out * self->m_amplitude * self->m_amp_mod;

    // --- self->m_amp_mod is set in update()
    return out * self->m_amplitude * self->m_amp_mod;
}

void qb_reset_oscillator(oscillator *self)
{
    osc_reset(self);
    // --- saw/tri starts at 0.5
    if (self->m_waveform == SAW1 || self->m_waveform == SAW2 ||
        self->m_waveform == SAW3 || self->m_waveform == TRI) {
        self->m_modulo = 0.5;
    }
}
void qb_start_oscillator(oscillator *self)
{
    osc_reset(self);
    self->m_note_on = true;
}

void qb_stop_oscillator(oscillator *self) { self->m_note_on = false; }
