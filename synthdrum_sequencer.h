#pragma once

#include "defjams.h"
#include "envelope_generator.h"
#include "filter_moogladder.h"
#include "qblimited_oscillator.h"
#include "sequencer.h"
#include "sound_generator.h"

static const char DRUMSYNTH_SAVED_SETUPS_FILENAME[512] =
    "settings/synthdrumpatches.dat";

typedef struct pattern_hit_metadata {
    bool played;
    bool playing;
} pattern_hit_metadata;

typedef struct synthdrum_sequencer {
    SOUNDGEN sg;
    sequencer m_seq;
    double m_pitch;
    char m_patch_name[512];
    double vol;
    unsigned drumtype; // KICK or SNARE
    unsigned int midi_controller_mode;

    qblimited_oscillator m_osc1;
    double osc1_amp;

    qblimited_oscillator m_osc2;
    double osc2_amp;

    envelope_generator m_eg1;
    double eg1_sustain_len_in_samples;
    int eg1_sustain_counter;

    envelope_generator m_eg2;
    double eg2_osc2_intensity;
    double eg2_sustain_len_in_samples;
    int eg2_sustain_counter;

    envelope_generator m_eg3;
    double eg3_sustain_len_in_samples;
    int eg3_sustain_counter;

    filter_moogladder m_filter;

    double m_distortion_threshold;

    pattern_hit_metadata metadata[SEQUENCER_PATTERN_LEN];

    bool mod_pitch;

    bool active;
    bool started;

} synthdrum_sequencer;

synthdrum_sequencer *new_synthdrum_seq(void);
void synthdrum_del_self(synthdrum_sequencer *sds);

void sds_status(void *self, wchar_t *ss);
void sds_setvol(void *self, double v);
double sds_gennext(void *self);
double sds_getvol(void *self);

void sds_start(void *self);
void sds_stop(void *self);
int sds_get_num_tracks(void *self);
void sds_make_active_track(void *self, int tracknum);

void sds_trigger(synthdrum_sequencer *sds);
void sds_parse_midi(synthdrum_sequencer *s, int status, int data1, int data2);
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
void synthdrum_set_mod_pitch(synthdrum_sequencer *sds, bool b);
void synthdrum_set_osc_amp(synthdrum_sequencer *sds, int osc_num, double val);
