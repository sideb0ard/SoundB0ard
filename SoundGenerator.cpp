#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "SoundGenerator.h"
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
#include "stereodelay.h"
#include "waveshaper.h"

double SoundGenerator::getVolume() { return volume; }

void SoundGenerator::setVolume(double val)
{
    if (val >= 0.0 && val <= 1.0)
        volume = val;
}

double SoundGenerator::getPan() { return pan; }

void SoundGenerator::setPan(double val)
{
    if (val >= -1.0 && val <= 1.0)
        pan = val;
}

static int soundgen_add_fx(SoundGenerator *self, fx *f)
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

int add_delay_soundgen(SoundGenerator *self, float duration)
{
    printf("Booya, adding a new DELAY to SoundGenerator: %f!\n", duration);
    stereodelay *sd = new_stereo_delay(duration);
    return soundgen_add_fx(self, (fx *)sd);
}

int add_reverb_soundgen(SoundGenerator *self)
{
    printf("Booya, adding a new REVERB to SoundGenerator!\n");
    reverb *r = new_reverb();
    return soundgen_add_fx(self, (fx *)r);
}

int add_waveshape_soundgen(SoundGenerator *self)
{
    printf("WAVshape\n");
    waveshaper *ws = new_waveshaper();
    return soundgen_add_fx(self, (fx *)ws);
}

int add_basicfilter_soundgen(SoundGenerator *self)
{
    printf("Fffuuuuhfilter!\n");
    filterpass *fp = new_filterpass();
    return soundgen_add_fx(self, (fx *)fp);
}

int add_bitcrush_soundgen(SoundGenerator *self)
{
    printf("BITCRUSH!\n");
    bitcrush *bc = new_bitcrush();
    return soundgen_add_fx(self, (fx *)bc);
}

int add_compressor_soundgen(SoundGenerator *self)
{
    printf("COMPresssssion!\n");
    dynamics_processor *dp = new_dynamics_processor();
    return soundgen_add_fx(self, (fx *)dp);
}

int add_follower_soundgen(SoundGenerator *self)
{
    printf("RAWK! ENvelope Followerrr!\n");
    envelope_follower *ef = new_envelope_follower();
    return soundgen_add_fx(self, (fx *)ef);
}

int add_beatrepeat_soundgen(SoundGenerator *self, int nbeats, int sixteenth)
{
    printf("RAR! BEATREPEAT all up in this kittycat\n");
    beatrepeat *b = new_beatrepeat(nbeats, sixteenth);
    return soundgen_add_fx(self, (fx *)b);
}

int add_moddelay_soundgen(SoundGenerator *self)
{
    printf("Booya, adding a new MODDELAY to SoundGenerator!\n");
    mod_delay *md = new_mod_delay();
    return soundgen_add_fx(self, (fx *)md);
}

int add_modfilter_soundgen(SoundGenerator *self)
{
    printf("Booya, adding a new MODFILTERRRRR to SoundGenerator!\n");
    modfilter *mf = new_modfilter();
    return soundgen_add_fx(self, (fx *)mf);
}

int add_distortion_soundgen(SoundGenerator *self)
{
    printf("BOOYA! Distortion all up in this kittycat\n");
    distortion *d = new_distortion();
    return soundgen_add_fx(self, (fx *)d);
}

int add_envelope_soundgen(SoundGenerator *self)
{
    printf("Booya, adding a new envelope to SoundGenerator!\n");
    envelope *e = new_envelope();
    return soundgen_add_fx(self, (fx *)e);
}

stereo_val effector(SoundGenerator *self, stereo_val val)
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

bool is_synth(SoundGenerator *self)
{
    if (self->type == MINISYNTH_TYPE || self->type == DIGISYNTH_TYPE ||
        self->type == DXSYNTH_TYPE)
        return true;

    return false;
}

bool is_stepper(SoundGenerator *self)
{
    if (self->type == DRUMSYNTH_TYPE || self->type == DRUMSAMPLER_TYPE)
        return true;
    return false;
}
