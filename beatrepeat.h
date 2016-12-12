#ifndef BEATREPEAT_H_
#define BEATREPEAT_H_

#include <stdbool.h>

#include "effect.h"

typedef struct beatrepeat {
    EFFECT effect;
    double *m_buffer;
    size_t m_buffer_size;
    size_t m_buffer_position;
    size_t m_beat_size;
    unsigned int m_interval;
    unsigned int m_offset;
    unsigned int m_grid;
    unsigned int m_gate;
    bool m_recording;
    bool m_have_recording;
    bool m_active;
} beatrepeat;


double beatrepeat_gennext(beatrepeat *br, double input);

#endif // BEATREPEAT_H_
