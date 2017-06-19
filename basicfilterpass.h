#pragma once

#include <stdbool.h>

#include "defjams.h"
#include "fx.h"

enum { LOWPASS, HIGHPASS, ALLPASS, BANDPASS };

typedef struct filterpass {
    fx m_fx; // API
    double m_buf[2];
    int m_buf_pos;
    double m_freq;
    double m_coef;
    double m_costh;
    double m_rr;
    double m_rsq;
    double m_scal;
    unsigned int m_type; // ^ ALLPASS etc.
} filterpass;

filterpass *new_filterpass(double freq, unsigned int type);
void filterpass_cook(filterpass *fp);

void filterpass_status(void *self, char *string);
double filterpass_process_audio(void *self, double input);

void filterpass_set_freq(filterpass *fp, double freq);
void filterpass_set_type(filterpass *fp, unsigned int type);
