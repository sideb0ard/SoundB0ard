#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "effect.h"
#include "utils.h"

EFFECT *new_delay(double duration, effect_type e_type)
{
    EFFECT *e;
    e = (EFFECT *)calloc(1, sizeof(EFFECT));
    if (e == NULL)
        return NULL;

    printf("Durrr! %f\n", duration);

    double *buffer;
    int buf_length = (int)(duration * SAMPLE_RATE);
    buffer = (double *)calloc(buf_length, sizeof(double));
    if (buffer == NULL) {
        perror("Couldn't allocate effect buffer");
        free(e);
        return NULL;
    }
    e->m_duration = duration;
    e->m_delay_in_samples = buf_length;
    e->buffer = buffer;
    e->buf_length = buf_length;
    e->type = e_type;

    return e;
}

EFFECT *new_decimator()
{
    EFFECT *e;
    e = (EFFECT *)calloc(1, sizeof(EFFECT));
    if (e == NULL)
        return NULL;
    e->cnt = 0;
    e->bits = 16;
    e->rate = 0.5;
    e->m = 1 << (e->bits - 1);
    return e;
}

EFFECT *new_distortion()
{
    EFFECT *e;
    e = (EFFECT *)calloc(1, sizeof(EFFECT));
    if (e == NULL)
        return NULL;

    return e;
}

EFFECT *new_freq_pass(double freq, effect_type pass_type)
{
    EFFECT *e;
    e = (EFFECT *)calloc(1, sizeof(EFFECT));
    if (e == NULL)
        return NULL;

    double *buffer;
    int buf_length = 2;
    buffer = (double *)calloc(buf_length, sizeof(double));
    if (buffer == NULL) {
        perror("Couldn't allocate effect buffer");
        free(e);
        return NULL;
    }

    // double costh;
    double r, bw, rr, rsq, costh, scal;
    switch (pass_type) {
    case LOWPASS:
        costh = 2. - cos(TWO_PI * freq / SAMPLE_RATE);
        e->coef = sqrt(costh * costh - 1.) - costh;
        e->type = LOWPASS;
        e->costh = costh;
        break;
    case HIGHPASS:
        costh = 2. - cos(TWO_PI * freq / SAMPLE_RATE);
        e->coef = costh - sqrt(costh * costh - 1.);
        e->type = HIGHPASS;
        e->costh = costh;
        break;
    case BANDPASS:
        bw = 50; // bandwidth - completely random number??! (noidea)
        rr = 2 * (r = 1. - M_PI * (bw / SAMPLE_RATE));
        rsq = r * r;
        costh = (rr / (1. + rsq)) * cos(TWO_PI * freq / SAMPLE_RATE);
        scal = (1 - rsq) * sin(acos(costh));
        e->rr = rr;
        e->rsq = rsq;
        e->costh = costh;
        e->scal = scal;
        e->type = BANDPASS;
        break;
    default:
        break;
    }

    e->buffer = buffer;
    e->buf_length = buf_length;

    return e;
}

void write_delay_and_inc(EFFECT *self, double val)
{
    self->buffer[self->buf_write_idx] = val;
    self->buf_write_idx++;
    if (self->buf_write_idx >= self->buf_length)
        self->buf_write_idx = 0;
    self->buf_read_idx++;
    if (self->buf_read_idx >= self->buf_length)
        self->buf_read_idx = 0;
}

double read_delay(EFFECT *self)
{
    double yn = self->buffer[self->buf_read_idx];
    int read_idx_1 = self->buf_read_idx - 1;
    if (read_idx_1 < 0)
        read_idx_1 = self->buf_length - 1;
    double yn_1 = self->buffer[read_idx_1];
    double frac_delay =
        self->m_delay_in_samples - (int)self->m_delay_in_samples;
    return scaleybum(0, 1, yn, yn_1, frac_delay);
}
