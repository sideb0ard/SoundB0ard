#include <stdlib.h>
#include <string.h>

#include "ddlmodule.h"
#include "defjams.h"
#include "utils.h"

void ddl_initialize(ddlmodule *d)
{
    d->m_delay_in_samples = 0;
    d->m_feedback = 0;
    d->m_delay_ms = 0;
    d->m_feedback_pct = 0;
    d->m_wet_level = 0;
    d->m_read_index = 0;
    d->m_write_index = 0;
    d->m_buffer = NULL; // TODO - destructor and deletion
    d->m_buffer_size = 0;

    ddl_cook_variables(d);

    d->m_buffer_size = 2 * SAMPLE_RATE;
    if (d->m_buffer)
        free(d->m_buffer);

    d->m_buffer = (double *)calloc(d->m_buffer_size, sizeof(double));
    ddl_reset_delay(d);
    ddl_cook_variables(d);

    printf("Cooked, mon!\n");
}

void ddl_cook_variables(ddlmodule *d)
{
    d->m_feedback = d->m_feedback_pct / 100.0;
    d->m_wet_level = d->m_wet_level_pct / 100.0;
    d->m_delay_in_samples = d->m_delay_ms * (SAMPLE_RATE / 1000.0);

    int precookidx = d->m_read_index;
    d->m_read_index = d->m_write_index - (int)d->m_delay_in_samples;

    if (d->m_read_index < 0)
        d->m_read_index += d->m_buffer_size;

    if (d->m_read_index >= d->m_buffer_size)
        printf("WHOA NELLIE! Before: %d After: %d Buffer Size: %d DelayMs: %f  "
               "Delay In Samples: %f\n",
               precookidx, d->m_read_index, d->m_buffer_size, d->m_delay_ms,
               d->m_delay_in_samples);
}

void ddl_reset_delay(ddlmodule *d)
{
    if (d->m_buffer)
        memset(d->m_buffer, 0, d->m_buffer_size * sizeof(double));
    d->m_write_index = 0;
    d->m_read_index = 0;
}

double ddl_get_current_feedback_output(ddlmodule *d)
{
    return d->m_feedback * d->m_buffer[d->m_read_index];
}

void ddl_set_current_feedback_input(ddlmodule *d, double f)
{
    d->m_feedback_in = f;
}

void ddl_set_uses_external_feedback(ddlmodule *d, bool b)
{
    d->m_use_external_feedback = b;
}

bool ddl_process_audio_frame(ddlmodule *d, double *input_buffer,
                             double *output_buffer,
                             unsigned int num_input_channels,
                             unsigned int num_output_channels)
{
    double xn = input_buffer[0];
    double yn = d->m_buffer[d->m_read_index];
    if (d->m_read_index == d->m_write_index && d->m_delay_in_samples < 1.0)
    {
        yn = xn;
    }

    int read_index_1 = d->m_read_index - 1;
    if (read_index_1 < 0)
        read_index_1 = d->m_buffer_size - 1;

    double yn_1 = d->m_buffer[read_index_1];

    double frac_delay = d->m_delay_in_samples - (int)d->m_delay_in_samples;

    double interp = lin_terp(0, 1, yn, yn_1, frac_delay);

    if (d->m_delay_in_samples == 0)
        yn = xn;
    else
        yn = interp;

    if (!d->m_use_external_feedback)
        d->m_buffer[d->m_write_index] = xn + d->m_feedback * yn; // norm
    else
        d->m_buffer[d->m_write_index] = xn + d->m_feedback_in;

    d->m_buffer[d->m_write_index] = xn + d->m_feedback * yn;

    output_buffer[0] = d->m_wet_level * yn + (1.0 - d->m_wet_level) * xn;

    d->m_write_index++;
    if (d->m_write_index >= d->m_buffer_size)
        d->m_write_index = 0;

    d->m_read_index++;
    if (d->m_read_index >= d->m_buffer_size)
    {
        d->m_read_index = 0;
    }

    if (num_input_channels == 1 && num_output_channels == 2)
        output_buffer[1] = output_buffer[0]; // copying mono

    if (num_input_channels == 2 && num_output_channels == 2)
        output_buffer[1] = output_buffer[0]; // copying mono

    return true;
}
