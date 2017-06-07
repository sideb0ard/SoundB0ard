#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "beatrepeat.h"
#include "defjams.h"
#include "fx.h"
#include "mixer.h"
#include "modular_delay.h"
#include "utils.h"

extern mixer *mixr;

fx *effect_new_delay(double duration)
{

    stereodelay *sd = new_stereo_delay();
    printf("DurrrLAY! %f\n", duration);

    stereo_delay_prepare_for_play(sd);
    stereo_delay_set_delay_time_ms(sd, duration);
    stereo_delay_set_feedback_percent(sd, 2);
    stereo_delay_set_delay_ratio(sd, 0.2);
    stereo_delay_set_wet_mix(sd, 0.7);
    stereo_delay_set_mode(sd, PINGPONG);
    stereo_delay_update(sd);

    return (fx*) sd;
}

// fx *new_beatrepeat(int looplen)
// {
//     beatrepeat *b = (beatrepeat *)calloc(1, sizeof(beatrepeat));
//     if (b == NULL)
//         return NULL;
// 
//     printf("NEW BEAT REPEAT! %d loops\n", looplen);
//     b->m_buffer_size = mixr->loop_len_in_samples;
//     b->m_buffer = (double *)calloc(b->m_buffer_size, sizeof(double));
//     b->m_sixteenth_note_size = b->m_buffer_size / 16;
//     b->m_selected_sixteenth = 0;
//     b->m_num_beats_to_repeat = 6;
// 
//     b->effect.type = BEATREPEAT;
//     b->m_active = true;
// 
//     return (fx *)b;
// }
// 
// 
// fx *effect_new_mod_delay()
// {
//     fx *e;
//     e = (fx *)calloc(1, sizeof(fx));
//     if (e == NULL)
//         return NULL;
// 
//     e->moddelay = new_mod_delay();
//     mod_delay_prepare_for_play(e->moddelay);
//     e->type = MODDELAY;
//     printf("MOD DurrrLAY!\n");
// 
//     return e;
// }
// 
// fx *effect_new_mod_filter()
// {
//     fx *e;
//     e = (fx *)calloc(1, sizeof(fx));
//     if (e == NULL)
//         return NULL;
// 
//     e->modfilter = new_modfilter();
//     e->type = MODFILTER;
//     printf("MOD FILTER!\n");
// 
//     return e;
// }
// 
// fx *new_reverb_effect()
// {
//     fx *e;
//     e = (fx *)calloc(1, sizeof(fx));
//     if (e == NULL)
//         return NULL;
// 
//     e->type = REVERB;
// 
//     e->r = new_reverb();
//     printf("REVERB!\n");
// 
//     return e;
// }
// 
// fx *new_decimator()
// {
//     fx *e;
//     e = (fx *)calloc(1, sizeof(fx));
//     if (e == NULL)
//         return NULL;
//     e->type = DECIMATOR;
//     e->cnt = 0;
//     e->bits = 16;
//     e->rate = 0.5;
//     e->m = 1 << (e->bits - 1);
//     return e;
// }
// 
// fx *new_distortion()
// {
//     fx *e;
//     e = (fx *)calloc(1, sizeof(fx));
//     if (e == NULL)
//         return NULL;
// 
//     e->type = DISTORTION;
//     printf("Distortion added\n");
//     return e;
// }
// 
// fx *new_freq_pass(double freq, effect_type pass_type)
// {
//     fx *e;
//     e = (fx *)calloc(1, sizeof(fx));
//     if (e == NULL)
//         return NULL;
// 
//     e->freq = freq;
// 
//     double *buffer;
//     int buf_length = 2;
//     buffer = (double *)calloc(buf_length, sizeof(double));
//     if (buffer == NULL) {
//         perror("Couldn't allocate effect buffer");
//         free(e);
//         return NULL;
//     }
// 
//     // double costh;
//     double r, bw, rr, rsq, costh, scal;
//     switch (pass_type) {
//     case LOWPASS:
//         costh = 2. - cos(TWO_PI * freq / SAMPLE_RATE);
//         e->coef = sqrt(costh * costh - 1.) - costh;
//         e->type = LOWPASS;
//         e->costh = costh;
//         break;
//     case HIGHPASS:
//         costh = 2. - cos(TWO_PI * freq / SAMPLE_RATE);
//         e->coef = costh - sqrt(costh * costh - 1.);
//         e->type = HIGHPASS;
//         e->costh = costh;
//         break;
//     case BANDPASS:
//         bw = 50; // bandwidth - completely random number??! (noidea)
//         rr = 2 * (r = 1. - M_PI * (bw / SAMPLE_RATE));
//         rsq = r * r;
//         costh = (rr / (1. + rsq)) * cos(TWO_PI * freq / SAMPLE_RATE);
//         scal = (1 - rsq) * sin(acos(costh));
//         e->rr = rr;
//         e->rsq = rsq;
//         e->costh = costh;
//         e->scal = scal;
//         e->type = BANDPASS;
//         break;
//     default:
//         break;
//     }
// 
//     e->buffer = buffer;
//     e->buf_length = buf_length;
// 
//     return e;
// }

// void write_delay_and_inc(fx *self, double val)
// {
//     self->buffer[self->buf_write_idx] = val;
//     self->buf_write_idx++;
//     if (self->buf_write_idx >= self->buf_length)
//         self->buf_write_idx = 0;
//     self->buf_read_idx++;
//     if (self->buf_read_idx >= self->buf_length)
//         self->buf_read_idx = 0;
// }
// 
// double read_delay(fx *self)
// {
//     double yn = self->buffer[self->buf_read_idx];
//     int read_idx_1 = self->buf_read_idx - 1;
//     if (read_idx_1 < 0)
//         read_idx_1 = self->buf_length - 1;
//     double yn_1 = self->buffer[read_idx_1];
//     double frac_delay =
//         self->m_delay_in_samples - (int)self->m_delay_in_samples;
//     return scaleybum(0, 1, yn, yn_1, frac_delay);
// }
