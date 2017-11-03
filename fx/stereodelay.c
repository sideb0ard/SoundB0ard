#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "stereodelay.h"

stereodelay *new_stereo_delay(double duration)
{
    stereodelay *d = (stereodelay *)calloc(1, sizeof(stereodelay));

    d->m_delay_time_ms = duration;
    d->m_feedback_percent = 2;
    d->m_delay_ratio = 0.2;
    d->m_wet_mix = 0.7;
    d->m_mode = PINGPONG;

    d->m_fx.type = DELAY;
    d->m_fx.enabled = true;
    d->m_fx.status = &stereo_delay_status;
    d->m_fx.process = &stereo_delay_process_wrapper;

    stereo_delay_prepare_for_play(d);
    stereo_delay_update(d);

    return d;
}

void stereo_delay_reset(stereodelay *d)
{
    delayline_reset(&d->m_left_delay);
    delayline_reset(&d->m_right_delay);
}

void stereo_delay_prepare_for_play(stereodelay *d)
{
    // delay line memory allocated here
    delayline_init(&d->m_left_delay, 2.0 * SAMPLE_RATE);
    delayline_init(&d->m_right_delay, 2.0 * SAMPLE_RATE);
    stereo_delay_reset(d);
}

void stereo_delay_set_mode(stereodelay *d, unsigned mode)
{
    if (mode < MAX_NUM_DELAY_MODE)
        d->m_mode = mode;
    else
        printf("Delay mode must be between 0 and %d\n", MAX_NUM_DELAY_MODE);
}

void stereo_delay_set_delay_time_ms(stereodelay *d, double delay_ms)
{
    if (delay_ms >= 0 && delay_ms <= 2000)
    {
        d->m_delay_time_ms = delay_ms;
        stereo_delay_update(d);
    }
    else
        printf("Delay time ms must be between 0 and 2000\n");
}

void stereo_delay_set_feedback_percent(stereodelay *d, double feedback_percent)
{
    if (feedback_percent >= -100 && feedback_percent <= 100)
    {
        d->m_feedback_percent = feedback_percent;
        stereo_delay_update(d);
    }
    else
        printf("Feedback %% must be between -100 and 100\n");
}

void stereo_delay_set_delay_ratio(stereodelay *d, double delay_ratio)
{
    if (delay_ratio >= -1 && delay_ratio <= 1)
    {
        d->m_delay_ratio = delay_ratio;
        stereo_delay_update(d);
    }
    else
        printf("Delay ratio must be between -1 and 1\n");
}

void stereo_delay_set_wet_mix(stereodelay *d, double wet_mix)
{
    if (wet_mix >= 0 && wet_mix <= 100)
    {
        d->m_wet_mix = wet_mix;
        stereo_delay_update(d);
    }
    else
        printf("Wetmix must be between 0 and 100\n");
}

void stereo_delay_update(stereodelay *d)
{
    if (d->m_mode == TAP1 || d->m_mode == TAP2)
    {
        if (d->m_delay_ratio < 0)
        {
            d->m_tap2_left_delay_time_ms =
                -d->m_delay_ratio * d->m_delay_time_ms;
            d->m_tap2_right_delay_time_ms =
                (1.0 + d->m_delay_ratio) * d->m_delay_time_ms;
        }
        else if (d->m_delay_ratio > 0)
        {
            d->m_tap2_left_delay_time_ms =
                (1.0 - d->m_delay_ratio) * d->m_delay_time_ms;
            d->m_tap2_right_delay_time_ms =
                d->m_delay_ratio * d->m_delay_time_ms;
        }
        else
        {
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

    if (d->m_delay_ratio < 0)
    {
        delayline_set_delay_ms(&d->m_left_delay,
                               -d->m_delay_ratio * d->m_delay_time_ms);
        delayline_set_delay_ms(&d->m_right_delay, d->m_delay_time_ms);
    }
    else if (d->m_delay_ratio > 0)
    {
        delayline_set_delay_ms(&d->m_left_delay, d->m_delay_time_ms);
        delayline_set_delay_ms(&d->m_right_delay,
                               d->m_delay_ratio * d->m_delay_time_ms);
    }
    else
    {
        delayline_set_delay_ms(&d->m_left_delay, d->m_delay_time_ms);
        delayline_set_delay_ms(&d->m_right_delay, d->m_delay_time_ms);
    }
}

bool stereo_delay_process_audio(stereodelay *d, double *input_left,
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

    switch (d->m_mode)
    {
    case TAP1:
    {
        left_tap2_out = delayline_read_delay_at(&d->m_left_delay,
                                                d->m_tap2_left_delay_time_ms);
        right_tap2_out = delayline_read_delay_at(&d->m_right_delay,
                                                 d->m_tap2_right_delay_time_ms);
        break;
    }
    case TAP2:
    {
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
    case PINGPONG:
    {
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

void stereo_delay_status(void *self, char *status_string)
{
    stereodelay *sd = (stereodelay *)self;
    snprintf(status_string, MAX_PS_STRING_SZ, "delayms:%.2f fb:%.2f ratio:%.2f "
                                              "wetmx:%.2f mode:%d "
                                              "tap2left:%.2f tap2right:%.2f",
             sd->m_delay_time_ms, sd->m_feedback_percent, sd->m_delay_ratio,
             sd->m_wet_mix, sd->m_mode, sd->m_tap2_left_delay_time_ms,
             sd->m_tap2_right_delay_time_ms);
}

double stereo_delay_process_wrapper(void *self, double input)
{
    stereodelay *sd = (stereodelay *)self;
    double output = 0.;
    stereo_delay_process_audio(sd, &input, &input, &output, &output);
    return output;
}
