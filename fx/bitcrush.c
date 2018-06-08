#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitcrush.h"
#include "utils.h"

bitcrush *new_bitcrush()
{
    bitcrush *bc = calloc(1, sizeof(bitcrush));
    bitcrush_init(bc);

    bc->m_fx.type = BITCRUSH;
    bc->m_fx.enabled = true;
    bc->m_fx.status = &bitcrush_status;
    bc->m_fx.process = &bitcrush_process_audio;

    return bc;
}

void bitcrush_init(bitcrush *bc)
{
    bc->bitdepth = 6;
    bc->bitrate = 4096;
    bc->sample_hold_freq = 1;
    bc->phasor = 0;
    bc->last = 0;
    bitcrush_update(bc);
}

void bitcrush_update(bitcrush *bc)
{
    bc->step = 2 * fast_pow(0.5, bc->bitdepth);
    bc->inv_step = 1 / bc->step;
}

void bitcrush_status(void *self, char *status_string)
{
    bitcrush *bc = (bitcrush *)self;
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "bitdepth:%d bitrate:%d sample_hold_freq:%.2f", bc->bitdepth,
             bc->bitrate, bc->sample_hold_freq);
}

void bitcrush_set_bitdepth(bitcrush *bc, int val)
{
    if (val >= 1 && val <= 16)
    {
        bc->bitdepth = val;
        bitcrush_update(bc);
    }
    else
        printf("Val must be between 1 and 16\n");
}

void bitcrush_set_sample_hold_freq(bitcrush *bc, double val)
{
    val = clamp(0, 1, val);
    bc->sample_hold_freq = val;
    bitcrush_update(bc);
}

void bitcrush_set_bitrate(bitcrush *bc, int val)
{
    if (val >= 200 && val <= SAMPLE_RATE)
    {
        bc->bitrate = val;
        bitcrush_update(bc);
    }
    else
        printf("Val must be between 200 and %d\n", SAMPLE_RATE);
}

double bitcrush_process_audio(void *self, double input)
{
    bitcrush *bc = (bitcrush *)self;
    bc->phasor += bc->sample_hold_freq;
    if (bc->phasor >= 1)
    {
        bc->phasor -= 1;
        bc->last = bc->step * floor((input * bc->inv_step) + 0.5);
    }

    return bc->last;
}
