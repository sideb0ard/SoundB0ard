#include <stdio.h>
#include <stdlib.h>

#include "modular_delay.h"
#include "utils.h"

static char *mod_type_as_string[] = {"FLANGER", "VIBRATO", "CHORUS"};

mod_delay *new_mod_delay()
{
    mod_delay *md = (mod_delay *)calloc(1, sizeof(mod_delay));
    if (!md) // barf
        return NULL;

    wt_initialize(&md->m_lfo);
    ddl_initialize(&md->m_ddl);

    // fx API
    md->m_fx.type = MODDELAY;
    md->m_fx.enabled = true;
    md->m_fx.status = &mod_delay_status;
    md->m_fx.process = &mod_delay_process_wrapper;

    md->m_min_delay_msec = 0.0;
    md->m_max_delay_msec = 0.0;
    md->m_chorus_offset = 0;

    md->m_ddl.m_use_external_feedback = false;
    md->m_ddl.m_delay_ms = 0.;

    md->m_lfo.polarity = 1;
    md->m_lfo.mode = 0;

    // "gui"
    md->m_mod_depth_pct = 50;   // percent
    md->m_mod_freq = 0.18;      // range: 0.02 - 5
    md->m_feedback_percent = 0; //  range: -100 - 100
    md->m_mod_type = FLANGER;   // FLANGER, VIBRATO, CHORUS
    md->m_lfo_type = 0; // TRI or SINE // these don't match other OSC enums

    mod_delay_update(md);
    wt_start(&md->m_lfo);

    return md;
}

bool mod_delay_update(mod_delay *md)
{
    mod_delay_cook_mod_type(md);
    mod_delay_update_lfo(md);
    mod_delay_update_ddl(md);
    return true;
}

void mod_delay_cook_mod_type(mod_delay *md)
{
    switch (md->m_mod_type)
    {
    case VIBRATO:
    {
        md->m_min_delay_msec = 0;
        md->m_max_delay_msec = 7;
        md->m_ddl.m_wet_level_pct = 100.0;
        md->m_ddl.m_feedback_pct = 0.0;
        break;
    }
    case CHORUS:
    {
        md->m_min_delay_msec = 5;
        md->m_max_delay_msec = 30;
        md->m_ddl.m_wet_level_pct = 50.0;
        md->m_ddl.m_feedback_pct = md->m_feedback_percent;
        break;
    }
    case FLANGER:
    default:
    {
        md->m_min_delay_msec = 0;
        md->m_max_delay_msec = 7;
        md->m_ddl.m_wet_level_pct = 50.0;
        md->m_ddl.m_feedback_pct = md->m_feedback_percent;
        break;
    }
    }
}

void mod_delay_update_lfo(mod_delay *md)
{
    md->m_lfo.freq = md->m_mod_freq;
    md->m_lfo.waveform =
        md->m_lfo_type == 0 ? 2 : 0; // tri or sine // dumb and mixed up
    wt_update(&md->m_lfo);
}

void mod_delay_update_ddl(mod_delay *md)
{
    if (md->m_mod_type != VIBRATO)
        md->m_ddl.m_feedback_pct = md->m_feedback_percent;
    ddl_cook_variables(&md->m_ddl);
}

double mod_delay_calculate_delay_offset(mod_delay *md, double lfo_sample)
{
    if (md->m_mod_type == FLANGER || md->m_mod_type == VIBRATO)
    {
        return (md->m_mod_depth_pct / 100.0) *
                   (lfo_sample *
                    (md->m_max_delay_msec - md->m_min_delay_msec)) +
               md->m_min_delay_msec;
    }
    else if (md->m_mod_type == CHORUS)
    {
        double start = md->m_min_delay_msec + md->m_chorus_offset;
        return (md->m_mod_depth_pct / 100.0) *
                   (lfo_sample *
                    (md->m_max_delay_msec - md->m_min_delay_msec)) +
               start;
    }
    return 0.0; // shouldn't happen
}

bool mod_delay_process_audio(mod_delay *md, double *input_left,
                             double *input_right, double *output_left,
                             double *output_right)
{
    (void)input_right;
    (void)output_right;

    double yn = 0;
    double yqn = 0;
    yn = wt_do_oscillate(&md->m_lfo, &yqn);
    // yn = scaleybum(-1.0, 1.0, 0, 1.0, yn);

    double delay = 0.0;
    // QUAD
    if (md->m_lfo_phase == 1) // quad
        delay = mod_delay_calculate_delay_offset(md, yn);
    else
        delay = mod_delay_calculate_delay_offset(md, yqn);

    md->m_ddl.m_delay_ms = delay;
    ddl_cook_variables(&md->m_ddl);

    ddl_process_audio_frame(&md->m_ddl, input_left, output_left, 1, 1);

    return true;
}

void mod_delay_status(void *self, char *status_string)
{
    mod_delay *md = (mod_delay *)self;
    snprintf(status_string, MAX_PS_STRING_SZ, "depth:%.2f rate:%.2f "
                                              "fb:%.2f offset:%.2f "
                                              "type:%s lfo:%s",
             md->m_mod_depth_pct, md->m_mod_freq, md->m_feedback_percent,
             md->m_chorus_offset, mod_type_as_string[md->m_mod_type],
             md->m_lfo_type ? "SIN" : "TRI");
}

double mod_delay_process_wrapper(void *self, double input)
{
    mod_delay *md = (mod_delay *)self;
    double out = 0.;
    mod_delay_process_audio(md, &input, &input, &out, &out);
    return out;
}

void mod_delay_set_depth(mod_delay *md, double val)
{
    if (val >= 0 && val <= 100)
        md->m_mod_depth_pct = val;
    else
        printf("Val has to be between 0 and 100\n");
    mod_delay_update(md);
}

void mod_delay_set_rate(mod_delay *md, double val)
{
    if (val >= 0.02 && val <= 5)
        md->m_mod_freq = val;
    else
        printf("Val has to be between 0.02 and 5\n");
    mod_delay_update(md);
}

void mod_delay_set_feedback_percent(mod_delay *md, double val)
{
    if (val >= -100 && val <= 100)
        md->m_feedback_percent = val;
    else
        printf("Val has to be between -100 and 100\n");
    mod_delay_update(md);
}

void mod_delay_set_chorus_offset(mod_delay *md, double val)
{
    if (val >= 0 && val <= 30)
        md->m_chorus_offset = val;
    else
        printf("Val has to be between 0 and 30\n");
    mod_delay_update(md);
}

void mod_delay_set_mod_type(mod_delay *md, unsigned int val)
{
    if (val < MAX_MOD_TYPE)
        md->m_mod_type = val;
    else
        printf("Val has to be between 0 and %d\n", MAX_MOD_TYPE - 1);
    mod_delay_update(md);
}

void mod_delay_set_lfo_type(mod_delay *md, unsigned int val)
{
    if (val < 2)
        md->m_lfo_type = val;
    else
        printf("Val has to be 0 or 1\n");
    mod_delay_update(md);
}
