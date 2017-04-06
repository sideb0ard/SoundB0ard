#pragma once

#include <stdbool.h>

typedef struct minisynth minisynth;

enum arp_rate { SIXTEEN, EIGHTH, FOUR } arp_rate;
enum arp_step { ROOT, THIRD, FIFTH, MAX_STEPS };

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
