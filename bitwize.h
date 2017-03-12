#ifndef BITWIZE_H
#define BITWIZE_H

#include <stdio.h>
#include <wchar.h>

#include "sound_generator.h"

typedef struct t_bitwize {

    SOUNDGEN sound_generator;

    lfo *m_lfo;

    double incr;
    double vol;

    int pattern;
    int tick;
    double current_val;

} BITWIZE;

BITWIZE *new_bitwize(int pattern);

char bitwize_process(int pattern, int t);
double bitwize_gennext(void *self);
void bitwize_setvol(void *self, double v);
double bitwize_getvol(void *self);

void bitwize_status(void *self, wchar_t *status_string);

#endif // BITWIZE_H
