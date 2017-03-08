#include <stdlib.h>

#include "modular_delay.h"

mod_delay *new_mod_delay()
{
    mod_delay *md = calloc(1, sizeof(mod_delay));
    if (!md) // barf
        return NULL;

    md->m_lfo = lfo_new();
    if (!md->m_lfo)
        return NULL;

    md->m_delay = new_stereo_delay();
    if (!md->m_delay)
        return NULL;

    md->m_min_delay_msec = 0.0;
    md->m_max_delay_msec = 0.0;

    // "gui"
    md->m_depth = 50;
    md->m_rate = 0.18;
    md->m_feedback_percent = 0;
    md->m_chorus_offset = 0;
    md->m_mod_type = FLANGER;
    md->m_lfo_type = TRI;

    return md;
}

void mod_delay_update_lfo(mod_delay *md)
{
    md->m_lfo->osc.m_osc_fo = md->m_rate;
    md->m_lfo->osc.m_waveform = md->m_lfo_type;
    osc_update(&md->m_lfo->osc, "modular_delay");
}

void mod_delay_update_delay(mod_delay *md)
{
    if (md->m_mod_type != VIBRATO)
        md->m_delay->m_feedback_percent = md->m_feedback_percent;
    delay_update(md->m_delay);
}

void mod_delay_cook_mod_type(mod_delay *md)
{
    switch(md->m_mod_type) {
        case VIBRATO:
        {
            md->m_min_delay_msec = 0;
            md->m_max_delay_msec = 7;
            md->m_delay->m_wet_mix = 100.0;
            md->m_delay->m_feedback_percent = 0.0;
            break;
        }
        case CHORUS:
        {
            md->m_min_delay_msec = 5;
            md->m_max_delay_msec = 30;
            md->m_delay->m_wet_mix = 50.0;
            md->m_delay->m_feedback_percent = md->m_feedback_percent;
            break;
        }
        case FLANGER:
        default:
        {
            md->m_min_delay_msec = 0;
            md->m_max_delay_msec = 7;
            md->m_delay->m_wet_mix = 50.0;
            md->m_delay->m_feedback_percent = md->m_feedback_percent;
            break;
        }
    }
}

double mod_delay_calculate_delay_offset(mod_delay *md, double lfo_sample)
{
    if (md->m_mod_type == FLANGER || md->m_mod_type == VIBRATO)
    {
        return (md->m_depth / 100.0)*(lfo_sample*(md->m_max_delay_msec - md->m_min_delay_msec)) + md->m_min_delay_msec;
    }
    else if (md->m_mod_type == CHORUS)
    {
        double start = md->m_min_delay_msec + md->m_chorus_offset;
        return (md->m_depth / 100.0)*(lfo_sample*(md->m_max_delay_msec - md->m_min_delay_msec)) + start;
    }

    return 0.0; // shouldn't happen
}

bool mod_delay_prepare_for_play(mod_delay *md)
{
    lfo_start_oscillator((oscillator*)md->m_lfo);
    delay_prepare_for_play(md->m_delay);

    mod_delay_update(md);

    return true;

}

bool mod_delay_update(mod_delay *md)
{
    mod_delay_cook_mod_type(md);
    mod_delay_update_lfo(md);
    mod_delay_update_delay(md);
    return true;
}

bool mod_delay_process_audio(mod_delay *md, double *input_left,
                       double *input_right, double *output_left,
                       double *output_right)
{
    double yn = 0;
    double yqn = 0;
    yn = lfo_do_oscillate((oscillator*)md->m_lfo, &yqn);
    double delay = 0.0;
    // QUAD
    delay = mod_delay_calculate_delay_offset(md, yn);
    delay_set_delay_time_ms(md->m_delay, delay);
    mod_delay_update_delay(md);

    delay_process_audio(md->m_delay, input_left, input_right, output_left, output_right);

    return true;
}

