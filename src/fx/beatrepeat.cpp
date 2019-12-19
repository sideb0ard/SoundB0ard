#include <stdlib.h>

#include "beatrepeat.h"
#include "mixer.h"

extern mixer *mixr;

beatrepeat *new_beatrepeat(int nbeats, int sixteenth)
{
    printf("NEW BEAT REPEAT! nbeats: %d sixteenth:%d\n", nbeats, sixteenth);
    beatrepeat *b = (beatrepeat *)calloc(1, sizeof(beatrepeat));

    b->m_fx.type = BEATREPEAT;
    b->m_fx.enabled = true;
    b->m_fx.status = &beatrepeat_status;
    b->m_fx.process = &beatrepeat_gennext;
    b->m_fx.event_notify = &fx_noop_event_notify;

    // TODO - this doesn't handle a change of BPM
    b->m_buffer_size = mixr->timing_info.loop_len_in_frames;
    b->m_buffer = (double *)calloc(b->m_buffer_size, sizeof(double));
    b->m_sixteenth_note_size = b->m_buffer_size / 16;
    b->m_num_beats_to_repeat = nbeats;
    b->m_selected_sixteenth = sixteenth;

    b->m_active = true;

    return b;
}

void beatrepeat_status(void *self, char *status_string)
{
    beatrepeat *b = (beatrepeat *)self;
    snprintf(status_string, MAX_STATIC_STRING_SZ, "numbeats:%zu sixteenth:%zu",
             b->m_num_beats_to_repeat, b->m_selected_sixteenth);
}

stereo_val beatrepeat_gennext(void *self, stereo_val inval)
{
    beatrepeat *b = (beatrepeat *)self;

    // if ( mixr->sixteenth_note_tick % 16 == b->m_selected_sixteenth
    if (mixr->timing_info.sixteenth_note_tick % 16 == 0 && !b->m_recording &&
        !b->m_have_recording)
    {
        printf("Starting recording samples...\n");
        b->m_recording = true;
        printf("m_buffer_size = %lu\n", b->m_buffer_size);
    }

    if (b->m_recording)
    {
        b->m_buffer[b->m_buffer_position++] = inval.left;
        if (b->m_buffer_position >= b->m_buffer_size)
        {
            b->m_have_recording = true;
            b->m_recording = false;
            b->m_buffer_position = 0;
            printf("Finished recording - i gots a beat!\n");
        }
    }

    if (b->m_have_recording && b->m_active)
    {
        size_t relative_sample =
            mixr->timing_info.cur_sample % mixr->timing_info.loop_len_in_frames;
        size_t start_of_beat_repeat =
            ((b->m_selected_sixteenth + 1) * b->m_sixteenth_note_size) %
            mixr->timing_info.loop_len_in_frames;
        size_t end_of_beat_repeat =
            (start_of_beat_repeat +
             (b->m_sixteenth_note_size * b->m_num_beats_to_repeat)) %
            mixr->timing_info.loop_len_in_frames;

        if (end_of_beat_repeat < start_of_beat_repeat)
        {
            if ((relative_sample > start_of_beat_repeat &&
                 relative_sample < mixr->timing_info.loop_len_in_frames) ||
                (relative_sample > 0 && relative_sample < end_of_beat_repeat))
            {
                inval.left +=
                    b->m_buffer[relative_sample % b->m_sixteenth_note_size];
                inval.right = inval.left;
            }
        }
        else
        {
            if (relative_sample > start_of_beat_repeat &&
                relative_sample < end_of_beat_repeat)
            {
                // printf("pos %lu\n", relative_sample %
                // b->m_buffer_size);
                // return inval + b->m_buffer[relative_sample %
                // b->m_buffer_size];
                inval.left +=
                    b->m_buffer[relative_sample % b->m_sixteenth_note_size];
                inval.right = inval.left;
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
