#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"
#include "envelope.h"
#include "fx.h"

typedef struct soundgenerator
{
    stereo_val (*gennext)(void *self);
    void (*status)(void *self, wchar_t *wstring);
    void (*setvol)(void *self, double val);
    double (*getvol)(void *self);
    void (*start)(void *self);
    void (*stop)(void *self);
    void (*make_active_track)(void *self, int track_num);
    int (*get_num_patterns)(void *self);
    void (*set_num_patterns)(void *self, int num_patterns);
    void (*self_destruct)(void *self);
    void (*event_notify)(void *self, unsigned int event_type);
    midi_event *(*get_pattern)(void *self, int pattern_num);
    void (*set_pattern)(void *self, int pattern_num, midi_event *pattern);
    bool (*is_valid_pattern)(void *self, int pattern_num);

    sound_generator_type type;
    int num_patterns;
    bool active;

    int effects_size; // size of array
    int effects_num;  // num of effects
    fx **effects;
    int effects_on; // bool

} soundgenerator;

bool is_synth(soundgenerator *self);
bool is_stepper(soundgenerator *self);
int add_beatrepeat_soundgen(soundgenerator *self, int nbeats, int sixteenth);
int add_basicfilter_soundgen(soundgenerator *self);
int add_bitcrush_soundgen(soundgenerator *self);
int add_compressor_soundgen(soundgenerator *self);
int add_distortion_soundgen(soundgenerator *self);
int add_delay_soundgen(soundgenerator *self, float duration);
int add_envelope_soundgen(soundgenerator *self);
int add_moddelay_soundgen(soundgenerator *self);
int add_modfilter_soundgen(soundgenerator *self);
int add_follower_soundgen(soundgenerator *self);
int add_reverb_soundgen(soundgenerator *self);
int add_waveshape_soundgen(soundgenerator *self);
double effector(soundgenerator *self, double val);
