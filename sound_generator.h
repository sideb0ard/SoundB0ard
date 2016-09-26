#ifndef SOUNDGEN_H
#define SOUNDGEN_H

#include <stdbool.h>

#include "defjams.h"
#include "effect.h"
#include "envelope.h"

typedef struct t_soundgen {
    // void (*gennext)(void *self, double* frame_vals, int framesPerBuffer);
    double (*gennext)(void *self);
    void (*status)(void *self, char *string);
    void (*setvol)(void *self, double val);
    double (*getvol)(void *self);
    sound_generator_type type;

    bool sidechain_on;
    int sidechain_input;
    double sidechain_amount; // percent

    int effects_size; // size of array
    int effects_num;  // num of effects
    EFFECT **effects;
    int effects_on; // bool

    int envelopes_size; // size of array
    int envelopes_num;  // num of effects
    ENVSTREAM **envelopes;
    int envelopes_on; // bool

} SOUNDGEN;

int add_distortion_soundgen(SOUNDGEN *self);
int add_decimator_soundgen(SOUNDGEN *self);
int add_delay_soundgen(SOUNDGEN *self, float duration, effect_type e_type);
int add_freq_pass_soundgen(SOUNDGEN *self, float freq, effect_type pass_type);
float effector(SOUNDGEN *self, float val);

int add_envelope_soundgen(SOUNDGEN *self, int env_len, int type);
float envelopor(SOUNDGEN *self, float val);

#endif // SOUNDGEN_H
