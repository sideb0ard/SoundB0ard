#include "arpeggiator.h"
#include "minisynth.h"
#include "mixer.h"
#include <stdlib.h>

extern mixer *mixr;

#include <stdbool.h>

void arpeggiator_init(arpeggiator *arp)
{
    arp->active = false;
    arp->latch = true;
    arp->single_note_repeat = false;
    arp->cur_octave = 0;
    arp->octave_range = 1;
    arp->mode = RANDOM;
    arp->direction = UP;
    arp->rate = ASIXTEENTH;
    arp->cur_step = ROOT;
}

void arpeggiate(minisynth *ms, arpeggiator *arp)
{
    int note = 0;
    int velocity = 100;

    if ((mixr->timing_info.is_thirtysecond && arp->rate == ATHIRTYSECOND) ||
        (mixr->timing_info.is_sixteenth && arp->rate == ASIXTEENTH) ||
        (mixr->timing_info.is_eighth && arp->rate == AEIGHTH) ||
        (mixr->timing_info.is_quarter && arp->rate == AQUARTER))
    {
        if (arp->single_note_repeat)
        {
            note = ms->m_last_midi_notes[MAX_VOICES - 1];
        }
        else
        {
            switch (arp->cur_step)
            {
            case (ROOT):
                note = ms->m_last_midi_notes[MAX_VOICES - 1];
                break;
            case (THIRD):
                note = ms->m_last_midi_notes[MAX_VOICES - 1] + 4;
                break;
            case (FIFTH):
                note = ms->m_last_midi_notes[MAX_VOICES - 1] + 7;
                break;
            }
        }

        if (note > 8)
        {
            synth_handle_midi_note((soundgenerator *)ms,
                                   note + (12 * arp->cur_octave), velocity,
                                   false);
        }
        else
            printf("IGNORING NOTe:%d\n", note);

        switch (arp->mode)
        {
        case (UP):
            if (arp->cur_step == FIFTH)
                arp->cur_octave = (arp->cur_octave + 1) % arp->octave_range;

            arp->cur_step = (arp->cur_step + 1) % MAX_ARP_STEPS;
            break;
        case (DOWN):
            if (arp->cur_step == ROOT)
            {
                arp->cur_octave--;
                if (arp->cur_octave < 0)
                    arp->cur_octave = arp->octave_range - 1;
                arp->cur_step = MAX_ARP_STEPS - 1;
            }
            else
                arp->cur_step--;
            break;
        case (UPDOWN):
            if (arp->direction == UP)
            {
                if (arp->cur_step == FIFTH)
                {
                    if (arp->cur_octave == arp->octave_range - 1)
                        arp->direction = DOWN;
                    else
                        arp->cur_octave =
                            (arp->cur_octave + 1) % arp->octave_range;
                }
                arp->cur_step = (arp->cur_step + 1) % MAX_ARP_STEPS;
            }
            else
            {
                if (arp->cur_step == ROOT)
                {
                    arp->cur_octave--;
                    if (arp->cur_octave < 0)
                    {
                        arp->cur_octave = 0;
                        arp->direction = UP;
                    }
                    arp->cur_step = MAX_ARP_STEPS - 1;
                }
                else
                    arp->cur_step--;
            }
            break;
        case (RANDOM):
            arp->cur_octave = rand() % arp->octave_range;
            arp->cur_step = rand() % MAX_ARP_STEPS;
            break;
        }
    }
}
