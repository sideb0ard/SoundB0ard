#pragma once

#include <stdbool.h>

typedef struct minisynth minisynth;

enum { UP, DOWN, UPDOWN, RANDOM, MAX_ARP_MODE} mode;
enum { SIXTEEN, EIGHT, FOUR, MAX_ARP_RATE } arp_rate;
enum { ROOT, THIRD, FIFTH, MAX_ARP_STEPS } arp_step;

typedef struct arpeggiator {
    bool active;
    bool latch;
    bool single_note_repeat;
    int octave_range;
    unsigned int mode;
    unsigned int rate;
    unsigned int cur_step;
    int last_midi_note;

} arpeggiator;

void arpeggiator_init(arpeggiator *arp);
void arpeggiate(minisynth *ms, arpeggiator *arp);
