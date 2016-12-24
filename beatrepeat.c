#include <stdlib.h>

#include "beatrepeat.h"
#include "mixer.h"

extern mixer *mixr;

double beatrepeat_gennext(beatrepeat *b, double inval)
{
    // if ( mixr->sixteenth_note_tick % 16 == b->m_selected_sixteenth
    if (mixr->sixteenth_note_tick % 16 == 0 && !b->m_recording &&
        !b->m_have_recording) {
        printf("Starting recording samples...\n");
        b->m_recording = true;
        printf("m_buffer_size = %lu\n", b->m_buffer_size);
    }

    if (b->m_recording) {
        b->m_buffer[b->m_buffer_position++] = inval;
        if (b->m_buffer_position >= b->m_buffer_size) {
            b->m_have_recording = true;
            b->m_recording = false;
            b->m_buffer_position = 0;
            printf("Finished recording - i gots a beat!\n");
        }
    }

    if (b->m_have_recording && b->m_active) {
        size_t relative_sample = mixr->cur_sample % mixr->loop_len_in_samples;
        size_t start_of_beat_repeat =
            ((b->m_selected_sixteenth + 1) * b->m_sixteenth_note_size) %
            mixr->loop_len_in_samples;
        size_t end_of_beat_repeat =
            (start_of_beat_repeat +
             (b->m_sixteenth_note_size * b->m_num_beats_to_repeat)) %
            mixr->loop_len_in_samples;

        if (end_of_beat_repeat < start_of_beat_repeat) {
            if ((relative_sample > start_of_beat_repeat &&
                 relative_sample < mixr->loop_len_in_samples) ||
                (relative_sample > 0 && relative_sample < end_of_beat_repeat)) {
                return inval +
                       b->m_buffer[relative_sample % b->m_sixteenth_note_size];
            }
        }
        else {
            if (relative_sample > start_of_beat_repeat &&
                relative_sample < end_of_beat_repeat) {
                // printf("pos %lu\n", relative_sample % b->m_buffer_size);
                // return inval + b->m_buffer[relative_sample %
                // b->m_buffer_size];
                return inval +
                       b->m_buffer[relative_sample % b->m_sixteenth_note_size];
            }
        }
    }

    return inval;
}

void beatrepeat_change_num_beats_to_repeat(beatrepeat *br, int num_beats)
{
    br->m_num_beats_to_repeat = num_beats;
}

void beatrepeat_change_selected_sixteenth(beatrepeat *br, int selected)
{
    br->m_selected_sixteenth = selected;
}
