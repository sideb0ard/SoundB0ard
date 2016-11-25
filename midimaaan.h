#pragma once

typedef struct nanosynth nanosynth;

typedef enum { NOTEON, NOTEOFF, PITCHBEND, CONTROL } midi_event_type;

typedef struct midi_event {
    unsigned tick;
    unsigned event_type;
    unsigned data1;
    unsigned data2;
} midi_event;

void *midiman(void *);

void midinoteon(nanosynth *ns, unsigned int midinote, int velocity);
void midinoteoff(nanosynth *ns, unsigned int midinote, int velocity);
void midipitchbend(nanosynth *ns, int data1, int data2);
void midicontrol(nanosynth *ns, int data1, int data2);

midi_event *new_midi_event(int tick, int event_type, int data1, int data2);
void print_midi_event_rec(midi_event *ev);
