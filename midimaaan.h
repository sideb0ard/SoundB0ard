#pragma once

#include "defjams.h"
#include "fx.h"
#include "pthread.h"
#include <stdbool.h>

typedef struct minisynth minisynth;
typedef struct spork spork;

typedef enum { OCTAVE_CHANGE } custom_event_type;

typedef struct midi_event
{
    unsigned tick;
    unsigned event_type;
    unsigned data1;
    unsigned data2;
    bool delete_after_use;
    unsigned tick_off; // if this is a key on event, keep track of corresponding
                       // key off
} midi_event;

typedef midi_event *midi_events_loop[PPNS];

void *midiman(void *);

void midi_delay_control(fx *e, int data1, int data2);

midi_event *new_midi_event(int tick, int event_type, int data1, int data2);
void midi_event_free(midi_event *ms);
void print_midi_event_rec(midi_event *ev);

void midi_parse_midi_event(minisynth *ms, midi_event *ev);

void midi_melody_print(midi_event **melody);
void midi_melody_quantize(midi_event **melody);

void spork_parse_midi(spork *s, int data1, int data2);
