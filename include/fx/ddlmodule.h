#pragma once
#include <stdbool.h>

typedef struct ddlmodule
{
    double m_delay_in_samples;
    double m_feedback;
    double m_wet_level;

    double *m_buffer{nullptr};
    int m_read_index;
    int m_write_index;
    int m_buffer_size;

    bool m_use_external_feedback;
    double m_feedback_in;

    double m_delay_ms;
    double m_feedback_pct;
    double m_wet_level_pct;
} ddlmodule;

void ddl_initialize(ddlmodule *d);
bool ddl_process_audio_frame(ddlmodule *d, double *input_buffer,
                             double *output_buffer,
                             unsigned int num_input_channels,
                             unsigned int num_output_channels);
void ddl_update_ui(ddlmodule *d);
void ddl_cook_variables(ddlmodule *d);
void ddl_reset_delay(ddlmodule *d);

double ddl_get_current_feedback_output(ddlmodule *d);
void ddl_set_current_feedback_input(ddlmodule *d, double f);
void ddl_set_uses_external_feedback(ddlmodule *d, bool b);
