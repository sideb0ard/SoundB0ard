#pragma once

#include "defjams.h"
#include "distortion.h"
#include "envelope_generator.h"
#include "filter_moogladder.h"
#include "qblimited_oscillator.h"
#include "sequencer.h"
#include "sound_generator.h"

static const char DRUMSYNTH_SAVED_SETUPS_FILENAME[512] =
    "settings/synthdrumpatches.dat";

typedef struct pattern_hit_metadata
{
    bool played;
    bool playing;
} pattern_hit_metadata;

typedef struct synthdrum_sequencer
{
    soundgenerator sg;
    sequencer m_seq;
    char m_patch_name[512];
    double vol;

    qblimited_oscillator m_osc1;
    double osc1_amp;

    qblimited_oscillator m_osc2;
    double osc2_amp;

    // osc1 amp
    envelope_generator m_eg1;
    double eg1_sustain_len_in_samples;
    int eg1_sustain_ms;
    int eg1_sustain_counter;

    // osc2 pitch
    envelope_generator m_eg2;
    double eg2_osc2_intensity;
    double eg2_sustain_len_in_samples;
    int eg2_sustain_ms;
    int eg2_sustain_counter;

    // output amp
    envelope_generator m_eg3;
    double eg3_sustain_len_in_samples;
    int eg3_sustain_ms;
    int eg3_sustain_counter;

    int m_filter_type;
    double m_filter_fc;
    double m_filter_q;
    double m_distortion_threshold;

    filter_moogladder m_filter;
    distortion m_distortion;

    pattern_hit_metadata metadata[SEQUENCER_PATTERN_LEN];

    int mod_semitones_range;
    bool started;

} synthdrum_sequencer;

synthdrum_sequencer *new_synthdrum_seq(void);
void synthdrum_del_self(void *self);

void sds_status(void *self, wchar_t *ss);
void sds_setvol(void *self, double v);
double sds_gennext(void *self);
double sds_getvol(void *self);

void sds_start(void *self);
void sds_stop(void *self);
int sds_get_num_tracks(void *self);
void sds_make_active_track(void *self, int tracknum);

void sds_trigger(synthdrum_sequencer *sds);
bool synthdrum_save_patch(synthdrum_sequencer *sds, char *name);
bool synthdrum_open_patch(synthdrum_sequencer *sds, char *name);
bool synthdrum_list_patches(void);
void synthdrum_set_osc_wav(synthdrum_sequencer *sds, int osc_num,
                           unsigned int wave);
void synthdrum_set_osc_fo(synthdrum_sequencer *sds, int osc_num, double freq);
void synthdrum_set_eg_attack(synthdrum_sequencer *sds, int eg_num, double val);
void synthdrum_set_eg_decay(synthdrum_sequencer *sds, int eg_num, double val);
void synthdrum_set_eg_sustain_ms(synthdrum_sequencer *sds, int eg_num,
                                 double val);
void synthdrum_set_eg_release(synthdrum_sequencer *sds, int eg_num, double val);
void synthdrum_set_eg2_osc_intensity(synthdrum_sequencer *sds, double val);
void synthdrum_set_osc_amp(synthdrum_sequencer *sds, int osc_num, double val);
void synthdrum_set_distortion_threshold(synthdrum_sequencer *sds, double val);
void synthdrum_set_filter_freq(synthdrum_sequencer *sds, double val);
void synthdrum_set_filter_q(synthdrum_sequencer *sds, double val);
void synthdrum_set_filter_type(synthdrum_sequencer *sds, unsigned int val);
void synthdrum_set_mod_semitones_range(synthdrum_sequencer *sds, int val);

void synthdrum_randomize(synthdrum_sequencer *sds);
