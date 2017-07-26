#pragma once

#include <stdbool.h>

#include "defjams.h"
#include "filter_moogladder.h"
#include "fx.h"

enum { LOWPASS, HIGHPASS, ALLPASS, BANDPASS };

typedef struct filterpass {
    fx m_fx; // API
    filter_moogladder m_filter;
} filterpass;

filterpass *new_filterpass(void);

void filterpass_status(void *self, char *string);
double filterpass_process_audio(void *self, double input);
