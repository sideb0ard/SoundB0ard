#include <stdlib.h>

#include "beatrepeat.h"
#include "mixer.h"

extern mixer *mixr;


double beatrepeat_gennext(beatrepeat *b, double inval)
{
    if ( mixr->sixteenth_note_tick % 16 == 0 
        && !b->m_recording && !b->m_have_recording)
    {
        printf("Starting recording samples...\n");
        b->m_recording = true;
        printf("m_buffer_size = %lu\n", b->m_buffer_size);
    }

    if (b->m_recording)
    {
        b->m_buffer[b->m_buffer_position++] = inval;
        if ( b->m_buffer_position >= b->m_buffer_size )
        {
            b->m_have_recording = true;
            b->m_recording = false;
            b->m_buffer_position = 0;
            printf("Finished recording - i gots a beat!\n");
        }
    }

    if (b->m_have_recording && b->m_active)
    {
        size_t relative_sample = mixr->cur_sample % mixr->loop_len_in_samples;
        if (relative_sample > b->m_buffer_size
            && relative_sample < (b->m_buffer_size * 7))
        {
            //printf("pos %lu\n", relative_sample % b->m_buffer_size);
            return inval + b->m_buffer[relative_sample % b->m_buffer_size];
        }
    }

    return inval;
}


