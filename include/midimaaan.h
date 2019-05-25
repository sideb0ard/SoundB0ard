#pragma once

#include "SoundGenerator.h"
#include "defjams.h"
#include "fx.h"
#include "pthread.h"

typedef enum
{
    OCTAVE_CHANGE
} custom_event_type;

void *midi_init(void *);

midi_event new_midi_event(unsigned int event_type, unsigned int data1,
                          unsigned int data2);
void midi_event_cp(midi_event *from, midi_event *to);
void midi_event_clear(midi_event *ev);
void midi_event_print(midi_event *ev);

void midi_parse_midi_event(SoundGenerator *sg, midi_event *ev);

void midi_pattern_print(midi_event *pattern);
void midi_pattern_quantize(midi_pattern *pattern);
int get_midi_note_from_string(char *string);
int get_midi_note_from_mixer_key(unsigned int key, int octave);

void midi_pattern_set_velocity(midi_event *pattern, unsigned int midi_tick,
                               unsigned int velocity);
void midi_pattern_rand_amp(midi_event *pattern);
