#pragma once

#include "defjams.h"
#include "fx.h"
#include "pthread.h"
#include "sound_generator.h"
#include <stdbool.h>

typedef enum { OCTAVE_CHANGE } custom_event_type;

void *midiman(void *);

midi_event new_midi_event(int event_type, int data1, int data2);
void midi_event_cp(midi_event *from, midi_event *to);
void midi_event_clear(midi_event *ev);

void midi_parse_midi_event(soundgenerator *sg, midi_event ev);

void midi_melody_print(midi_event *pattern);
void midi_melody_quantize(midi_pattern *melody);
int get_midi_note_from_string(char *string);
