#pragma once

#include "defjams.h"
#include "envelope_generator.h"
#include "filter_moogladder.h"
#include "qblimited_oscillator.h"
#include "sequencer.h"
#include "sound_generator.h"


static const char DRUMSYNTH_SAVED_SETUPS_FILENAME[512] = "settings/synthdrumpatches.dat";

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
    double osc1_sustain_len_in_samples;
    int osc1_sustain_counter;
    double osc1_amp;
    double eg1_osc1_intensity;

    qblimited_oscillator m_osc2;
    double osc2_sustain_len_in_samples;
    int osc2_sustain_counter;
    double osc2_amp;

    qblimited_oscillator m_osc3;
    double osc3_sustain_len_in_samples;
    int osc3_sustain_counter;
    double osc3_amp;

    envelope_generator m_eg1;
    envelope_generator m_eg2;

    filter_moogladder m_filter;

    double m_distortion_threshold;


    pattern_hit_metadata metadata[SEQUENCER_PATTERN_LEN];

    bool started;

} synthdrum_sequencer;

synthdrum_sequencer *new_synthdrum_seq(void);

void sds_status(void *self, wchar_t *ss);
void sds_setvol(void *self, double v);
double sds_gennext(void *self);
double sds_getvol(void *self);
void sds_trigger(synthdrum_sequencer *sds);
void sds_stop(synthdrum_sequencer *sds);
void sds_parse_midi(synthdrum_sequencer *s, int status, int data1, int data2);
bool synthdrum_save_patch(synthdrum_sequencer *sds, char *name);
bool synthdrum_open_patch(synthdrum_sequencer *sds, char *name);
bool synthdrum_list_patches(void);
