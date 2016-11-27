#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "effect.h"
#include "sound_generator.h"

static int resize_effects_array(SOUNDGEN *self)
{

    EFFECT **new_effects = NULL;
    if (self->effects_size <= self->effects_num) {
        if (self->effects_size == 0) {
            self->effects_size = DEFAULT_ARRAY_SIZE;
        }
        else {
            self->effects_size *= 2;
        }

        new_effects =
            realloc(self->effects, self->effects_size * sizeof(EFFECT *));
        if (new_effects == NULL) {
            printf("Ooh, burney - cannae allocate memory for new effects");
            return -1;
        }
        else {
            self->effects = new_effects;
        }
    }
    return 0;
}

int add_decimator_soundgen(SOUNDGEN *self)
{
    printf("RAR! DECIMATOR all up in this kittycat\n");
    int res = resize_effects_array(self);
    if (res == -1) {
        perror("Couldn't resize effects array");
        return -1;
    }
    EFFECT *e = new_decimator();
    if (e == NULL) {
        perror("Couldn't create DECIMATOR effect");
        return -1;
    }
    self->effects[self->effects_num] = e;
    self->effects_on = 1;
    printf("done adding effect\n");
    return self->effects_num++;
}

int add_distortion_soundgen(SOUNDGEN *self)
{
    printf("BOOYA! Distortion all up in this kittycat\n");
    int res = resize_effects_array(self);
    if (res == -1) {
        perror("Couldn't resize effects array");
        return -1;
    }
    EFFECT *e = new_distortion();
    if (e == NULL) {
        perror("Couldn't create DISTORTion effect");
        return -1;
    }
    self->effects[self->effects_num] = e;
    self->effects_on = 1;
    printf("done adding effect\n");
    return self->effects_num++;
}

int add_delay_soundgen(SOUNDGEN *self, float duration)
{
    printf("Booya, adding a new DELAY to SOUNDGEN: %f!\n", duration);

    int res = resize_effects_array(self);
    if (res == -1) {
        perror("Couldn't resize effects array");
        return -1;
    }

    EFFECT *e = new_delay(duration);
    if (e == NULL) {
        perror("Couldn't create effect");
        return -1;
    }
    self->effects[self->effects_num] = e;
    self->effects_on = 1;
    printf("done adding effect\n");
    return self->effects_num++;
}

int add_freq_pass_soundgen(SOUNDGEN *self, float freq, effect_type pass_type)
{
    printf("Booya, adding a new *PASS to SOUNDGEN: %f!\n", freq);
    int res = resize_effects_array(self);
    if (res == -1) {
        perror("Couldn't resize effects array");
        return -1;
    }

    EFFECT *e = new_freq_pass(freq, pass_type);
    if (e == NULL) {
        perror("Couldn't create effect");
        return -1;
    }
    self->effects[self->effects_num] = e;
    self->effects_on = 1;
    printf("done adding effect\n");
    return self->effects_num++;
}

float effector(SOUNDGEN *self, float val)
{
    double left_in = val;
    double right_in = val;
    double left_out = 0.0;
    double right_out = 0.0;

    double val_copy = val;

    if (self->effects_on) {

        for (int i = 0; i < self->effects_num; i++) {

            int delay_p;
            double *delay;
            float val1 = 0;
            float val2 = 0;
            double loop_time = 0.04; // temp trial for reverb
            double reverb_time = 1.5;
            double decay = pow(0.001, loop_time / reverb_time);
            float mix = 0.1;
            float atten = 1.0;

            switch (self->effects[i]->type) {
            case DECIMATOR:
                // printf("INVAL %f\n", val);
                if (val > 0.0) {
                    self->effects[i]->cnt += self->effects[i]->rate;
                    val *= 2;
                    if (self->effects[i]->cnt >= 1) {
                        self->effects[i]->cnt -= 1;
                        val = (long)(val * self->effects[i]->m) /
                              (double)self->effects[i]->m;
                    }
                    // printf("OUTVAL %f\n", val);
                }
                break;
            case DISTORTION:
                // if ( val > 0.0 ) {
                //    val *= 2;
                //    val = (val / (1.0 + 0.28 * (val * val)));
                //}
                if (val > 0.0) {
                    val *= 2;
                    val = 1 / 100 * atan(val * 100);
                }
                break;
            case DELAY:
                // delay_update(self->effects[i]->delay);
                delay_process_audio(self->effects[i]->delay, &left_in,
                                    &right_in, &left_out, &right_out);
                val = left_out;
                break;
            case REVERB:
                delay_p = self->effects[i]->buf_p;
                delay = self->effects[i]->buffer;
                val = delay[delay_p] * decay;
                delay[delay_p++] = (val_copy * atten) + val;
                val = (val_copy * (1 - mix)) + (val * mix);
                if (delay_p >= self->effects[i]->buf_length)
                    delay_p = 0;
                self->effects[i]->buf_p = delay_p;
                break;
            case RES:
                delay_p = self->effects[i]->buf_p;
                delay = self->effects[i]->buffer;
                val = delay[delay_p];
                delay[delay_p++] = (val_copy + val) * 0.5;
                if (delay_p >= self->effects[i]->buf_length)
                    delay_p = 0;
                self->effects[i]->buf_p = delay_p;
                break;
            case ALLPASS:
                delay_p = self->effects[i]->buf_p;
                delay = self->effects[i]->buffer;
                val1 = delay[delay_p];
                val2 = val - (val1 * 0.5);
                delay[delay_p++] = val2;
                val = val1 + (val2 * 0.2);
                if (delay_p >= self->effects[i]->buf_length)
                    delay_p = 0;
                self->effects[i]->buf_p = delay_p;
                break;
            case LOWPASS:
                val = (val * (1 + self->effects[i]->coef) -
                       self->effects[i]->buffer[0] * self->effects[i]->coef);
                self->effects[i]->buffer[0] = val;
                break;
            case HIGHPASS:
                val = (val * (1 - self->effects[i]->coef) -
                       self->effects[i]->buffer[0] * self->effects[i]->coef);
                self->effects[i]->buffer[0] = val;
                break;
            case BANDPASS:
                val = (val * self->effects[i]->scal +
                       self->effects[i]->rr * self->effects[i]->costh *
                           self->effects[i]->buffer[0] -
                       self->effects[i]->rsq * self->effects[i]->buffer[1]);
                self->effects[i]->buffer[1] = self->effects[i]->buffer[0];
                self->effects[i]->buffer[0] = val;
                break;
            }
        }
    }
    return val;
}

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

        new_envelopes = realloc(self->envelopes,
                                self->envelopes_size * sizeof(ENVSTREAM *));
        if (new_envelopes == NULL) {
            printf("Ooh, burney - cannae allocate memory for new envelopes");
            return -1;
        }
        else {
            self->envelopes = new_envelopes;
        }
    }

    // ENVSTREAM *e = new_envelope_stream(env_len, type);
    // if (e == NULL) {
    //     perror("Couldn't create envelope");
    //     return -1;
    // }
    //
    self->envelopes[self->envelopes_num] = e;
    self->envelopes_enabled = 1;
    printf("done adding envelope\n");
    return self->envelopes_num++;
}

float envelopor(SOUNDGEN *self, float val)
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
