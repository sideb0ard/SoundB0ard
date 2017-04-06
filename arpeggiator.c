#include "arpeggiator.h"
#include "minisynth.h"
#include "mixer.h"

extern mixer *mixr;

#include <stdbool.h>

void arpeggiator_init(arpeggiator *arp)
{
    arp->active = false;
    arp->latch = true;
    arp->single_note_repeat = true;
    arp->octave_range = 2;
    arp->arp_mode = UP;
    arp->arp_rate = SIXTEEN;
}

void arpeggiate(minisynth *ms, arpeggiator *arp)
{
    if (mixr->is_sixteenth) {
        printf("Arp 16!\n");
    }
}
