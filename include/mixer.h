#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>
#include <pthread.h>

#include <ableton_link_wrapper.h>
#include <defjams.h>
#include <dxsynth.h>
#include <fx/fx.h>
#include <minisynth.h>
#include <process.hpp>
#include <soundgenerator.h>
#include <table.h>

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

    std::vector<DelayedMidiEvent> _action_items = {};

    AbletonLink *m_ableton_link{nullptr};

    stereo_val soundgen_cur_val[MAX_NUM_SOUND_GENERATORS] = {};
    double soundgen_volume[MAX_NUM_SOUND_GENERATORS] = {};

    bool debug_mode{false};

    double bpm{140};

    mixer_timing_info timing_info = {};

    double volume{0.7};

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
    void AddDrumSynth();
    void AddSample(std::string sample_path);
    void AddLooper(std::string filename, bool loop_mode);

    void AddSoundGenerator(std::shared_ptr<SoundGenerator> sg);

    void VolChange(float vol);
    void VolChange(int sig, float vol);
    void PanChange(int sig, float vol);

    void UpdateTimingInfo(long long int frame_time);
    int GenNext(float *out, int frames_per_buffer);

    bool IsValidProcess(int proc_num);
    bool IsValidSoundgenNum(int soundgen_num);
    bool IsValidFx(int soundgen_num, int fx_num);

    double GetHzPerBar();
    double GetHzPerTimingUnit(unsigned int timing_unit);
    int GetTicksPerCycleUnit(unsigned int event_type);
    void CheckForAudioActionQueueMessages();

    void AddFileToMonitor(std::string filepath);
};

#endif // MIXER_H
