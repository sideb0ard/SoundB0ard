#ifndef LOOPER_H
#define LOOPER_H

#include "envelope_generator.h"
#include "lfo.h"
#include "sound_generator.h"
#include "step_sequencer.h"
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
    LOOPER_ENV_PARABOLIC,
    LOOPER_ENV_TRAPEZOIDAL,
    LOOPER_ENV_RAISED_COSINE_BELL,
    LOOPER_ENV_NUM
};

typedef struct looper
{
    soundgenerator sound_generator;

    bool started;
    bool have_active_buffer;

    char filename[512];
    double *audio_buffer;
    int num_channels;
    int audio_buffer_len;
    int size_of_sixteenth;
    double audio_buffer_read_idx;
    int audio_buffer_write_idx;
    int external_source_sg; // XOR - external or file

    int num_active_grains;
    int highest_grain_num;
    int cur_grain_num;
    sound_grain m_grains[MAX_CONCURRENT_GRAINS];

    int granular_spray_frames;
    int quasi_grain_fudge;
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

    bool loop_mode;
    double loop_len; // bars

    bool scramble_mode;
    int scramble_diff;

    bool stutter_mode;
    int stutter_idx;

    int cur_sixteenth; // used to track scramble

    double vol;
} looper;

looper *new_looper(char *filename);

stereo_val looper_gennext(void *self);
void looper_status(void *self, wchar_t *ss);
void looper_setvol(void *self, double v);
double looper_getvol(void *self);
void looper_start(void *self);
void looper_stop(void *self);
int looper_get_num_patterns(void *self);
void looper_set_num_patterns(void *self, int num_patterns);
void looper_make_active_track(void *self, int tracknum);
void looper_event_notify(void *self, unsigned int event_type);

void looper_import_file(looper *g, char *filename);
void looper_set_external_source(looper *g, int sound_gen_num);

void looper_update_lfos(looper *g);
int looper_calculate_grain_spacing(looper *g);
void looper_set_sequencer_mode(looper *g, bool b);
void looper_set_grain_pitch(looper *g, double pitch);
void looper_set_grain_duration(looper *g, int dur);
void looper_set_grains_per_sec(looper *g, int gps);
void looper_set_grain_attack_size_pct(looper *g, int att);
void looper_set_grain_release_size_pct(looper *g, int rel);
void looper_set_audio_buffer_read_idx(looper *g, int position);
void looper_set_granular_spray(looper *g, int spray_ms);
void looper_set_quasi_grain_fudge(looper *g, int fudgefactor);
void looper_set_selection_mode(looper *g, unsigned int mode);
void looper_set_envelope_mode(looper *g, unsigned int mode);
void looper_set_movement_mode(looper *g, bool b);
void looper_set_reverse_mode(looper *g, bool b);
void looper_set_loop_mode(looper *g, bool b);
void looper_set_granulate_mode(looper *g, bool b);
void looper_set_loop_len(looper *g, double bars);
void looper_set_scramble_mode(looper *g, bool b);
void looper_set_stutter_mode(looper *g, bool b);

int looper_get_available_grain_num(looper *g);
int looper_count_active_grains(looper *g);

void looper_set_lfo_amp(looper *g, int lfonum, double amp);
void looper_set_lfo_voice(looper *g, int lfonum, unsigned int voice);
void looper_set_lfo_rate(looper *g, int lfonum, double rate);
void looper_set_lfo_min(looper *g, int lfonum, double minval);
void looper_set_lfo_max(looper *g, int lfonum, double maxval);
void looper_set_lfo_sync(looper *g, int lfonum, int numloops);

void sound_grain_init(sound_grain *g, int dur, int starting_idx, int attack_pct,
                      int release_pct, bool reverse, double pitch,
                      int num_channels);
stereo_val sound_grain_generate(sound_grain *g, double *audio_buffer,
                                int buffer_len);
double sound_grain_env(sound_grain *g, unsigned int envelope_mode);

void looper_del_self(void *self);

#endif // LOOPER
