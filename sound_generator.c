#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "basicfilterpass.h"
#include "beatrepeat.h"
#include "bitcrush.h"
#include "defjams.h"
#include "distortion.h"
#include "dynamics_processor.h"
#include "envelope_follower.h"
#include "fx.h"
#include "modfilter.h"
#include "modular_delay.h"
#include "reverb.h"
#include "sound_generator.h"
#include "stereodelay.h"
#include "waveshaper.h"

void sound_generator_init(sound_generator *sg)
{
    printf("SG INTIT!\n");
    sg->set_volume(sg, 0.7);
    sg->set_pan(sg, 0.0);
}

double sound_generator_get_volume(sound_generator *sg) { return sg->volume; }

void sound_generator_set_volume(sound_generator *sg, double val)
{
    if (val >= 0.0 && val <= 1.0)
        sg->volume = val;
}

double sound_generator_get_pan(sound_generator *sg) { return sg->pan; }

void sound_generator_set_pan(sound_generator *sg, double val)
{
    if (val >= -1.0 && val <= 1.0)
        sg->pan = val;
}

static int soundgen_add_fx(sound_generator *self, fx *f)
{

    fx **new_effects = NULL;
    if (self->effects_size <= self->effects_num)
    {
        if (self->effects_size == 0)
        {
            self->effects_size = DEFAULT_ARRAY_SIZE;
        }
        else
        {
            self->effects_size *= 2;
        }

        new_effects =
            (fx **)realloc(self->effects, self->effects_size * sizeof(fx *));
        if (new_effects == NULL)
        {
            printf("Ooh, burney - cannae allocate memory for new effects");
            exit(1);
        }
        else
        {
            self->effects = new_effects;
        }
    }
    self->effects[self->effects_num] = f;
    self->effects_on = 1;

    printf("done adding effect\n");
    return self->effects_num++;
}

int add_delay_soundgen(sound_generator *self, float duration)
{
    printf("Booya, adding a new DELAY to sound_generator: %f!\n", duration);
    stereodelay *sd = new_stereo_delay(duration);
    return soundgen_add_fx(self, (fx *)sd);
}

int add_reverb_soundgen(sound_generator *self)
{
    printf("Booya, adding a new REVERB to sound_generator!\n");
    reverb *r = new_reverb();
    return soundgen_add_fx(self, (fx *)r);
}

int add_waveshape_soundgen(sound_generator *self)
{
    printf("WAVshape\n");
    waveshaper *ws = new_waveshaper();
    return soundgen_add_fx(self, (fx *)ws);
}

int add_basicfilter_soundgen(sound_generator *self)
{
    printf("Fffuuuuhfilter!\n");
    filterpass *fp = new_filterpass();
    return soundgen_add_fx(self, (fx *)fp);
}

int add_bitcrush_soundgen(sound_generator *self)
{
    printf("BITCRUSH!\n");
    bitcrush *bc = new_bitcrush();
    return soundgen_add_fx(self, (fx *)bc);
}

int add_compressor_soundgen(sound_generator *self)
{
    printf("COMPresssssion!\n");
    dynamics_processor *dp = new_dynamics_processor();
    return soundgen_add_fx(self, (fx *)dp);
}

int add_follower_soundgen(sound_generator *self)
{
    printf("RAWK! ENvelope Followerrr!\n");
    envelope_follower *ef = new_envelope_follower();
    return soundgen_add_fx(self, (fx *)ef);
}

int add_beatrepeat_soundgen(sound_generator *self, int nbeats, int sixteenth)
{
    printf("RAR! BEATREPEAT all up in this kittycat\n");
    beatrepeat *b = new_beatrepeat(nbeats, sixteenth);
    return soundgen_add_fx(self, (fx *)b);
}

int add_moddelay_soundgen(sound_generator *self)
{
    printf("Booya, adding a new MODDELAY to sound_generator!\n");
    mod_delay *md = new_mod_delay();
    return soundgen_add_fx(self, (fx *)md);
}

int add_modfilter_soundgen(sound_generator *self)
{
    printf("Booya, adding a new MODFILTERRRRR to sound_generator!\n");
    modfilter *mf = new_modfilter();
    return soundgen_add_fx(self, (fx *)mf);
}

int add_distortion_soundgen(sound_generator *self)
{
    printf("BOOYA! Distortion all up in this kittycat\n");
    distortion *d = new_distortion();
    return soundgen_add_fx(self, (fx *)d);
}

int add_envelope_soundgen(sound_generator *self)
{
    printf("Booya, adding a new envelope to sound_generator!\n");
    envelope *e = new_envelope();
    return soundgen_add_fx(self, (fx *)e);
}

stereo_val effector(sound_generator *self, stereo_val val)
{
    for (int i = 0; i < self->effects_num; i++)
    {
        fx *f = self->effects[i];
        if (f->enabled)
        {
            val = f->process(f, val);
        }
    }
    return val;
}

bool is_synth(sound_generator *self)
{
    if (self->type == MINISYNTH_TYPE || self->type == DIGISYNTH_TYPE ||
        self->type == DXSYNTH_TYPE)
        return true;

    return false;
}

bool is_stepper(sound_generator *self)
{
    if (self->type == DRUMSYNTH_TYPE || self->type == DRUMSAMPLER_TYPE)
        return true;
    return false;
}
