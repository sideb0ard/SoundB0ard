#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"
#include "envelope.h"
#include "fx.h"

typedef struct sound_generator
{
    stereo_val (*gennext)(void *self);
    void (*status)(void *self, wchar_t *wstring);
    void (*self_destruct)(void *self);

    void (*set_volume)(struct sound_generator *self, double val);
    double (*get_volume)(struct sound_generator *self);

    void (*set_pan)(struct sound_generator *self, double val);
    double (*get_pan)(struct sound_generator *self);

    void (*start)(void *self);
    void (*stop)(void *self);

    void (*event_notify)(void *self, broadcast_event event);

    sound_generator_type type;
    int mixer_idx;
    //  int num_patterns;
    bool active;

    double volume; // between 0 and 1.0
    double pan;    // between -1(hard left) and 1(hard right)

    int effects_size; // size of array
    int effects_num;  // num of effects
    fx **effects;
    int effects_on; // bool

} sound_generator;

void sound_generator_init(sound_generator *sg);

double sound_generator_get_volume(sound_generator *sg);
void sound_generator_set_volume(sound_generator *sg, double val);
double sound_generator_get_pan(sound_generator *sg);
void sound_generator_set_pan(sound_generator *sg, double val);

bool is_synth(sound_generator *self);
bool is_stepper(sound_generator *self);
int add_beatrepeat_soundgen(sound_generator *self, int nbeats, int sixteenth);
int add_basicfilter_soundgen(sound_generator *self);
int add_bitcrush_soundgen(sound_generator *self);
int add_compressor_soundgen(sound_generator *self);
int add_distortion_soundgen(sound_generator *self);
int add_delay_soundgen(sound_generator *self, float duration);
int add_envelope_soundgen(sound_generator *self);
int add_moddelay_soundgen(sound_generator *self);
int add_modfilter_soundgen(sound_generator *self);
int add_follower_soundgen(sound_generator *self);
int add_reverb_soundgen(sound_generator *self);
int add_waveshape_soundgen(sound_generator *self);
stereo_val effector(sound_generator *self, stereo_val val);
