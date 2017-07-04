#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>

#include "fx.h"
#include "minisynth.h"
#include "sbmsg.h"
#include "sequencer.h"
#include "sound_generator.h"
#include "table.h"

#define MAX_SCENES 100
#define MAX_TRACKS_PER_SCENE 100

typedef struct environment_variable {
    char key[ENVIRONMENT_KEY_SIZE];
    int val;
} env_var;

typedef struct soundgen_track {
    int soundgen_num;
    int soundgen_track_num;
} soundgen_track;

typedef struct scene {
    int num_bars_to_play;
    int num_tracks;
    soundgen_track soundgen_tracks[MAX_TRACKS_PER_SCENE];
} scene;

typedef unsigned int compat_key_list[6];

typedef struct t_mixer {

    SOUNDGEN **sound_generators;
    int soundgen_num;  // actual number of SGs
    int soundgen_size; // number of memory slots reserved for SGszz

    env_var environment[ENVIRONMENT_ARRAY_SIZE];
    int env_var_count;

    bool debug_mode;

    scene scenes[MAX_SCENES];
    int num_scenes; // actual amount of scenes
    int current_scene;
    int current_scene_bar_count;

    unsigned int midi_control_destination;
    unsigned int m_midi_controller_mode; // to switch control knob routing
    unsigned int m_key_controller_mode;  // to switch key control routing

    int active_midi_soundgen_num;
    int active_midi_soundgen_effect_num;

    double bpm;

    int samples_per_midi_tick;
    int midi_ticks_per_ms;
    int sixteenth_note_tick;
    int cur_sample; // inverse of SAMPLE RATE
    int midi_tick;

    // informational for other sound generators
    unsigned int loop_len_in_samples;
    unsigned int loop_len_in_ticks;

    bool start_of_loop; // true for one sample during loop time
    bool is_thirtysecond;
    bool is_sixteenth;
    bool is_eighth;
    bool is_quarter;
    bool is_midi_tick;

    bool scene_mode;
    bool scene_start_pending;

    double volume;

    unsigned int key;

} mixer;

mixer *new_mixer(void);

void mixer_ps(mixer *mixr);
void mixer_update_bpm(mixer *mixr, int bpm);
bool mixer_del_soundgen(mixer *mixr, int soundgen_num);
const compat_key_list *mixer_get_compat_notes(mixer *mixr);
void mixer_generate_melody(mixer *mixr);

int add_algorithm(char *line);
int add_bytebeat(mixer *mixr, char *pattern);
int mixer_add_spork(mixer *mixr, double freq);
int mixer_add_synthdrum(mixer *mixr, int pattern);
int add_chaosmonkey(int soundgen);
int add_minisynth(mixer *mixr);
// int add_seq_char_pattern(mixer *mixr, char *filename, char *pattern);
int add_seq_euclidean(mixer *mixr, char *filename, int num_beats,
                      bool start_on_first_beat);
int add_looper(mixer *mixr, char *filename, double loop_len);
int add_sound_generator(mixer *mixr, SOUNDGEN *sg);
int add_effect(mixer *mixr);
void mixer_vol_change(mixer *mixr, float vol);
void vol_change(mixer *mixr, int sig, float vol);
void freq_change(mixer *mixr, int sig, float freq);

void mixer_toggle_midi_mode(mixer *mixr);
void mixer_toggle_key_mode(mixer *mixr);
bool mixer_is_valid_soundgen_num(mixer *mixr, int soundgen_num);
void mixer_play_scene(mixer *mixr, int scene_num);

void update_environment(char *key, int val);
int get_environment_val(char *key, int *return_val);

double mixer_gennext(mixer *mixr);

bool mixer_is_valid_soundgen_track_num(mixer *mixr, int soundgen_num,
                                       int track_num);
int mixer_add_scene(mixer *mixr, int num_bars);
bool mixer_add_soundgen_track_to_scene(mixer *mixr, int scene_num,
                                       int soundgen_num, int soundgen_track);
bool mixer_rm_soundgen_track_from_scene(mixer *mixr, int scene_num,
                                        int soundgen_num, int soundgen_track);
bool mixer_is_valid_scene_num(mixer *mixr, int scene_num);
bool mixer_is_soundgen_in_scene(int soundgen_num, scene *scene_num);
bool mixer_cp_scene(mixer *mixr, int scene_num_from, int scene_num_to);

#endif // MIXER_H
