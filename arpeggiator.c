#include "arpeggiator.h"
#include "minisynth.h"
#include "mixer.h"

extern mixer *mixr;

#include <stdbool.h>

void arpeggiator_init(arpeggiator *arp)
{
    arp->active = false;
    arp->latch = true;
    arp->single_note_repeat = false;
    arp->octave_range = 2;
    arp->mode = UP;
    arp->rate = SIXTEEN;
    arp->cur_step = ROOT;
}

void arpeggiate(minisynth *ms, arpeggiator *arp)
{
    int note = 0;
    int velocity = 100;

    if (mixr->is_sixteenth) {
        if (arp->single_note_repeat) {
            note = ms->m_last_midi_note;
        }
        else {
            switch (arp->cur_step) {
            case (ROOT):
                note = ms->m_last_midi_note;
                break;
            case (THIRD):
                note = ms->m_last_midi_note + 4;
                break;
            case (FIFTH):
                note = ms->m_last_midi_note + 7;
                break;
            }
            arp->cur_step = (arp->cur_step + 1) % MAX_ARP_STEPS;
        }

        if (note > 8) {
            minisynth_handle_midi_note(ms, note, velocity, false);
        }
    }
}
