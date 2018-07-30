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

static int soundgen_add_fx(soundgenerator *self, fx *f)
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

int add_delay_soundgen(soundgenerator *self, float duration)
{
    printf("Booya, adding a new DELAY to soundgenerator: %f!\n", duration);
    stereodelay *sd = new_stereo_delay(duration);
    return soundgen_add_fx(self, (fx *)sd);
}

int add_reverb_soundgen(soundgenerator *self)
{
    printf("Booya, adding a new REVERB to soundgenerator!\n");
    reverb *r = new_reverb();
    return soundgen_add_fx(self, (fx *)r);
}

int add_waveshape_soundgen(soundgenerator *self)
{
    printf("WAVshape\n");
    waveshaper *ws = new_waveshaper();
    return soundgen_add_fx(self, (fx *)ws);
}

int add_basicfilter_soundgen(soundgenerator *self)
{
    printf("Fffuuuuhfilter!\n");
    filterpass *fp = new_filterpass();
    return soundgen_add_fx(self, (fx *)fp);
}

int add_bitcrush_soundgen(soundgenerator *self)
{
    printf("BITCRUSH!\n");
    bitcrush *bc = new_bitcrush();
    return soundgen_add_fx(self, (fx *)bc);
}

int add_compressor_soundgen(soundgenerator *self)
{
    printf("COMPresssssion!\n");
    dynamics_processor *dp = new_dynamics_processor();
    return soundgen_add_fx(self, (fx *)dp);
}

int add_follower_soundgen(soundgenerator *self)
{
    printf("RAWK! ENvelope Followerrr!\n");
    envelope_follower *ef = new_envelope_follower();
    return soundgen_add_fx(self, (fx *)ef);
}

int add_beatrepeat_soundgen(soundgenerator *self, int nbeats, int sixteenth)
{
    printf("RAR! BEATREPEAT all up in this kittycat\n");
    beatrepeat *b = new_beatrepeat(nbeats, sixteenth);
    return soundgen_add_fx(self, (fx *)b);
}

int add_moddelay_soundgen(soundgenerator *self)
{
    printf("Booya, adding a new MODDELAY to soundgenerator!\n");
    mod_delay *md = new_mod_delay();
    return soundgen_add_fx(self, (fx *)md);
}

int add_modfilter_soundgen(soundgenerator *self)
{
    printf("Booya, adding a new MODFILTERRRRR to soundgenerator!\n");
    modfilter *mf = new_modfilter();
    return soundgen_add_fx(self, (fx *)mf);
}

int add_distortion_soundgen(soundgenerator *self)
{
    printf("BOOYA! Distortion all up in this kittycat\n");
    distortion *d = new_distortion();
    return soundgen_add_fx(self, (fx *)d);
}

int add_envelope_soundgen(soundgenerator *self)
{
    printf("Booya, adding a new envelope to soundgenerator!\n");
    envelope *e = new_envelope();
    return soundgen_add_fx(self, (fx *)e);
}

double effector(soundgenerator *self, double val)
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

bool is_synth(soundgenerator *self)
{
    if (self->type == MINISYNTH_TYPE || self->type == DIGISYNTH_TYPE ||
        self->type == DXSYNTH_TYPE)
        return true;

    return false;
}

bool is_stepper(soundgenerator *self)
{
    if (self->type == SYNTHDRUM_TYPE || self->type == SEQUENCER_TYPE)
        return true;
    return false;
}
