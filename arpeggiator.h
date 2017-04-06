#pragma once

#include <stdbool.h>

typedef struct minisynth minisynth;

enum arp_rate { SIXTEEN, EIGHTH, FOUR } arp_rate;

typedef struct arpeggiator {
    bool active;
    bool latch;
    bool single_note_repeat;
    int octave_range;
    unsigned int arp_mode;
    unsigned int arp_rate;

} arpeggiator;

void arpeggiator_init(arpeggiator *arp);
void arpeggiate(minisynth *ms, arpeggiator *arp);
