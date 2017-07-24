#ifndef GRANULATOR_H
#define GRANULATOR_H

#include "envelope_generator.h"
#include "lfo.h"
#include "sequencer.h"
#include "sound_generator.h"
#include <stdbool.h>
#include <wchar.h>

#define MAX_CONCURRENT_GRAINS 1000
#define MAX_GRAIN_STREAM_LEN_SEC 10 // assuming a loop is never more than 10sec

typedef struct sound_grain {
    int grain_len_samples;
    int audiobuffer_num;
    int audiobuffer_start_idx;
    int audiobuffer_cur_pos;
    int audiobuffer_pitch;
    int release_time_pct; // percent of grain_len_samples
    int attack_time_pct;  // percent of grain_len_samples
    bool active;
    bool deactivation_pending;
} sound_grain;

enum {
    GRAIN_SELECTION_STATIC,
    GRAIN_SELECTION_RANDOM,
    GRAIN_NUM_SELECTION_MODES
};

typedef struct granulator {
    SOUNDGEN sound_generator;

    bool active;
    bool started;

    char filename[512];
    double *filecontents;
    int filecontents_len;
    int current_file_read_position;

    int num_active_grains;
    int highest_grain_num;
    int cur_grain_num;
    sound_grain m_grains[MAX_CONCURRENT_GRAINS];
    sound_grain m_next_grain;

    int granular_spray;
    int quasi_grain_fudge;
    int grain_file_position;
    int grain_duration_ms;
    int grains_per_sec;
    int num_grains_per_looplen;
    unsigned int selection_mode;

    bool scan_through_file;
    int scan_speed;

    int grain_stream[MAX_GRAIN_STREAM_LEN_SEC * SAMPLE_RATE];
    int grain_attack_time_pct;
    int grain_release_time_pct;

    sequencer m_seq;
    bool sequencer_mode;
    int sequencer_gate;

    envelope_generator m_eg1;
    lfo m_lfo1;

    double vol;
} granulator;

granulator *new_granulator(char *filename);

// void granulator_gennext(void* self, double* frame_vals, int framesPerBuffer);
double granulator_gennext(void *self);

void granulator_status(void *self, wchar_t *ss);
void granulator_setvol(void *self, double v);
double granulator_getvol(void *self);
void granulator_start(void *self);
void granulator_stop(void *self);
int granulator_get_num_tracks(void *self);
void granulator_make_active_track(void *self, int tracknum);

void granulator_import_file(granulator *g, char *filename);

void granulator_refresh_grain_stream(granulator *g);
void granulator_set_scan_mode(granulator *g, bool b);
void granulator_set_sequencer_mode(granulator *g, bool b);
void granulator_set_scan_speed(granulator *g, int speed);
void granulator_set_grain_duration(granulator *g, int dur);
void granulator_set_grains_per_sec(granulator *g, int gps);
void granulator_set_grain_attack_size_pct(granulator *g, int att);
void granulator_set_grain_release_size_pct(granulator *g, int rel);
void granulator_set_grain_file_position(granulator *g, int position);
void granulator_set_granular_spray(granulator *g, int spray_ms);
void granulator_set_quasi_grain_fudge(granulator *g, int fudgefactor);
void granulator_set_selection_mode(granulator *g, unsigned int mode);
int granulator_get_available_grain_num(granulator *g);
int granulator_deactivate_other_grains(granulator *g);

void granulator_set_lfo_amp(granulator *g, double amp);
void granulator_set_lfo_voice(granulator *g, unsigned int voice);
void granulator_set_lfo_rate(granulator *g, double rate);

void sound_grain_init(sound_grain *g, int dur, int starting_idx, int attack_pct,
                      int release_pct, int pitch);
int sound_grain_generate_idx(sound_grain *g);
double sound_grain_env(sound_grain *g);

void granulator_del_self(granulator *g);

#endif // GRANULATOR
