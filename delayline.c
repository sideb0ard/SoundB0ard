#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "delayline.h"
#include "utils.h"

void delayline_init(delayline *dl, int delay_length)
{
    dl->m_buffer_size = delay_length;

    if (dl->m_buffer)
        free(dl->m_buffer);

    dl->m_buffer = calloc(dl->m_buffer_size, sizeof(double));
}

void delayline_reset(delayline *dl)
{
    if (dl->m_buffer)
        memset(dl->m_buffer, 0, dl->m_buffer_size * sizeof(double));
    dl->m_write_index = 0;
    dl->m_read_index = 0;
    delayline_cook_variables(dl);
}

void delayline_set_delay_ms(delayline *dl, double delay_ms)
{
    dl->m_delay_ms = delay_ms;
    delayline_cook_variables(dl);
}

void delayline_cook_variables(delayline *dl)
{
    dl->m_delay_in_samples = dl->m_delay_ms * ((double)SAMPLE_RATE / 1000.0);
    dl->m_read_index = dl->m_write_index - (int)dl->m_delay_in_samples;
    if (dl->m_read_index < 0)
        dl->m_read_index += dl->m_buffer_size;
}

double delayline_read_delay(delayline *dl)
{
    double yn = dl->m_buffer[dl->m_read_index];

    int read_index_minus_one = dl->m_read_index - 1;
    if (read_index_minus_one < 0)
        read_index_minus_one = dl->m_buffer_size - 1;

    double yn_minus_one = dl->m_buffer[read_index_minus_one];
    double frac_delay = dl->m_delay_in_samples - (int)dl->m_delay_in_samples;
    return lin_terp(0, 1, yn, yn_minus_one, frac_delay);
}

double delayline_read_delay_at(delayline *dl, double ms)
{
    double delay_in_samples = ms * ((float)SAMPLE_RATE) / 1000.0;
    int read_index = dl->m_write_index - (int)delay_in_samples;

    if (read_index < 0)
        read_index += dl->m_buffer_size;

    double yn = dl->m_buffer[read_index];

    int read_index_minus_one = read_index - 1;
    if (read_index_minus_one < 0)
        read_index_minus_one = dl->m_buffer_size - 1;

    double yn_minus_one = dl->m_buffer[read_index_minus_one];
    double frac_delay = dl->m_delay_in_samples - (int)dl->m_delay_in_samples;
    return lin_terp(0, 1, yn, yn_minus_one, frac_delay);
}

void delayline_write_delay_and_inc(delayline *dl, double delay_input)
{
    dl->m_buffer[dl->m_write_index] = delay_input;

    dl->m_write_index++;
    if (dl->m_write_index >= dl->m_buffer_size)
        dl->m_write_index = 0;

    dl->m_read_index++;
    if (dl->m_read_index >= dl->m_buffer_size)
        dl->m_read_index = 0;
}

bool delayline_process_audio(delayline *dl, double *input, double *output)
{
    *output = dl->m_delay_in_samples == 0 ? *input : delayline_read_delay(dl);
    delayline_write_delay_and_inc(dl, *input);
    return true;
}
