#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "beatrepeat.h"
#include "defjams.h"
#include "dynamics_processor.h"
#include "envelope_follower.h"
#include "fx.h"
#include "modfilter.h"
#include "modular_delay.h"
#include "reverb.h"
#include "sound_generator.h"
#include "stereodelay.h"
#include "waveshaper.h"

static int soundgen_add_fx(SOUNDGEN *self, fx *f)
{

    fx **new_effects = NULL;
    if (self->effects_size <= self->effects_num) {
        if (self->effects_size == 0) {
            self->effects_size = DEFAULT_ARRAY_SIZE;
        }
        else {
            self->effects_size *= 2;
        }

        new_effects =
            (fx **)realloc(self->effects, self->effects_size * sizeof(fx *));
        if (new_effects == NULL) {
            printf("Ooh, burney - cannae allocate memory for new effects");
            exit(1);
        }
        else {
            self->effects = new_effects;
        }
    }
    self->effects[self->effects_num] = f;
    self->effects_on = 1;

    printf("done adding effect\n");
    return self->effects_num++;
}

int add_delay_soundgen(SOUNDGEN *self, float duration)
{
    printf("Booya, adding a new DELAY to SOUNDGEN: %f!\n", duration);
    stereodelay *sd = new_stereo_delay(duration);
    return soundgen_add_fx(self, (fx *)sd);
}

int add_reverb_soundgen(SOUNDGEN *self)
{
    printf("Booya, adding a new REVERB to SOUNDGEN!\n");
    reverb *r = new_reverb();
    return soundgen_add_fx(self, (fx *)r);
}

int add_waveshape_soundgen(SOUNDGEN *self)
{
    printf("WAVshape\n");
    waveshaper *ws = new_waveshaper();
    return soundgen_add_fx(self, (fx *)ws);
}

int add_compressor_soundgen(SOUNDGEN *self)
{
    printf("COMPresssssion!\n");
    dynamics_processor *dp = new_dynamics_processor();
    return soundgen_add_fx(self, (fx *)dp);
}

int add_follower_soundgen(SOUNDGEN *self)
{
    printf("RAWK! ENvelope Followerrr!\n");
    envelope_follower *ef = new_envelope_follower();
    return soundgen_add_fx(self, (fx *)ef);
}
int add_beatrepeat_soundgen(SOUNDGEN *self, int nbeats, int sixteenth)
{
    printf("RAR! BEATREPEAT all up in this kittycat\n");
    beatrepeat *b = new_beatrepeat(nbeats, sixteenth);
    return soundgen_add_fx(self, (fx *)b);
}

int add_moddelay_soundgen(SOUNDGEN *self)
{
    printf("Booya, adding a new MODDELAY to SOUNDGEN!\n");
    mod_delay *md = new_mod_delay();
    return soundgen_add_fx(self, (fx *)md);
}

int add_modfilter_soundgen(SOUNDGEN *self)
{
    printf("Booya, adding a new MODFILTERRRRR to SOUNDGEN!\n");
    modfilter *mf = new_modfilter();
    return soundgen_add_fx(self, (fx *)mf);
}

int add_decimator_soundgen(SOUNDGEN *self)
{
    printf("RAR! DECIMATOR all up in this kittycat\n");
    // fx *e = new_decimator();
    // if (e == NULL) {
    //    perror("Couldn't create DECIMATOR effect");
    //    return -1;
    //}
    // self->effects[self->effects_num] = e;
    // self->effects_on = 1;
    // printf("done adding effect\n");
    // return self->effects_num++;
    return self->effects_num;
}

int add_distortion_soundgen(SOUNDGEN *self)
{
    printf("BOOYA! Distortion all up in this kittycat\n");
    // fx *e = new_distortion();
    // if (e == NULL) {
    //    perror("Couldn't create DISTORTion effect");
    //    return -1;
    //}
    // self->effects[self->effects_num] = e;
    // self->effects_on = 1;
    // printf("done adding effect\n");
    // return self->effects_num++;
    return self->effects_num;
}

int add_freq_pass_soundgen(SOUNDGEN *self, float freq, fx_type pass_type)
{
    printf("Booya, adding a new *PASS to SOUNDGEN: %f!\n", freq);
    (void)pass_type;

    // fx *e = new_freq_pass(freq, pass_type);
    // if (e == NULL) {
    //    perror("Couldn't create effect");
    //    return -1;
    //}
    // self->effects[self->effects_num] = e;
    // self->effects_on = 1;
    // printf("done adding effect\n");
    // return self->effects_num++;
    return self->effects_num;
}

double effector(SOUNDGEN *self, double val)
{
    for (int i = 0; i < self->effects_num; i++) {
        fx *f = self->effects[i];
        if (f->enabled) {
            val = f->process(f, val);
        }
    }

    return val;

    // case DECIMATOR:
    //     if (val > 0.0) {
    //         self->effects[i]->cnt += self->effects[i]->rate;
    //         val *= 2;
    //         if (self->effects[i]->cnt >= 1) {
    //             self->effects[i]->cnt -= 1;
    //             val = (long)(val * self->effects[i]->m) /
    //                   (double)self->effects[i]->m;
    //         }
    //     }
    //     break;
    // case DISTORTION:
    //     if (val > 0.0) {
    //         val *= 2;
    //         val = 1 / 100 * atan(val * 100);
    //     }
    //     break;
    // case RES:
    //     delay_p = self->effects[i]->buf_p;
    //     delay = self->effects[i]->buffer;
    //     val = delay[delay_p];
    //     delay[delay_p++] = (val_copy + val) * 0.5;
    //     if (delay_p >= self->effects[i]->buf_length)
    //         delay_p = 0;
    //     self->effects[i]->buf_p = delay_p;
    //     break;
    // case ALLPASS:
    //     delay_p = self->effects[i]->buf_p;
    //     delay = self->effects[i]->buffer;
    //     val1 = delay[delay_p];
    //     val2 = val - (val1 * 0.5);
    //     delay[delay_p++] = val2;
    //     val = val1 + (val2 * 0.2);
    //     if (delay_p >= self->effects[i]->buf_length)
    //         delay_p = 0;
    //     self->effects[i]->buf_p = delay_p;
    //     break;
    // case LOWPASS:
    //     val = (val * (1 + self->effects[i]->coef) -
    //            self->effects[i]->buffer[0] * self->effects[i]->coef);
    //     self->effects[i]->buffer[0] = val;
    //     break;
    // case HIGHPASS:
    //     val = (val * (1 - self->effects[i]->coef) -
    //            self->effects[i]->buffer[0] * self->effects[i]->coef);
    //     self->effects[i]->buffer[0] = val;
    //     break;
    // case BANDPASS:
    //     val = (val * self->effects[i]->scal +
    //            self->effects[i]->rr * self->effects[i]->costh *
    //                self->effects[i]->buffer[0] -
    //            self->effects[i]->rsq * self->effects[i]->buffer[1]);
    //     self->effects[i]->buffer[1] = self->effects[i]->buffer[0];
    //     self->effects[i]->buffer[0] = val;
    //     break;
    // }
}

//////////////////////////////////////////////////////

// int add_envelope_soundgen(SOUNDGEN *self, int env_len, int type)
int add_envelope_soundgen(SOUNDGEN *self, ENVSTREAM *e)
{
    printf("Booya, adding a new envelope to SOUNDGEN!\n");
    ENVSTREAM **new_envelopes = NULL;
    if (self->envelopes_size <= self->envelopes_num) {
        if (self->envelopes_size == 0) {
            self->envelopes_size = DEFAULT_ARRAY_SIZE;
        }
        else {
            self->envelopes_size *= 2;
        }

        new_envelopes = (ENVSTREAM **)realloc(
            self->envelopes, self->envelopes_size * sizeof(ENVSTREAM *));
        if (new_envelopes == NULL) {
            printf("Ooh, burney - cannae allocate memory for new envelopes");
            return -1;
        }
        else {
            self->envelopes = new_envelopes;
        }
    }

    self->envelopes[self->envelopes_num] = e;
    self->envelopes_enabled = 1;
    printf("done adding envelope\n");
    return self->envelopes_num++;
}

double envelopor(SOUNDGEN *self, double val)
{

    if (self->envelopes_num > 0 && self->envelopes_enabled) {
        for (int i = 0; i < self->envelopes_num; i++) {
            double mix_env = envelope_stream_tick(self->envelopes[i]);
            if (self->envelopes[i]->started) {
                val *= mix_env;
            }
            else if (mix_env == 1) {
                self->envelopes[i]->started = 1;
                val *= mix_env;
            }
        }
    }
    return val;
}
