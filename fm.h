#ifndef FM_H
#define FM_H

#include "oscil.h"
#include "sound_generator.h"

typedef struct FM FM;

typedef struct FM {
    // OSC **oscillators;
    // int num_oscil;
    SOUNDGEN sound_generator;

    OSCIL *mod_osc;
    OSCIL *car_osc;
    float vol;

} FM;

FM *new_fm(double modf, double carf);
FM *new_fm_x(char *m_osc, double modf, char *c_osc, double carf);

double fm_gennext(void *self);
// void fm_gennext(void* self, double* frame_vals, int framesPerBuffer);
void fm_status(void *self, char *status_string);
double fm_getvol(void *self);
void fm_setvol(void *self, double v);
void mfm(void *self, char *osc, double val);

#endif
