#pragma once

#include <stdbool.h>
#include <wchar.h>

#include <atomic>

#include <defjams.h>
#include <fx/envelope.h>
#include <fx/fx.h>
#include <sequenceengine.h>

class SoundGenerator
{
  public:
    SoundGenerator();
    virtual ~SoundGenerator() = default;

    virtual stereo_val genNext() = 0;

    virtual std::string Info() = 0;
    virtual std::string Status() = 0;

    virtual void SetParam(std::string name, double val) = 0;
    virtual double GetParam(std::string name) = 0;

    virtual void start();
    virtual void stop();

    virtual void noteOn(midi_event ev) { (void)ev; };
    virtual void noteOff(midi_event ev) { (void)ev; };
    virtual void ChordOn(midi_event ev) { (void)ev; };
    virtual void Load(std::string preset_name);
    virtual void Save(std::string preset_name);

    virtual void control(midi_event ev) { (void)ev; };
    virtual void pitchBend(midi_event ev) { (void)ev; };
    virtual void randomize(){};
    virtual void allNotesOff(){};

    virtual void eventNotify(broadcast_event event, mixer_timing_info tinfo);

    void parseMidiEvent(midi_event ev, mixer_timing_info tinfo);

    void SetVolume(double val);
    double GetVolume();

    void SetPan(double val);
    double GetPan();

  public:
    sound_generator_type type;

    sequenceengine engine;

    int mixer_idx;
    //  int num_patterns;
    bool active;

    double volume{0.7}; // between 0 and 1.0
    double pan{0.};     // between -1(hard left) and 1(hard right)

    int note_duration_ms_{100};

    std::atomic<int16_t> effects_num{0}; // num of effects
    Fx *effects[kMaxNumSoundGenFx] = {};
    bool effects_on{true}; // bool

  public:
    int AddFx(Fx *f);
    int AddBasicfilter();
    int AddBitcrush();
    int AddCompressor();
    int AddDistortion();
    int AddDelay();
    int AddEnvelope();
    int AddModdelay();
    int AddModfilter();
    int AddFollower();
    int AddReverb();
    int AddWaveshape();

    stereo_val Effector(stereo_val val);

    bool IsSynth();
    bool IsStepper();
};
