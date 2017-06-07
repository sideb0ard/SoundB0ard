#ifndef SOUNDGEN_H
#define SOUNDGEN_H

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"
#include "effect.h"
#include "envelope.h"

typedef struct t_soundgen {
    // void (*gennext)(void *self, double* frame_vals, int framesPerBuffer);
    double (*gennext)(void *self);
    void (*status)(void *self, wchar_t *wstring);
    void (*setvol)(void *self, double val);
    double (*getvol)(void *self);
    void (*start)(void *self);
    void (*stop)(void *self);
    void (*make_active_track)(void *self, int track_num);
    int (*get_num_tracks)(void *self);

    sound_generator_type type;

    int effects_size; // size of array
    int effects_num;  // num of effects
    fx **effects;
    int effects_on; // bool

    int envelopes_size; // size of array
    int envelopes_num;  // num of effects
    ENVSTREAM **envelopes;
    int envelopes_enabled; // bool

} SOUNDGEN;

int add_beatrepeat_soundgen(SOUNDGEN *self, int looplen);
int add_distortion_soundgen(SOUNDGEN *self);
int add_decimator_soundgen(SOUNDGEN *self);
int add_delay_soundgen(SOUNDGEN *self, float duration);
int add_moddelay_soundgen(SOUNDGEN *self);
int add_modfilter_soundgen(SOUNDGEN *self);
int add_reverb_soundgen(SOUNDGEN *self);
int add_freq_pass_soundgen(SOUNDGEN *self, float freq, fx_type pass_type);
float effector(SOUNDGEN *self, double val);

int add_envelope_soundgen(SOUNDGEN *self, ENVSTREAM *e);
float envelopor(SOUNDGEN *self, float val);

#endif // SOUNDGEN_H
