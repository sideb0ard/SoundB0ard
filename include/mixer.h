#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>

#include "algorithm.h"
#include "defjams.h"
#include "dxsynth.h"
#include "fx.h"
#include "minisynth.h"
#include "sbmsg.h"
#include "sequence_generator.h"
#include "sound_generator.h"
#include "step_sequencer.h"
#include "table.h"

#define MAX_SCENES 100
#define MAX_TRACKS_PER_SCENE 100
#define MAX_NUM_soundgenerator 100

typedef enum { Q32, Q16, Q8, Q4 } quantize_size;

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

typedef struct AbletonLink AbletonLink;

typedef struct mixer
{

    algorithm **algorithms;
    int algorithm_num;
    int algorithm_size;

    soundgenerator **sound_generators;
    int soundgen_num;  // actual number of SGs
    int soundgen_size; // number of memory slots reserved for SGszz

    sequence_generator **sequence_generators;
    int sequence_gen_num;  // actual number of SGs
    int sequence_gen_size; // number of memory slots reserved for SGszz

    AbletonLink *m_ableton_link;

    stereo_val
        soundgen_cur_val[MAX_NUM_soundgenerator]; // cache for current val,
    // currently used for sidechain
    // compressor TODO there are no
    // checks for this num

    env_var environment[ENVIRONMENT_ARRAY_SIZE];
    int env_var_count;

    bool debug_mode;

    scene scenes[MAX_SCENES];
    int num_scenes; // actual amount of scenes
    int current_scene;
    int current_scene_bar_count;

    bool have_midi_controller;
    char midi_controller_name[128];
    unsigned int midi_control_destination;
    unsigned int m_midi_controller_mode; // to switch control knob routing
    unsigned int m_key_controller_mode;  // to switch key control routing

    int active_midi_soundgen_num;
    int active_midi_soundgen_effect_num;

    double bpm;

    mixer_timing_info timing_info;

    bool scene_mode;
    bool scene_start_pending;

    double volume;

    unsigned int key;
    unsigned int quantize;

} mixer;

mixer *new_mixer(double output_latency);

void mixer_ps(mixer *mixr);
void mixer_update_bpm(mixer *mixr, int bpm);
void mixer_update_time_unit(mixer *mixr, unsigned int time_type, int val);
void mixer_emit_event(mixer *mixr, unsigned int event_type);
bool mixer_del_soundgen(mixer *mixr, int soundgen_num);
void mixer_generate_pattern(mixer *mixr, int synthnum, int pattern_num);
void mixer_print_compat_keys(mixer *mixr);
int mixer_add_bitshift(mixer *mixr, int num_wurds, char wurds[][SIZE_OF_WURD]);
int mixer_add_euclidean(mixer *mixr, int num_hits, int num_steps);
int mixer_add_algorithm(mixer *mixr, algorithm *a);

int add_bytebeat(mixer *mixr, char *pattern);
int add_minisynth(mixer *mixr);
int add_dxsynth(mixer *mixr);
int add_digisynth(mixer *mixr, char *filename);
// int add_seq_char_pattern(mixer *mixr, char *filename, char *pattern);
int add_seq_euclidean(mixer *mixr, char *filename, int num_beats,
                      bool start_on_first_beat);
int add_looper(mixer *mixr, char *filename);
int add_sound_generator(mixer *mixr, soundgenerator *sg);
int add_sequence_generator(mixer *mixr, sequence_generator *sg);
int add_effect(mixer *mixr);
void mixer_vol_change(mixer *mixr, float vol);
void vol_change(mixer *mixr, int sig, float vol);
void freq_change(mixer *mixr, int sig, float freq);

void mixer_toggle_midi_mode(mixer *mixr);
void mixer_toggle_key_mode(mixer *mixr);
void mixer_play_scene(mixer *mixr, int scene_num);

void mixer_preview_track(mixer *mixr, char *filename);

void update_environment(char *key, int val);
int get_environment_val(char *key, int *return_val);

void mixer_update_timing_info(mixer *mixr, long long int frame_time);
int mixer_gennext(mixer *mixr, float *out, int frames_per_buffer);

bool mixer_is_valid_env_var(mixer *mixr, char *key);
bool mixer_is_valid_seq_gen_num(mixer *mixr, int sgnum);
bool mixer_is_valid_soundgen_num(mixer *mixr, int soundgen_num);
bool mixer_is_valid_soundgen_track_num(mixer *mixr, int soundgen_num,
                                       int track_num);
bool mixer_is_valid_scene_num(mixer *mixr, int scene_num);
bool mixer_is_soundgen_in_scene(int soundgen_num, scene *scene_num);

int mixer_add_scene(mixer *mixr, int num_bars);
bool mixer_add_soundgen_track_to_scene(mixer *mixr, int scene_num,
                                       int soundgen_num, int soundgen_track);
bool mixer_rm_soundgen_track_from_scene(mixer *mixr, int scene_num,
                                        int soundgen_num, int soundgen_track);
bool mixer_cp_scene(mixer *mixr, int scene_num_from, int scene_num_to);

synthbase *get_synthbase(soundgenerator *self);
void synth_handle_midi_note(soundgenerator *sg, int note, int velocity,
                            bool update_last_midi);

#endif // MIXER_H
