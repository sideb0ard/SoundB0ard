#pragma once
#include <stdbool.h>

typedef struct delayline
{
    double *m_buffer{nullptr};
    double m_delay_ms;
    double m_delay_in_samples;
    int m_read_index;
    int m_write_index;
    int m_buffer_size;
} delayline;

// delayline *delayline_new();
void delayline_init(delayline *dl, int delay_length);

void delayline_reset(delayline *dl);
void delayline_set_delay_ms(delayline *dl, double delay_ms);
void delayline_cook_variables(delayline *dl);
double delayline_read_delay(delayline *dl);
double delayline_read_delay_at(delayline *dl, double ms);
double delayline_read_delay_at_idx(delayline *dl, int idx);
void delayline_write_delay_and_inc(delayline *dl, double delay_input);
bool delayline_process_audio(delayline *dl, double *input, double *output);
