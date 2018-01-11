#pragma once

#include "sound_generator.h"

typedef struct metronome
{
    soundgenerator sound_generator;

} metronome;

metronome *new_metronome(void);

stereo_val metronome_gennext(void *self);
void metronome_status(void *self, wchar_t *ss);
void metronome_setvol(void *self, double v);
double metronome_getvol(void *self);
void metronome_start(void *self);
void metronome_stop(void *self);
void metronome_make_active_track(void *self, int track_num);
int metronome_get_num_tracks(void *self);
void metronome_event_notify(void *self, unsigned int event_type);
void metronome_del_self(void *self);
