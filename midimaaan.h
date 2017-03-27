#pragma once

#include "effect.h"

typedef struct minisynth minisynth;
typedef struct spork spork;

typedef enum { OCTAVE_CHANGE } custom_event_type;

typedef struct midi_event {
    unsigned tick;
    unsigned event_type;
    unsigned data1;
    unsigned data2;
    bool delete_after_use;
} midi_event;

typedef midi_event *midi_events_loop_t[PPNS];

void *midiman(void *);

void midi_delay_control(EFFECT *e, int data1, int data2);

midi_event *new_midi_event(int tick, int event_type, int data1, int data2);
void print_midi_event_rec(midi_event *ev);

void midi_parse_midi_event(minisynth *ms, midi_event *ev);

void midi_melody_quantize(midi_event **melody);

void spork_parse_midi(spork *s, int data1, int data2);
