#pragma once

#include "fx.h"

typedef struct distortion {
    fx m_fx; // API

    double m_threshold;

} distortion;

// enum { COMP, LIMIT, EXPAND, GATE };

distortion *new_distortion(void);
void distortion_init(distortion *d);

void distortion_status(void *self, char *status_string);
double distortion_process(void *self, double input);

void distortion_set_threshold(distortion *d, double val);
