#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>
#include <portmidi.h>
#include <pthread.h>

#include <ableton_link_wrapper.h>
#include <defjams.h>
#include <dxsynth.h>
#include <fx/fx.h>
#include <minisynth.h>
#include <process.hpp>
#include <soundgenerator.h>
#include <table.h>

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

struct scene
{
    int num_bars_to_play;
    int num_tracks;
    soundgen_track soundgen_tracks[MAX_TRACKS_PER_SCENE];
};

typedef unsigned int compat_key_list[6];

// struct AbletonLink AbletonLink;

struct preview_buffer
{
    char filename[512];
    double *audio_buffer{nullptr};
    int num_channels;
    int audio_buffer_len;
    int audio_buffer_read_idx;
    bool enabled;
};

stereo_val preview_buffer_generate(preview_buffer *buffy);
void preview_buffer_import_file(preview_buffer *buffy, char *filename);

struct file_monitor
{
    std::string function_file_filepath;
    std::time_t function_file_filepath_last_write_time{0};
};

struct DelayedMidiEvent
{
    DelayedMidiEvent() = default;
    DelayedMidiEvent(int target_tick, midi_event event,
                     std::shared_ptr<SoundGenerator> sg)
        : target_tick{target_tick}, event{event}, sg{sg}
    {
    }
    int target_tick{0};
    midi_event event{};
    std::shared_ptr<SoundGenerator> sg{};
};

struct Mixer
{

  public:
    Mixer(double output_latency);

    preview_buffer preview;

    // for importing functions - monitor these files for changes
    std::vector<file_monitor> file_monitors;

    // std::vector<std::shared_ptr<Process>> processes;
    std::array<std::shared_ptr<Process>, MAX_NUM_PROC> processes_ = {};
    bool proc_initialized_{false};

    std::shared_ptr<SoundGenerator>
        sound_generators_[MAX_NUM_SOUND_GENERATORS] = {};
    std::atomic_int soundgen_num{0}; // actual number of SGs

    std::vector<DelayedMidiEvent> _action_items;

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

    void CheckForDelayedEvents();

    void Help();
    void Ps(bool all);

    std::string StatusEnv();
    std::string StatusMixr();
    std::string StatusProcz(bool all = false);
    std::string StatusSgz(bool all);

    void UpdateBpm(int bpm);
    void UpdateTimeUnit(unsigned int time_type, int val);
    void MidiTick();
    void EmitEvent(broadcast_event event);
    bool DelSoundgen(int soundgen_num);

    void PreviewAudio(char *filename);

    void PrintTimingInfo();
    void PrintMidiInfo();

    void AddMinisynth();
    void AddDxsynth();
    void AddSample(std::string sample_path);
    void AddLooper(std::string filename, bool loop_mode);

    void AddSoundGenerator(std::shared_ptr<SoundGenerator> sg);

    void VolChange(float vol);
    void VolChange(int sig, float vol);
    void PanChange(int sig, float vol);

    void ToggleMidiMode();
    void ToggleKeyMode();
    void PlayScene(int scene_num);

    void UpdateTimingInfo(long long int frame_time);
    int GenNext(float *out, int frames_per_buffer);

    bool IsValidProcess(int proc_num);
    bool IsValidSoundgenNum(int soundgen_num);
    bool IsValidFx(int soundgen_num, int fx_num);

    void SetKey(unsigned int key);
    void SetKey(std::string str_key);
    void SetNotes();
    void SetOctave(int octave);
    void SetBarsPerChord(int bars);

    double GetHzPerBar();
    double GetHzPerTimingUnit(unsigned int timing_unit);
    int GetTicksPerCycleUnit(unsigned int event_type);
    void SetChordProgression(unsigned int prog_num);
    void ChangeChord(unsigned int root, unsigned int chord_type);
    int GetKeyFromDegree(unsigned int scale_degree);
    void EnablePrintMidi(bool b);
    void CheckForMidiMessages();
    void CheckForAudioActionQueueMessages();
    void SetMidiBank(int num);
    void SetShouldProgressChords(bool b);
    bool ShouldProgressChords(int tick);
    void NextChord();

    void AddFileToMonitor(std::string filepath);
};

#endif // MIXER_H
