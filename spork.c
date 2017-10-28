#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "spork.h"
#include "utils.h"

extern mixer *mixr;

spork *new_spork(double freq)
{
    spork *s = (spork *)calloc(1, sizeof(spork));

    wt_initialize(&s->m_osc);

    s->sg.gennext = &spork_gennext;
    s->sg.status = &spork_status;
    s->sg.getvol = &spork_getvol;
    s->sg.setvol = &spork_setvol;
    s->sg.start = &spork_start;
    s->sg.stop = &spork_stop;
    s->sg.type = SPORK_TYPE;

    s->m_volume = 0.6;

    s->freq = freq;
    s->mode = 0;
    s->waveform = 0;
    s->polarity = 0;
    spork_update(s);
    spork_start(s);

    return s;
}

stereo_val spork_gennext(void *sg, mixer_timing_info timing_info)
{
    spork *s = (spork *)sg;
    (void)timing_info;

    stereo_val return_val = {0, 0};
    double scratch_val;
    if (!s->active)
        return return_val;

    double quad_return_val = 0.;
    scratch_val = wt_do_oscillate(&s->m_osc, &quad_return_val);
    scratch_val = effector(&s->sg, scratch_val);
    scratch_val = envelopor(&s->sg, scratch_val);

    return_val.left = scratch_val * s->m_volume;
    return_val.right = scratch_val * s->m_volume;

    return return_val;
}

void spork_status(void *self, wchar_t *ss)
{
    spork *s = (spork *)self;
    swprintf(ss, MAX_PS_STRING_SZ, WANSI_COLOR_GREEN
             "[SPORK] vol: %.2lf freq:%.2f waveform:%d mode:%d polarity:%d\n",
             s->m_volume, s->freq, s->waveform, s->mode, s->polarity);
    wcscat(ss, WANSI_COLOR_RESET);
}

double spork_getvol(void *self)
{
    spork *s = (spork *)self;
    return s->m_volume;
}

void spork_setvol(void *self, double v)
{
    spork *s = (spork *)self;
    s->m_volume = v;
}

void spork_start(void *self)
{
    spork *s = (spork *)self;
    wt_start(&s->m_osc);
    s->active = true;
}

void spork_stop(void *self)
{
    spork *s = (spork *)self;
    wt_stop(&s->m_osc);
    s->active = false;
}

void spork_set_freq(spork *s, double freq)
{
    if (freq >= 25 && freq <= 4200)
        s->freq = freq;
    else
        printf("freq must be between 25 and 4200\n");

    spork_update(s);
}

void spork_set_waveform(spork *s, unsigned int wave)
{
    if (wave < 4)
        s->waveform = wave;
    else
        printf("Wave has to be between 0 and 3\n");
    spork_update(s);
}

void spork_set_mode(spork *s, unsigned int mode)
{
    if (mode < 2)
        s->mode = mode;
    else
        printf("Wave has to be 0(normal) or 1(bandlimit)\n");
    spork_update(s);
}

void spork_set_polarity(spork *s, unsigned int polarity)
{
    if (polarity < 2)
        s->polarity = polarity;
    else
        printf("Polarity has to be 0(bipolar) or 1(unipolar)\n");
    spork_update(s);
}

void spork_update(spork *s)
{
    s->m_osc.freq = s->freq;
    s->m_osc.waveform = s->waveform;
    s->m_osc.mode = s->mode;
    s->m_osc.polarity = s->polarity;
    wt_update(&s->m_osc);
}
