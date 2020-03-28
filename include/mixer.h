#ifndef MIXER_H
#define MIXER_H

#include <lo/lo.h>
#include <portaudio.h>
#include <portmidi.h>
#include <pthread.h>

#include <ableton_link_wrapper.h>
#include <defjams.h>
#include <digisynth.h>
#include <dxsynth.h>
#include <fx/fx.h>
#include <minisynth.h>
#include <process.hpp>
#include <sbmsg.h>
#include <soundgenerator.h>
#include <table.h>
#include <value_generator.h>

#define MAX_SCENES 100
#define MAX_TRACKS_PER_SCENE 100
#define MAX_NUM_PROC 100
#define MAX_NUM_SOUND_GENERATORS 100
#define MAX_NUM_VALUE_GENERATORS 100
#define NUM_PROGRESSIONS 4

typedef enum
{
    Q32,
    Q16,
    Q8,
    Q4,
    Q2
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
    double *audio_buffer{nullptr};
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

    // std::vector<std::shared_ptr<Process>> processes;
    std::array<std::shared_ptr<Process>, MAX_NUM_PROC> processes_;
    bool proc_initialized_{false};

    std::shared_ptr<SoundGenerator> SoundGenerators[MAX_NUM_SOUND_GENERATORS];
    std::atomic_int soundgen_num; // actual number of SGs

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

    lo_address processing_addr;
};

mixer *new_mixer(double output_latency);

void mixer_ps(mixer *mixr, bool all);
std::string mixer_status_env(mixer *mixr);
std::string mixer_status_mixr(mixer *mixr);
std::string mixer_status_procz(mixer *mixr, bool all);
std::string mixer_status_sgz(mixer *mixr, bool all);

void mixer_update_bpm(mixer *mixr, int bpm);
void mixer_update_time_unit(mixer *mixr, unsigned int time_type, int val);
void mixer_midi_tick(mixer *mixr);
void mixer_emit_event(mixer *mixr, broadcast_event event);
bool mixer_del_soundgen(mixer *mixr, int soundgen_num);

void mixer_preview_audio(mixer *mixr, char *filename);

int mixer_print_timing_info(mixer *mixr);

int add_minisynth(mixer *mixr);
int add_dxsynth(mixer *mixr);
int add_sample(mixer *mixr, std::string sample_path);
int add_digisynth(mixer *mixr, char *filename);
int add_looper(mixer *mixr, std::string filename, bool loop_mode);

int add_sound_generator(mixer *mixr, std::shared_ptr<SoundGenerator> sg);

void mixer_vol_change(mixer *mixr, float vol);
void vol_change(mixer *mixr, int sig, float vol);
void pan_change(mixer *mixr, int sig, float vol);

void mixer_toggle_midi_mode(mixer *mixr);
void mixer_toggle_key_mode(mixer *mixr);
void mixer_play_scene(mixer *mixr, int scene_num);

void mixer_update_timing_info(mixer *mixr, long long int frame_time);
int mixer_gennext(mixer *mixr, float *out, int frames_per_buffer);

bool mixer_is_valid_process(mixer *mixr, int proc_num);
bool mixer_is_valid_soundgen_num(mixer *mixr, int soundgen_num);
bool mixer_is_valid_fx(mixer *mixr, int soundgen_num, int fx_num);

void mixer_set_notes(mixer *mixr);
void mixer_set_octave(mixer *mixr, int octave);
void mixer_set_bars_per_chord(mixer *mixr, int bars);

double mixer_get_hz_per_bar(mixer *mixr);
double mixer_get_hz_per_timing_unit(mixer *mixr, unsigned int timing_unit);
int mixer_get_ticks_per_cycle_unit(mixer *mixr, unsigned int event_type);
void mixer_set_chord_progression(mixer *mixr, unsigned int prog_num);
void mixer_change_chord(mixer *mixr, unsigned int root,
                        unsigned int chord_type);
int mixer_get_key_from_degree(mixer *mixr, unsigned int scale_degree);
void mixer_enable_print_midi(mixer *mixr, bool b);
void mixer_check_for_midi_messages(mixer *mixr);
void mixer_check_for_audio_action_queue_messages(mixer *mixr);
void mixer_set_midi_bank(mixer *mixr, int num);
void mixer_set_should_progress_chords(mixer *mixr, bool b);
bool should_progress_chords(mixer *mixr, int tick);
void mixer_next_chord(mixer *mixr);

void mixer_help(mixer *mixr);

#endif // MIXER_H
