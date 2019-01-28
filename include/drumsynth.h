#pragma once

#include "defjams.h"
#include "distortion.h"
#include "envelope_generator.h"
#include "filter_moogladder.h"
#include "qblimited_oscillator.h"
#include "sequence_engine.h"
#include "sound_generator.h"

static const char DRUMSYNTH_SAVED_SETUPS_FILENAME[512] =
    "settings/drumsynthpatches.dat";

typedef struct drumsynth
{
    sound_generator sg;
    sequence_engine engine;
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
    distortion m_distortion;

    int mod_semitones_range;
    bool started;

    uint16_t cur_state;
    int current_velocity;

    bool debug;

} drumsynth;

drumsynth *new_drumsynth(void);
void drumsynth_del_self(void *self);

void drumsynth_status(void *self, wchar_t *ss);
stereo_val drumsynth_gennext(void *self);

void drumsynth_start(void *self);
void drumsynth_stop(void *self);
int drumsynth_get_num_patterns(void *self);
void drumsynth_set_num_patterns(void *self, int num_patterns);
bool drumsynth_is_valid_pattern(void *self, int pattern_num);
void drumsynth_make_active_track(void *self, int tracknum);

void drumsynth_trigger(drumsynth *ds);
bool drumsynth_save_patch(drumsynth *ds, char *name);
bool drumsynth_open_patch(drumsynth *ds, char *name);
bool drumsynth_list_patches(void);
void drumsynth_set_osc_wav(drumsynth *ds, int osc_num, unsigned int wave);
void drumsynth_set_osc_fo(drumsynth *ds, int osc_num, double freq);
void drumsynth_set_reset_osc(drumsynth *ds, bool b);
void drumsynth_set_eg_attack(drumsynth *ds, int eg_num, double val);
void drumsynth_set_eg_decay(drumsynth *ds, int eg_num, double val);
void drumsynth_set_eg_sustain_lvl(drumsynth *ds, int eg_num, double val);
void drumsynth_set_eg_release(drumsynth *ds, int eg_num, double val);
void drumsynth_set_eg_osc_intensity(drumsynth *ds, int eg, int osc, double val);
void drumsynth_set_osc_amp(drumsynth *ds, int osc_num, double val);
void drumsynth_set_distortion_threshold(drumsynth *ds, double val);
void drumsynth_set_filter_freq(drumsynth *ds, double val);
void drumsynth_set_filter_q(drumsynth *ds, double val);
void drumsynth_set_filter_type(drumsynth *ds, unsigned int val);
void drumsynth_set_mod_semitones_range(drumsynth *ds, int val);

void drumsynth_randomize(drumsynth *ds);

void drumsynth_set_debug(drumsynth *ds, bool debug);
