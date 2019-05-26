#ifndef MIXER_H
#define MIXER_H

#include "portaudio.h"
#include "portmidi.h"
#include <lo/lo.h>
#include <pthread.h>

#include "ableton_link_wrapper.h"

#include "SoundGenerator.h"
#include "algorithm.h"
#include "defjams.h"
#include "digisynth.h"
#include "dxsynth.h"
#include "fx.h"
#include "minisynth.h"
#include "pattern_generator.h"
#include "sbmsg.h"
#include "table.h"
#include "value_generator.h"

#define MAX_SCENES 100
#define MAX_TRACKS_PER_SCENE 100
#define MAX_NUM_ALGORITHMS 100
#define MAX_NUM_SOUND_GENERATORS 100
#define MAX_NUM_PATTERN_GENERATORS 100
#define MAX_NUM_VALUE_GENERATORS 100
#define NUM_PROGRESSIONS 4

typedef enum
{
    Q32,
    Q16,
    Q8,
    Q4
} quantize_size;

typedef struct environment_variable
{
    char key[ENVIRONMENT_KEY_SIZE];
    unsigned int env_var_type;
    void *data;
    int val;
} env_var;

typedef struct soundgen_track
{
    int soundgen_num;
    int soundgen_track_num;
} soundgen_track;

typedef struct scene
{
    int num_bars_to_play;
    int num_tracks;
    soundgen_track soundgen_tracks[MAX_TRACKS_PER_SCENE];
} scene;

typedef unsigned int compat_key_list[6];

// struct AbletonLink AbletonLink;

typedef struct preview_buffer
{
    char filename[512];
    double *audio_buffer;
    int num_channels;
    int audio_buffer_len;
    int audio_buffer_read_idx;
    bool enabled;
} preview_buffer;

stereo_val preview_buffer_generate(preview_buffer *buffy);
void preview_buffer_import_file(preview_buffer *buffy, char *filename);

struct mixer
{

    preview_buffer preview;

    algorithm *algorithms[MAX_NUM_ALGORITHMS];
    int algorithm_num;

    SoundGenerator *SoundGenerators[MAX_NUM_SOUND_GENERATORS];
    int soundgen_num; // actual number of SGs

    pattern_generator *pattern_generators[MAX_NUM_PATTERN_GENERATORS];
    int pattern_gen_num; // actual number of PGs

    value_generator *value_generators[MAX_NUM_VALUE_GENERATORS];
    int value_gen_num; // actual number of VGs

    AbletonLink *m_ableton_link;

    stereo_val
        soundgen_cur_val[MAX_NUM_SOUND_GENERATORS]; // cache for current val,
    // currently used for sidechain
    // compressor TODO there are no
    // checks for this num
    double
        soundgen_volume[MAX_NUM_SOUND_GENERATORS]; // separating instrument amp
    // from mixer volume per channel

    env_var environment[ENVIRONMENT_ARRAY_SIZE];
    int env_var_count;

    bool debug_mode;

    scene scenes[MAX_SCENES];
    int num_scenes; // actual amount of scenes
    int current_scene;
    int current_scene_bar_count;

    PortMidiStream *midi_stream;
    bool have_midi_controller;

    bool midi_print_notes;
    char midi_controller_name[128];
    unsigned int midi_control_destination;
    unsigned int m_midi_controller_mode; // to switch control knob routing
    unsigned int midi_bank_num;

    int active_midi_soundgen_num;
    int active_midi_soundgen_effect_num;

    double bpm;

    mixer_timing_info timing_info;

    bool scene_mode;
    bool scene_start_pending;

    double volume;

    int bars_per_chord;
    int bar_counter;

    // chord progressions
    int prog_len;
    int prog_degrees[6]; // max prog_len
    int prog_degrees_idx;
    unsigned int progression_type;
    bool should_progress_chords;

    unsigned int key;
    unsigned int chord;
    unsigned int chord_type;
    unsigned int octave;
    unsigned int notes[8];
    unsigned int quantize;

    lo_address processing_addr;
};

mixer *new_mixer(double output_latency);

void mixer_ps(mixer *mixr, bool all);
void mixer_status_valz(mixer *mixr);
void mixer_status_patz(mixer *mixr);
void mixer_status_sgz(mixer *mixr, bool all);
void mixer_status_mixr(mixer *mixr);
void mixer_status_algoz(mixer *mixr, bool all);
void mixer_update_bpm(mixer *mixr, int bpm);
void mixer_update_time_unit(mixer *mixr, unsigned int time_type, int val);
void mixer_events_output(mixer *mixr);
void mixer_emit_event(mixer *mixr, broadcast_event event);
bool mixer_del_soundgen(mixer *mixr, int soundgen_num);
void mixer_generate_pattern(mixer *mixr, int synthnum, int pattern_num);
int mixer_add_algorithm(mixer *mixr, algorithm *a);
int mixer_add_bitshift(mixer *mixr, int num_wurds, char wurds[][SIZE_OF_WURD]);
int mixer_add_euclidean(mixer *mixr, int num_hits, int num_steps);
int mixer_add_intdiv(mixer *mixr);
int mixer_add_juggler(mixer *mixr, unsigned int style);
int mixer_add_markov(mixer *mixr, unsigned int type);
int mixer_add_value_list(mixer *mixr, unsigned int values_type, int values_len,
                         void *values);
void mixer_preview_audio(mixer *mixr, char *filename);

int mixer_print_timing_info(mixer *mixr);

int add_bytebeat(mixer *mixr, char *pattern);
int add_minisynth(mixer *mixr);
int add_dxsynth(mixer *mixr);
int add_digisynth(mixer *mixr, char *filename);
int add_looper(mixer *mixr, char *filename);

int add_sound_generator(mixer *mixr, SoundGenerator *sg);
int add_pattern_generator(mixer *mixr, pattern_generator *sg);
int add_value_generator(mixer *mixr, value_generator *vg);

int add_effect(mixer *mixr);

void mixer_vol_change(mixer *mixr, float vol);
void vol_change(mixer *mixr, int sig, float vol);
void pan_change(mixer *mixr, int sig, float vol);

void mixer_toggle_midi_mode(mixer *mixr);
void mixer_toggle_key_mode(mixer *mixr);
void mixer_play_scene(mixer *mixr, int scene_num);

void update_environment(char *key, int val);

int get_environment_val(char *key, int *return_val);

void mixer_update_timing_info(mixer *mixr, long long int frame_time);
int mixer_gennext(mixer *mixr, float *out, int frames_per_buffer);

bool mixer_is_valid_algo(mixer *mixr, int algo_num);
bool mixer_is_valid_env_var(mixer *mixr, char *key);
bool mixer_is_valid_pattern_gen_num(mixer *mixr, int sgnum);
bool mixer_is_valid_value_gen_num(mixer *mixr, int sgnum);
bool mixer_is_valid_soundgen_num(mixer *mixr, int soundgen_num);
bool mixer_is_valid_soundgen_track_num(mixer *mixr, int soundgen_num,
                                       int track_num);
bool mixer_is_valid_scene_num(mixer *mixr, int scene_num);
bool mixer_is_valid_fx(mixer *mixr, int soundgen_num, int fx_num);
bool mixer_is_soundgen_in_scene(int soundgen_num, scene *scene_num);

int mixer_add_scene(mixer *mixr, int num_bars);
bool mixer_add_soundgen_track_to_scene(mixer *mixr, int scene_num,
                                       int soundgen_num, int soundgen_track);
bool mixer_rm_soundgen_track_from_scene(mixer *mixr, int scene_num,
                                        int soundgen_num, int soundgen_track);
bool mixer_cp_scene(mixer *mixr, int scene_num_from, int scene_num_to);

void mixer_set_notes(mixer *mixr);
void mixer_set_octave(mixer *mixr, int octave);
void mixer_set_bars_per_chord(mixer *mixr, int bars);

double mixer_get_hz_per_bar(mixer *mixr);
int mixer_get_ticks_per_cycle_unit(mixer *mixr, unsigned int event_type);
void mixer_set_chord_progression(mixer *mixr, unsigned int prog_num);
void mixer_change_chord(mixer *mixr, unsigned int root,
                        unsigned int chord_type);
int mixer_get_key_from_degree(mixer *mixr, unsigned int scale_degree);
void mixer_enable_print_midi(mixer *mixr, bool b);
void mixer_check_for_midi_messages(mixer *mixr);
void mixer_set_midi_bank(mixer *mixr, int num);
void mixer_set_should_progress_chords(mixer *mixr, bool b);
bool should_progress_chords(mixer *mixr, int tick);
void mixer_next_chord(mixer *mixr);

#endif // MIXER_H
