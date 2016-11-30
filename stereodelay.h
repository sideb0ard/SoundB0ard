#pragma once

#include <stdbool.h>

#include "defjams.h"
#include "delayline.h"

typedef enum { NORM, TAP1, TAP2, PINGPONG } delay_mode;

typedef struct stereodelay {
    delayline m_left_delay, m_right_delay;
    double m_delay_time_ms;
    double m_feedback_percent;
    double m_delay_ratio;
    double m_wet_mix;
    unsigned m_mode;
    double m_tap2_left_delay_time_ms;
    double m_tap2_right_delay_time_ms;
} stereodelay;

stereodelay *new_stereo_delay(void);

void delay_set_mode(stereodelay *d, unsigned mode);
void delay_set_delay_time_ms(stereodelay *d, double delay_ms);
void delay_set_feedback_percent(stereodelay *d, double feedback_percent);
void delay_set_delay_ratio(stereodelay *d, double delay_ratio);
void delay_set_wet_mix(stereodelay *d, double wet_mix);

void delay_prepare_for_play(stereodelay *d);
void delay_reset(stereodelay *d);
void delay_update(stereodelay *d);
bool delay_process_audio(stereodelay *d, double *input_left,
                         double *input_right, double *output_left,
                         double *output_right);