#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "lookuptables.h"
#include "qblimited_oscillator.h"
#include "utils.h"

qb_osc *qb_osc_new()
{
    qb_osc *qb = (qb_osc *)calloc(1, sizeof(qb_osc));
    if (qb == NULL)
    {
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

inline double qb_do_sawtooth(oscillator *self, double modulo, double inc)
{
    double trivial_saw = 0.0;
    double out = 0.0;

    if (self->m_waveform == SAW1)
        trivial_saw = unipolar_to_bipolar(modulo);
    else if (self->m_waveform == SAW2)
        trivial_saw = 2.0 * (tanh(1.5 * modulo) / tanh(1.5)) - 1.0;
    else if (self->m_waveform == SAW3)
    {
        trivial_saw = unipolar_to_bipolar(modulo);
        trivial_saw = tanh(1.5 * trivial_saw) / tanh(1.5);
    }

    // --- NOTE: Fs/8 = Nyquist/4
    if (self->m_fo <= SAMPLE_RATE / 8.0)
    {
        out = trivial_saw + do_blep_n(&blep_table_8_blkhar[0], blep_table_size,
                                      modulo,    /* current phase value */
                                      fabs(inc), /* abs(inc) is for FM
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
        out = trivial_saw + do_blep_n(&blep_table[0], blep_table_size,
                                      modulo,    /* current phase value */
                                      fabs(inc), /* abs(inc) is for FM
                                                     synthesis with negative
                                                     frequencies */
                                      1.0,   /* sawtooth edge height = 1.0 */
                                      false, /* falling edge */
                                      1,     /* 1 point per side */
                                      true); /* no interpolation */
    }

    return out;
}

inline double qb_do_square(oscillator *self, double modulo, double inc)
{
    self->m_waveform = SAW1;
    double dSaw1 = qb_do_sawtooth(self, modulo, inc);
    if (inc > 0)
        modulo += self->m_pulse_width / 100.0;
    else
        modulo -= self->m_pulse_width / 100.0;

    if (inc > 0 && modulo >= 1.0)
        modulo -= 1.0;

    if (inc < 0 && modulo <= 0.0)
        modulo += 1.0;

    double dSaw2 = qb_do_sawtooth(self, modulo, inc);
    double out = 0.5 * dSaw1 - 0.5 * dSaw2;
    double dCorr = 1.0 / (self->m_pulse_width / 100.0);

    if ((self->m_pulse_width / 100.0) < 0.5)
        dCorr = 1.0 / (1.0 - (self->m_pulse_width / 100.0));

    out *= dCorr;
    self->m_waveform = SQUARE;

    return out;
}

inline double qb_do_triangle(double modulo, double inc, double fo,
                             double square_modulator, double *z_register)
{

    double bipolar = unipolar_to_bipolar(modulo);
    double sq = bipolar * bipolar;
    double inv = 1.0 - sq;
    double sq_mod = inv * square_modulator;
    double differentiated_sq_mod = sq_mod - *z_register;
    *z_register = sq_mod;
    double c = SAMPLE_RATE / (4.0 * 2.0 * fo * (1 - inc));

    return differentiated_sq_mod * c;
}

inline double qb_do_oscillate(oscillator *self, double *aux_output)
{
    if (!self->m_note_on)
        return 0.0;

    double out = 0.0;

    self->just_wrapped = false;
    bool wrap = osc_check_wrap_modulo(self);
    if (wrap)
        self->just_wrapped = true;

    double calc_modulo = self->m_modulo + self->m_phase_mod;
    check_wrap_index(&calc_modulo);

    switch (self->m_waveform)
    {
    case SINE:
    {
        double angle = calc_modulo * 2.0 * (double)M_PI - (double)M_PI;
        out = parabolic_sine(-1.0 * angle, true);
        break;
    }

    case SAW1:
    case SAW2:
    case SAW3:
    {
        out = qb_do_sawtooth(self, calc_modulo, self->m_inc);
        break;
    }

    case SQUARE:
    {
        out = qb_do_square(self, calc_modulo, self->m_inc);
        break;
    }

    case TRI:
    {
        if (wrap)
            self->m_dpw_square_modulator *= -1.0;

        out = qb_do_triangle(calc_modulo, self->m_inc, self->m_fo,
                             self->m_dpw_square_modulator, &self->m_dpw_z1);
        break;
    }

    case NOISE:
    {
        out = do_white_noise();
        break;
    }

    case PNOISE:
    {
        out = do_pn_sequence(&self->m_pn_register);
        break;
    }
    default:
        break;
    }

    osc_inc_modulo(self);
    if (self->m_waveform == TRI)
        osc_inc_modulo(self);

    if (self->m_v_modmatrix)
    {
        self->m_v_modmatrix->m_sources[self->m_mod_dest_output1] =
            out * self->m_amplitude * self->m_amp_mod;
        self->m_v_modmatrix->m_sources[self->m_mod_dest_output2] =
            out * self->m_amplitude * self->m_amp_mod;
    }

    if (aux_output)
        *aux_output = out * self->m_amplitude * self->m_amp_mod;

    return out * self->m_amplitude * self->m_amp_mod;
}

inline void qb_reset_oscillator(oscillator *self)
{
    osc_reset(self);
    if (self->m_waveform == SAW1 || self->m_waveform == SAW2 ||
        self->m_waveform == SAW3 || self->m_waveform == TRI)
        self->m_modulo = 0.5;
}
inline void qb_start_oscillator(oscillator *self)
{
    osc_reset(self);
    self->m_note_on = true;
}

inline void qb_stop_oscillator(oscillator *self) { self->m_note_on = false; }
