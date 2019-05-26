#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"
#include "envelope.h"
#include "fx.h"

#include "SequenceEngine.h"

class SoundGenerator
{
  public:
    SoundGenerator();
    virtual ~SoundGenerator() = default;

    virtual stereo_val genNext() = 0;
    virtual void status(wchar_t *wstring) = 0;

    virtual void start();
    virtual void stop();

    virtual void noteOn(midi_event ev){};
    virtual void noteOff(midi_event ev){};
    virtual void control(midi_event ev){};
    virtual void pitchBend(midi_event ev){};
    virtual void randomize(){};
    virtual void allNotesOff(){};

    virtual void eventNotify(broadcast_event event, mixer_timing_info tinfo);

    void parseMidiEvent(midi_event ev, mixer_timing_info tinfo);

    void setVolume(double val);
    double getVolume();

    void setPan(double val);
    double getPan();

  public:
    sound_generator_type type;

    SequenceEngine engine;

    int mixer_idx;
    //  int num_patterns;
    bool active;

    double volume{0.7}; // between 0 and 1.0
    double pan{0.};     // between -1(hard left) and 1(hard right)

    int effects_size; // size of array
    int effects_num;  // num of effects
    fx **effects;
    int effects_on; // bool
};

bool is_synth(SoundGenerator *self);
bool is_stepper(SoundGenerator *self);
int add_beatrepeat_soundgen(SoundGenerator *self, int nbeats, int sixteenth);
int add_basicfilter_soundgen(SoundGenerator *self);
int add_bitcrush_soundgen(SoundGenerator *self);
int add_compressor_soundgen(SoundGenerator *self);
int add_distortion_soundgen(SoundGenerator *self);
int add_delay_soundgen(SoundGenerator *self, float duration);
int add_envelope_soundgen(SoundGenerator *self);
int add_moddelay_soundgen(SoundGenerator *self);
int add_modfilter_soundgen(SoundGenerator *self);
int add_follower_soundgen(SoundGenerator *self);
int add_reverb_soundgen(SoundGenerator *self);
int add_waveshape_soundgen(SoundGenerator *self);
stereo_val effector(SoundGenerator *self, stereo_val val);
