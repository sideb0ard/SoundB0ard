#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "basicfilterpass.h"
#include "defjams.h"

const char *filtertype_to_name[] = {"LOWPASS", "HIGHPASS", "ALLPASS",
                                    "BANDPASS"};

filterpass *new_filterpass(double freq, unsigned int type)
{
    filterpass *fp = calloc(1, sizeof(filterpass));
    fp->m_freq = freq;
    fp->m_type = type;

    fp->m_fx.type = BASICFILTER;
    fp->m_fx.enabled = true;
    fp->m_fx.status = &filterpass_status;
    fp->m_fx.process = &filterpass_process_audio;

    filterpass_cook(fp);

    return fp;
}

void filterpass_cook(filterpass *fp)
{
    double r;
    switch (fp->m_type) {
    case LOWPASS:
        fp->m_costh = 2. - cos(TWO_PI * fp->m_freq / SAMPLE_RATE);
        fp->m_coef = sqrt(fp->m_costh * fp->m_costh - 1.) - fp->m_costh;
        break;
    case HIGHPASS:
        fp->m_costh = 2. - cos(TWO_PI * fp->m_freq / SAMPLE_RATE);
        fp->m_coef = fp->m_costh - sqrt(fp->m_costh * fp->m_costh - 1.);
        break;
    case BANDPASS:
        r = 1. - M_PI * (50 / SAMPLE_RATE);
        fp->m_rr = 2 * r;
        fp->m_rsq = r * r;
        fp->m_costh = (fp->m_rr / (1. + fp->m_rsq)) *
                      cos(TWO_PI * fp->m_freq / SAMPLE_RATE);
        fp->m_scal = (1 - fp->m_rsq) * sin(acos(fp->m_costh));
        break;
    default:
        break;
    }
}

void filterpass_status(void *self, char *status_string)
{
    filterpass *fp = (filterpass *)self;
    snprintf(status_string, MAX_PS_STRING_SZ, "type:%s freq:%.2f",
             filtertype_to_name[fp->m_type], fp->m_freq);
}

double filterpass_process_audio(void *self, double input)
{
    filterpass *fp = (filterpass *)self;

    double returnval = 0.;
    double val1 = 0, val2 = 0;
    switch (fp->m_type) {
    case ALLPASS:
        val1 = fp->m_buf[fp->m_buf_pos];
        val2 = input - (val1 * 0.5);
        fp->m_buf[fp->m_buf_pos++] = val2;
        returnval = val1 + (val2 * 0.2);
        if (fp->m_buf_pos >= 2)
            fp->m_buf_pos = 0;
        break;
    case LOWPASS:
        returnval = (input * (1 + fp->m_coef) - fp->m_buf[0] * fp->m_coef);
        fp->m_buf[0] = returnval;
        break;
    case HIGHPASS:
        returnval = (input * (1 - fp->m_coef) - fp->m_buf[0] * fp->m_coef);
        fp->m_buf[0] = returnval;
        break;
    case BANDPASS:
        returnval =
            (input * fp->m_scal + fp->m_rr * fp->m_costh * fp->m_buf[0] -
             fp->m_rsq * fp->m_buf[1]);
        fp->m_buf[1] = fp->m_buf[0];
        fp->m_buf[0] = returnval;
        break;
    }

    return returnval;
}

void filterpass_set_freq(filterpass *fp, double freq)
{
    if (freq >= 20 && freq <= 20480) {
        fp->m_freq = freq;
        filterpass_cook(fp);
    }
    else
        printf("Freq has to be between 20 and 20480\n");
}

void filterpass_set_type(filterpass *fp, unsigned int type)
{
    if (type < 4) {
        fp->m_type = type;
        filterpass_cook(fp);
    }
    else
        printf("Filter type has to be 0-3 (ALLPASS, LOWPASS, HIGHPASS, "
               "BANDPASS)\n");
}
