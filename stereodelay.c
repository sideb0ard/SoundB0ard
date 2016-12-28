#include "stereodelay.h"
#include <stdlib.h>
#include <stdio.h>

stereodelay *new_stereo_delay()
{
    stereodelay *d = calloc(1, sizeof(stereodelay));
    d->m_delay_time_ms = 0;
    d->m_feedback_percent = 50;
    d->m_delay_ratio = 0;
    d->m_wet_mix = 0.0;

    return d;
}

void delay_reset(stereodelay *d)
{
    delayline_reset(&d->m_left_delay);
    delayline_reset(&d->m_right_delay);
}

void delay_prepare_for_play(stereodelay *d)
{
    // delay line memory allocated here
    delayline_init(&d->m_left_delay, 2.0 * SAMPLE_RATE);
    delayline_init(&d->m_right_delay, 2.0 * SAMPLE_RATE);
    delay_reset(d);
}

void delay_set_mode(stereodelay *d, unsigned mode) { d->m_mode = mode; }

void delay_set_delay_time_ms(stereodelay *d, double delay_ms)
{
    printf("Changing DELAY TIME TO %f\n", delay_ms);
    d->m_delay_time_ms = delay_ms;
    printf("NOW IT IS %f\n", d->m_delay_time_ms);
    //delay_update(d);
    printf("Aiight, back!\n");
}
void delay_set_feedback_percent(stereodelay *d, double feedback_percent)
{
    printf("Changing PERCENT TO %f\n", feedback_percent);
    d->m_feedback_percent = feedback_percent;
    printf("NOW IT IS %f\n", d->m_feedback_percent);
    //delay_update(d);
}
void delay_set_delay_ratio(stereodelay *d, double delay_ratio)
{
    printf("Changing RATIO TO %f\n", delay_ratio);
    d->m_delay_ratio = delay_ratio;
    printf("NOW IT IS %f\n", d->m_delay_ratio);
    //delay_update(d);
}
void delay_set_wet_mix(stereodelay *d, double wet_mix)
{
    printf("Changing WETMIX TO %f\n", wet_mix);
    d->m_wet_mix = wet_mix;
    printf("NOW IT IS %f\n", d->m_wet_mix);
    delay_update(d);
}

void delay_update(stereodelay *d)
{
    if (d->m_mode == TAP1 || d->m_mode == TAP2) {
        if (d->m_delay_ratio < 0) {
            d->m_tap2_left_delay_time_ms =
                -d->m_delay_ratio * d->m_delay_time_ms;
            d->m_tap2_right_delay_time_ms =
                (1.0 + d->m_delay_ratio) * d->m_delay_time_ms;
        }
        else if (d->m_delay_ratio > 0) {
            d->m_tap2_left_delay_time_ms =
                (1.0 - d->m_delay_ratio) * d->m_delay_time_ms;
            d->m_tap2_right_delay_time_ms =
                d->m_delay_ratio * d->m_delay_time_ms;
        }
        else {
            d->m_tap2_left_delay_time_ms = 0.0;
            d->m_tap2_right_delay_time_ms = 0.0;
        }
        delayline_set_delay_ms(&d->m_left_delay, d->m_delay_time_ms);
        delayline_set_delay_ms(&d->m_right_delay, d->m_delay_time_ms);

        return;
    }

    // else
    d->m_tap2_left_delay_time_ms = 0.0;
    d->m_tap2_right_delay_time_ms = 0.0;

    if (d->m_delay_ratio < 0) {
        delayline_set_delay_ms(&d->m_left_delay,
                               -d->m_delay_ratio * d->m_delay_time_ms);
        delayline_set_delay_ms(&d->m_right_delay, d->m_delay_time_ms);
    }
    else if (d->m_delay_ratio > 0) {
        delayline_set_delay_ms(&d->m_left_delay, d->m_delay_time_ms);
        delayline_set_delay_ms(&d->m_right_delay,
                               d->m_delay_ratio * d->m_delay_time_ms);
    }
    else {
        delayline_set_delay_ms(&d->m_left_delay, d->m_delay_time_ms);
        delayline_set_delay_ms(&d->m_right_delay, d->m_delay_time_ms);
    }
}

bool delay_process_audio(stereodelay *d, double *input_left,
                         double *input_right, double *output_left,
                         double *output_right)
{
    double left_delay_out = delayline_read_delay(&d->m_left_delay);
    double right_delay_out = delayline_read_delay(&d->m_right_delay);

    double left_delay_in =
        *input_left + left_delay_out * (d->m_feedback_percent / 100.0);
    double right_delay_in =
        *input_right + right_delay_out * (d->m_feedback_percent / 100.0);

    double left_tap2_out = 0.0;
    double right_tap2_out = 0.0;

    switch (d->m_mode) {
    case TAP1: {
        left_tap2_out = delayline_read_delay_at(&d->m_left_delay,
                                                d->m_tap2_left_delay_time_ms);
        right_tap2_out = delayline_read_delay_at(&d->m_right_delay,
                                                 d->m_tap2_right_delay_time_ms);
        break;
    }
    case TAP2: {
        left_tap2_out = delayline_read_delay_at(&d->m_left_delay,
                                                d->m_tap2_left_delay_time_ms);
        right_tap2_out = delayline_read_delay_at(&d->m_right_delay,
                                                 d->m_tap2_right_delay_time_ms);
        left_delay_in = *input_left +
                        (0.5 * left_delay_out + 0.5 * left_tap2_out) *
                            (d->m_feedback_percent / 100.0);
        right_delay_in = *input_right +
                         (0.5 * right_delay_out + 0.5 * right_tap2_out) *
                             (d->m_feedback_percent / 100.0);
        break;
    }
    case PINGPONG: {
        left_delay_in =
            *input_right + right_delay_out * (d->m_feedback_percent / 100.0);
        right_delay_in =
            *input_left + left_delay_out * (d->m_feedback_percent / 100.0);
        break;
    }
    }

    double left_out = 0.0;
    double right_out = 0.0;

    delayline_process_audio(&d->m_left_delay, &left_delay_in, &left_out);
    delayline_process_audio(&d->m_right_delay, &right_delay_in, &right_out);

    *output_left = *input_left * (1.0 - d->m_wet_mix) +
                   d->m_wet_mix * (left_out + left_tap2_out);
    *output_right = *input_right * (1.0 - d->m_wet_mix) +
                    d->m_wet_mix * (right_out + right_tap2_out);

    return true;
}
