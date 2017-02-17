#pragma once

#include "effect.h"

typedef enum { OCTAVE_CHANGE } custom_event_type;

typedef struct midi_event {
    unsigned tick;
    unsigned event_type;
    unsigned data1;
    unsigned data2;
} midi_event;

typedef midi_event *midi_events_loop_t[PPNS];

void *midiman(void *);

void midi_delay_control(EFFECT *e, int data1, int data2);

midi_event *new_midi_event(int tick, int event_type, int data1, int data2);
void print_midi_event_rec(midi_event *ev);
