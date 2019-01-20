#pragma once

#include "defjams.h"
#include "fx.h"
#include "pthread.h"
#include "sound_generator.h"

typedef enum
{
    OCTAVE_CHANGE
} custom_event_type;

void *midi_init(void *);

midi_event new_midi_event(int event_type, int data1, int data2);
void midi_event_cp(midi_event *from, midi_event *to);
void midi_event_clear(midi_event *ev);

void midi_parse_midi_event(sound_generator *sg, midi_event *ev);

void midi_pattern_print(midi_event *pattern);
void midi_pattern_quantize(midi_pattern *pattern);
int get_midi_note_from_string(char *string);
int get_midi_note_from_mixer_key(unsigned int key, int octave);

void midi_pattern_set_velocity(midi_event *pattern, unsigned int midi_tick,
                               unsigned int velocity);
void midi_pattern_rand_amp(midi_event *pattern);
