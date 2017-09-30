#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitcrush.h"

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
}

void bitcrush_status(void *self, char *status_string)
{
    bitcrush *bc = (bitcrush *)self;
    snprintf(status_string, MAX_PS_STRING_SZ, "bitdepth:%d bitrate:%d", bc->bitdepth, bc->bitrate);
}

void bitcrush_set_bitdepth(bitcrush *bc, int val)
{
    if (val >= 1 && val <= 16)
        bc->bitdepth = val;
    else
        printf("Val must be between 1 and 16\n");
}

void bitcrush_set_bitrate(bitcrush *bc, int val)
{
    if (val >= 1 && val <= SAMPLE_RATE)
        bc->bitrate = val;
    else
        printf("Val must be between 1 and %d\n", SAMPLE_RATE);
}
double bitcrush_process_audio(void *self, double input)
{
    bitcrush *bc = (bitcrush *)self;
    double xn = input;

    return xn;
}
