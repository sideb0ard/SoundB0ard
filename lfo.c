#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "defjams.h"
#include "lfo.h"

LFO *new_lfo(double freq, oscil_type type)
{
    LFO *lfo = calloc(1, sizeof(LFO));
    if ( lfo == NULL ) {
        printf("BURNEY - MEMORY ISSUES, PAL\n");
        return NULL;
    }

    lfo->m_amp = 1.0;
    lfo->m_freq = freq;
    lfo->m_incr = freq / SAMPLE_RATE;
    lfo->m_type = type;

    return lfo;
}

double lfo_gennext(LFO *self)
{
    switch(self->m_type)
    {
        case SINE:
            {
                double val;
                val = sin(self->m_curphase);
                self->m_curphase += self->m_incr;
                if (self->m_curphase >= TWO_PI)
                    self->m_curphase -= TWO_PI;
                if (self->m_curphase < 0.0)
                    self->m_curphase += TWO_PI;
                return val;
             }
        case SAWU:
        case SAWD:
            {
                if (self->m_curphase >= 1.0)
                    self->m_curphase -= 1.0;
                double val = 2.0 * self->m_curphase - 1.0; // bipolar output - no idea
                self->m_curphase += self->m_incr;
                if (self->m_type == SAWD)
                    val += -1.0; 
                return val;
            }
        case SQUARE:
            {
                if (self->m_curphase >= 1.0)
                    self->m_curphase -= 1.0;
                double val = self->m_curphase > 50/100.0 ? - 1.0 : +1.0;
                self->m_curphase += self->m_incr;
                return val;
            }
        case NOISE:
            {
                float noise = (float)rand();
                noise = 2.0*(noise/32767.0) - 1.0;
                return noise;
            }
        case TRI:
        default:
            {
                if (self->m_curphase >= 1.0)
                    self->m_curphase -= 1.0;
                double val = 2.0 * fabs(2.0*self->m_curphase - 1.0) - 1.0;
                self->m_curphase += self->m_incr;
                return val;
            }
    }
}


void lfo_change_freq(LFO *self, double freq)
{
    self->m_freq = freq;
}
