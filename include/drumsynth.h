#pragma once

#include <defjams.h>
#include <envelope_generator.h>
#include <filter_moogladder.h>
#include <fx/distortion.h>
#include <qblimited_oscillator.h>
#include <soundgenerator.h>

static const char DRUMSYNTH_SAVED_SETUPS_FILENAME[512] =
    "settings/drumsynthpatches.dat";

class DrumSynth : public SoundGenerator
{
  public:
    DrumSynth();
    ~DrumSynth() = default;
    std::string Info() override;
    std::string Status() override;
    stereo_val genNext() override;
    void noteOn(midi_event ev) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

  public:
    char m_patch_name[512];
    bool reset_osc;

    // OSC1 ///////////////////////////
    qblimited_oscillator m_osc1;
    double osc1_amp;

    // osc1 amp ENV
    envelope_generator m_eg1;
    double eg1_osc1_intensity;
    int eg1_sustain_ms;

    // OSC2 ///////////////////////////
    qblimited_oscillator m_osc2;
    double osc2_amp;

    // COMBINED osc2 pitch ENV and output ENV
    envelope_generator m_eg2;
    double eg2_osc2_intensity;
    int eg2_sustain_ms;

    // FILTER ///////////////////

    int m_filter_type;
    double m_filter_fc;
    double m_filter_q;
    double m_distortion_threshold;

    filter_moogladder m_filter;

    // DISTORTION
    Distortion m_distortion;

    int mod_semitones_range;
    bool started;

    uint16_t cur_state;
    int current_velocity;

    bool debug;
};

bool drumsynth_save_patch(DrumSynth *ds, char *name);
bool drumsynth_open_patch(DrumSynth *ds, char *name);
bool drumsynth_list_patches(void);
void drumsynth_set_osc_wav(DrumSynth *ds, int osc_num, unsigned int wave);
void drumsynth_set_osc_fo(DrumSynth *ds, int osc_num, double freq);
void drumsynth_set_reset_osc(DrumSynth *ds, bool b);
void drumsynth_set_eg_attack(DrumSynth *ds, int eg_num, double val);
void drumsynth_set_eg_decay(DrumSynth *ds, int eg_num, double val);
void drumsynth_set_eg_sustain_lvl(DrumSynth *ds, int eg_num, double val);
void drumsynth_set_eg_release(DrumSynth *ds, int eg_num, double val);
void drumsynth_set_eg_osc_intensity(DrumSynth *ds, int eg, int osc, double val);
void drumsynth_set_osc_amp(DrumSynth *ds, int osc_num, double val);
void drumsynth_set_distortion_threshold(DrumSynth *ds, double val);
void drumsynth_set_filter_freq(DrumSynth *ds, double val);
void drumsynth_set_filter_q(DrumSynth *ds, double val);
void drumsynth_set_filter_type(DrumSynth *ds, unsigned int val);
void drumsynth_set_mod_semitones_range(DrumSynth *ds, int val);

void drumsynth_randomize(DrumSynth *ds);

void drumsynth_set_debug(DrumSynth *ds, bool debug);
