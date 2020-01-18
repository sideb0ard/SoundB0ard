#pragma once

#include <stdbool.h>

#include "defjams.h"
#include "fx.h"

typedef struct waveshaper
{
    fx m_fx;                      // API
    double m_arc_tan_k_pos;       // 0.10 - 20
    double m_arc_tan_k_neg;       // 0.10 - 20
    unsigned int m_stages;        // 1 - 10
    unsigned int m_invert_stages; // OFF, ON
} waveshaper;

waveshaper *new_waveshaper(void);
void waveshaper_init(waveshaper *d);

void waveshaper_status(void *self, char *string);
stereo_val waveshaper_process_audio(void *d, stereo_val input);

void waveshaper_set_arc_tan_k_pos(waveshaper *d, double val);
void waveshaper_set_arc_tan_k_neg(waveshaper *d, double val);
void waveshaper_set_stages(waveshaper *d, unsigned int val);
void waveshaper_set_invert_stages(waveshaper *d, unsigned int val);
