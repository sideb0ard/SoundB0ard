#pragma once

#include "defjams.h"
#include "fx.h"
#include "pthread.h"
#include "sound_generator.h"
#include <stdbool.h>

typedef enum { OCTAVE_CHANGE } custom_event_type;

typedef struct midi_event
{
    int tick; // set to -1 if event is not in use
    unsigned event_type;
    unsigned data1;
    unsigned data2;
    bool delete_after_use;
    unsigned tick_off; // if this is a key on event, keep track of corresponding
                       // key off
} midi_event;

typedef midi_event midi_events_loop[PPNS];

void *midiman(void *);

midi_event new_blank_midi_event(void);
midi_event new_midi_event(int tick, int event_type, int data1, int data2);
void midi_event_clear(midi_events_loop melody, int tick_to_clear);
void print_midi_event_rec(midi_event ev);

void midi_parse_midi_event(soundgenerator *sg, midi_event ev);

void midi_melody_print(midi_events_loop *melody);
void midi_melody_quantize(midi_events_loop *melody);
