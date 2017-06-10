#ifndef BEATREPEAT_H_
#define BEATREPEAT_H_

#include <stdbool.h>

#include "fx.h"

typedef struct beatrepeat {
    fx m_fx;
    double *m_buffer;
    size_t m_buffer_size;
    size_t m_buffer_position;
    size_t m_sixteenth_note_size;
    size_t m_selected_sixteenth;
    size_t m_num_beats_to_repeat;
    unsigned int m_interval;
    unsigned int m_offset;
    unsigned int m_grid;
    unsigned int m_gate;
    bool m_recording;
    bool m_have_recording;
    bool m_active;
} beatrepeat;

beatrepeat *new_beatrepeat(int nbeats, int sixteenth);
void beatrepeat_change_num_beats_to_repeat(beatrepeat *br, int num_beats);
void beatrepeat_change_selected_sixteenth(beatrepeat *br, int selected);

void beatrepeat_status(void *self, char *status_string);
double beatrepeat_gennext(void *self, double input);

#endif // BEATREPEAT_H_
