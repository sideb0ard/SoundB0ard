#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "quad_flanger.h"

quad_flanger *new_quad_flanger()
{
    quad_flanger *qf = (quad_flanger *)calloc(1, sizeof(quad_flanger));
    if (!qf) // barf
        return NULL;

    // fx API
    qf->m_fx.type = QUADFLANGER;
    qf->m_fx.status = &quad_flanger_status;
    qf->m_fx.process = &quad_flanger_process_wrapper;

    // "gui"
    qf->m_mod_depth_pct = 50;    // percent
    qf->m_mod_freq = 0.18;       // range: 0.02 - 5
    qf->m_feedback_percent = 50; //  range: -100 - 100
    qf->m_lfo_type = 0; // TRI or SINE // these don't match other OSC enums

    mod_delay_init(&qf->m_moddelay_left);
    mod_delay_init(&qf->m_moddelay_right);
    quad_flanger_update_mod_delays(qf);

    return qf;
}

bool quad_flanger_update(quad_flanger *qf)
{
    quad_flanger_update_mod_delays(qf);
    return true;
}

bool quad_flanger_update_mod_delays(quad_flanger *qf)
{
    qf->m_moddelay_left.m_lfo_phase = 0;  // norm
    qf->m_moddelay_right.m_lfo_phase = 1; // quad
}

bool quad_flanger_process_audio(quad_flanger *qf, double *input_left,
                                double *input_right, double *output_left,
                                double *output_right)
{
    // TODO once i have stereo enabled, actually use the right moddelay
    mod_delay_process_audio(&qf->m_moddelay_left, input_left, input_right,
                            output_left, output_right);

    return true;
}

void quad_flanger_status(void *self, char *status_string)
{
    quad_flanger *qf = (quad_flanger *)self;
    snprintf(status_string, MAX_PS_STRING_SZ,
             "depth:%.2f rate:%.2f "
             "fb:%.2f lfo:%s ",
             qf->m_mod_depth_pct, qf->m_mod_freq, qf->m_feedback_percent,
             qf->m_lfo_type ? "SIN" : "TRI");
}

double quad_flanger_process_wrapper(void *self, double input)
{
    quad_flanger *qf = (quad_flanger *)self;
    double out = 0.;
    quad_flanger_process_audio(qf, &input, &input, &out, &out);
    return out;
}

void quad_flanger_set_depth(quad_flanger *qf, double val)
{
    if (val >= 0 && val <= 100)
        qf->m_mod_depth_pct = val;
    else
        printf("Val has to be between 0 and 100\n");
    quad_flanger_update(qf);
}

void quad_flanger_set_rate(quad_flanger *qf, double val)
{
    if (val >= 0.02 && val <= 5)
        qf->m_mod_freq = val;
    else
        printf("Val has to be between 0.02 and 5\n");
    quad_flanger_update(qf);
}

void quad_flanger_set_feedback_percent(quad_flanger *qf, double val)
{
    if (val >= -100 && val <= 100)
        qf->m_feedback_percent = val;
    else
        printf("Val has to be between -100 and 100\n");
    quad_flanger_update(qf);
}

void quad_flanger_set_lfo_type(quad_flanger *qf, unsigned int val)
{
    if (val < 2)
        qf->m_lfo_type = val;
    else
        printf("Val has to be 0 or 1\n");
    quad_flanger_update(qf);
}
