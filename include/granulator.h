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

typedef struct sound_grain
{
    int grain_len_frames;
    int audiobuffer_num;
    int audiobuffer_start_idx;
    int audiobuffer_num_channels;
    double audiobuffer_cur_pos;
    double audiobuffer_inc;
    double audiobuffer_pitch;
    int release_time_pct; // percent of grain_len_frames
    int attack_time_pct;  // percent of grain_len_frames
    int attack_time_samples;
    int release_time_samples;
    bool active;
    double amp;
    double slope;
    double curve;
    bool reverse_mode;
    double incr;
} sound_grain;

enum
{
    GRAIN_SELECTION_STATIC,
    GRAIN_SELECTION_RANDOM,
    GRAIN_NUM_SELECTION_MODES
};

enum
{
    GRANULATOR_ENV_PARABOLIC,
    GRANULATOR_ENV_TRAPEZOIDAL,
    GRANULATOR_ENV_RAISED_COSINE_BELL,
    GRANULATOR_ENV_NUM
};

typedef struct granulator
{
    soundgenerator sound_generator;

    bool started;
    bool have_active_buffer;

    char filename[512];
    double *audio_buffer;
    int audio_buffer_len;
    int num_channels;
    int audio_buffer_write_idx;
    int external_source_sg; // exclusive - external or file

    int num_active_grains;
    int highest_grain_num;
    int cur_grain_num;
    sound_grain m_grains[MAX_CONCURRENT_GRAINS];
    sound_grain m_next_grain;

    int granular_spray;
    int quasi_grain_fudge;
    int grain_buffer_position;
    int grain_duration_ms;
    int grains_per_sec;
    double grain_pitch;
    int num_grains_per_looplen;
    unsigned int selection_mode;
    unsigned int envelope_mode;
    unsigned int movement_mode;
    int movement_pct;
    unsigned int reverse_mode;

    int last_grain_launched_sample_time;
    int grain_attack_time_pct;
    int grain_release_time_pct;

    sequencer m_seq;
    bool sequencer_mode;
    int sequencer_gate;

    envelope_generator m_eg1; // start/stop amp
    envelope_generator m_eg2; // unused so far

    bool graindur_lfo_on;
    lfo m_lfo1; // grain dur
    double m_lfo1_min;
    double m_lfo1_max;
    bool lfo1_sync;

    bool grainps_lfo_on;
    lfo m_lfo2; // grains per sec
    double m_lfo2_min;
    double m_lfo2_max;
    bool lfo2_sync;

    bool grainscanfile_lfo_on;
    lfo m_lfo3; // file read position
    double m_lfo3_min;
    double m_lfo3_max;
    bool lfo3_sync;

    bool grainpitch_lfo_on;
    lfo m_lfo4; // file read position
    double m_lfo4_min;
    double m_lfo4_max;
    bool lfo4_sync;

    double vol;
} granulator;

granulator *new_granulator(char *filename);

stereo_val granulator_gennext(void *self);
void granulator_status(void *self, wchar_t *ss);
void granulator_setvol(void *self, double v);
double granulator_getvol(void *self);
void granulator_start(void *self);
void granulator_stop(void *self);
int granulator_get_num_tracks(void *self);
void granulator_make_active_track(void *self, int tracknum);
void granulator_event_notify(void *self, unsigned int event_type);

void granulator_import_file(granulator *g, char *filename);
void granulator_set_external_source(granulator *g, int sound_gen_num);

void granulator_update_lfos(granulator *g);
int granulator_calculate_grain_spacing(granulator *g);
void granulator_set_sequencer_mode(granulator *g, bool b);
void granulator_set_grain_pitch(granulator *g, double pitch);
void granulator_set_grain_duration(granulator *g, int dur);
void granulator_set_grains_per_sec(granulator *g, int gps);
void granulator_set_grain_attack_size_pct(granulator *g, int att);
void granulator_set_grain_release_size_pct(granulator *g, int rel);
void granulator_set_grain_buffer_position(granulator *g, int position);
void granulator_set_granular_spray(granulator *g, int spray_ms);
void granulator_set_quasi_grain_fudge(granulator *g, int fudgefactor);
void granulator_set_selection_mode(granulator *g, unsigned int mode);
void granulator_set_envelope_mode(granulator *g, unsigned int mode);
void granulator_set_movement_mode(granulator *g, bool b);
void granulator_set_reverse_mode(granulator *g, bool b);
int granulator_get_available_grain_num(granulator *g);
int granulator_count_active_grains(granulator *g);

void granulator_set_lfo_amp(granulator *g, int lfonum, double amp);
void granulator_set_lfo_voice(granulator *g, int lfonum, unsigned int voice);
void granulator_set_lfo_rate(granulator *g, int lfonum, double rate);
void granulator_set_lfo_min(granulator *g, int lfonum, double minval);
void granulator_set_lfo_max(granulator *g, int lfonum, double maxval);
void granulator_set_lfo_sync(granulator *g, int lfonum, int numloops);

void sound_grain_init(sound_grain *g, int dur, int starting_idx, int attack_pct,
                      int release_pct, bool reverse, double pitch,
                      int num_channels);
stereo_val sound_grain_generate(sound_grain *g, double *audio_buffer,
                                int buffer_len);
double sound_grain_env(sound_grain *g, unsigned int envelope_mode);

void granulator_del_self(void *self);

#endif // GRANULATOR
