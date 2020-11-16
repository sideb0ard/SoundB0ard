#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <defjams.h>
#include <fx/basicfilterpass.h>
#include <fx/bitcrush.h>
#include <fx/distortion.h>
#include <fx/dynamics_processor.h>
#include <fx/fx.h>
#include <fx/modfilter.h>
#include <fx/modular_delay.h>
#include <fx/reverb.h>
#include <fx/stereodelay.h>
#include <fx/waveshaper.h>
#include <soundgenerator.h>

SoundGenerator::SoundGenerator(){};

// extern mixer *mixr;

double SoundGenerator::GetVolume() { return volume; }

void SoundGenerator::SetVolume(double val)
{
    if (val >= 0.0 && val <= 1.0)
        volume = val;
}

void SoundGenerator::start() { active = true; }
void SoundGenerator::stop() { active = false; }

void SoundGenerator::Load(std::string preset_name)
{
    std::cout << "BASE CLASS LOAD " << preset_name << " - NO OP!" << std::endl;
}

void SoundGenerator::Save(std::string preset_name)
{
    std::cout << "BASE CLASS SAVE " << preset_name << " - NO OP!" << std::endl;
}

std::string SoundGenerator::Status() { return std::string{"BASE CLASS, YO"}; }

void SoundGenerator::parseMidiEvent(midi_event ev, mixer_timing_info tinfo)
{

    int cur_midi_tick = tinfo.midi_tick % PPBAR;
    int midi_note = ev.data1;

    int midi_notes[3] = {midi_note, 0, 0};
    int midi_notes_len = 1; // default single note

    switch (ev.event_type)
    {
    case (MIDI_ON):
    { // Hex 0x80
        noteOn(ev);
        break;
    }
    case (MIDI_OFF):
    { // Hex 0x90
        noteOff(ev);
        break;
    }
    case (MIDI_CONTROL):
    { // Hex 0xB0
        control(ev);
        break;
    }
    case (MIDI_PITCHBEND):
    { // Hex 0xE0
        pitchBend(ev);
        break;
    }
    default:
        std::cout
            << "HERE PAL, I've NAE IDEA WHIT KIND OF MIDI EVENT THAT WiS! "
            << ev << std::endl;
    }

    if (ev.delete_after_use)
    {
        midi_event_clear(&ev);
    }
}

void SoundGenerator::eventNotify(broadcast_event event, mixer_timing_info tinfo)
{
    (void)event;
    int idx = tinfo.midi_tick % PPBAR;

    // this temporal_events table is my first pass at a solution to
    // ensure note off events still happen, even when i'm using the
    // above count_by which ends up not reaching note off events
    // sometimes.
    if (engine.temporal_events[idx].event_type)
    {
        midi_event ev = engine.temporal_events[idx];
        if (ev.event_type == MIDI_ON)
        {
            noteOn(ev);
        }
        else
        {
            noteOff(ev);
        }
        midi_event_clear(&engine.temporal_events[idx]);
    }

    if (!active)
        return;

    if (tinfo.is_start_of_loop)
        engine.started = true;

    if (engine.started)
    {
        int idx = ((int)(engine.cur_step * PPSIXTEENTH) +
                   (tinfo.midi_tick % PPSIXTEENTH)) %
                  PPBAR;

        if (idx < 0 || idx >= PPBAR)
            printf("YOUHC! idx out of bounds: %d\n", idx);

        if (engine.pattern[idx].event_type)
        {
            midi_event ev = engine.pattern[idx];
            parseMidiEvent(ev, tinfo);
        }

        if (tinfo.is_sixteenth)
        {
            engine.cur_step++;
            if (engine.cur_step == 16)
                engine.cur_step = 0;
        }
    }
}

void SoundGenerator::noteOffDelayed(midi_event ev, int event_off_tick)
{
    sequence_engine_add_temporal_event(&engine, event_off_tick, ev);
}

double SoundGenerator::GetPan() { return pan; }

void SoundGenerator::SetPan(double val)
{
    if (val >= -1.0 && val <= 1.0)
        pan = val;
}

int SoundGenerator::AddFx(Fx *f)
{

    if (effects_num < kMaxNumSoundGenFx)
    {
        effects[effects_num] = f;
        printf("done adding effect\n");
        return effects_num++;
    }

    return -1;
}

int SoundGenerator::AddDelay(float duration)
{
    printf("Booya, adding a new DELAY to "
           "SoundGenerator: %f!\n",
           duration);
    StereoDelay *sd = new StereoDelay(duration);
    return AddFx((Fx *)sd);
}

int SoundGenerator::AddReverb()
{
    printf("Booya, adding a new REVERB to "
           "SoundGenerator!\n");
    Reverb *r = new Reverb();
    return AddFx((Fx *)r);
}

int SoundGenerator::AddWaveshape()
{
    printf("WAVshape\n");
    WaveShaper *ws = new WaveShaper();
    return AddFx((Fx *)ws);
}

int SoundGenerator::AddBasicfilter()
{
    printf("Fffuuuuhfilter!\n");
    FilterPass *fp = new FilterPass();
    return AddFx((Fx *)fp);
}

int SoundGenerator::AddBitcrush()
{
    printf("BITCRUSH!\n");
    BitCrush *bc = new BitCrush();
    return AddFx((Fx *)bc);
}

int SoundGenerator::AddCompressor()
{
    printf("COMPresssssion!\n");
    DynamicsProcessor *dp = new DynamicsProcessor();
    return AddFx((Fx *)dp);
}

int SoundGenerator::AddModdelay()
{
    printf("Booya, adding a new MODDELAY to "
           "SoundGenerator!\n");
    ModDelay *md = new ModDelay();
    return AddFx((Fx *)md);
}

int SoundGenerator::AddModfilter()
{
    printf("Booya, adding a new MODFILTERRRRR to "
           "SoundGenerator!\n");
    ModFilter *mf = new ModFilter();
    return AddFx((Fx *)mf);
}

int SoundGenerator::AddDistortion()
{
    printf("BOOYA! Distortion all up in this kittycat\n");
    Distortion *d = new Distortion();
    return AddFx((Fx *)d);
}

int SoundGenerator::AddEnvelope()
{
    printf("Booya, adding a new envelope to "
           "SoundGenerator!\n");
    Envelope *e = new Envelope();
    return AddFx((Fx *)e);
}

stereo_val SoundGenerator::Effector(stereo_val val)
{
    int num_fx = effects_num.load();
    for (int i = 0; i < num_fx; i++)
    {
        Fx *f = effects[i];
        if (f && f->enabled_)
        {
            val = f->Process(val);
        }
    }
    return val;
}

bool SoundGenerator::IsSynth()
{
    if (type == MINISYNTH_TYPE || type == DIGISYNTH_TYPE ||
        type == DXSYNTH_TYPE)
        return true;

    return false;
}

bool SoundGenerator::IsStepper()
{
    if (type == DRUMSYNTH_TYPE || type == DRUMSAMPLER_TYPE)
        return true;
    return false;
}
