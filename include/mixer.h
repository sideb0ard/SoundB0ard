#ifndef MIXER_H
#define MIXER_H

#include <audio_action_queue.h>
#include <defjams.h>
#include <dxsynth.h>
#include <fx/fx.h>
#include <minisynth.h>
#include <portaudio.h>
#include <pthread.h>
#include <soundgenerator.h>

#include <ableton/Link.hpp>
#include <process.hpp>

struct PreviewBuffer {
  std::string filename{};
  std::vector<double> audio_buffer{};
  int num_channels{};
  int audio_buffer_len{};
  int audio_buffer_read_idx{};
  bool enabled{};

  stereo_val Generate();
  void ImportFile(std::string filename);
};

struct file_monitor {
  std::string function_file_filepath;
  std::time_t function_file_filepath_last_write_time{0};
};

struct DelayedMidiEvent {
  DelayedMidiEvent() = default;
  DelayedMidiEvent(int target_tick, midi_event event,
                   std::shared_ptr<SoundGenerator> sg)
      : target_tick{target_tick}, event{event}, sg{sg} {}
  int target_tick{0};
  midi_event event{};
  std::shared_ptr<SoundGenerator> sg{};
};

struct Mixer {
 public:
  Mixer();

  PreviewBuffer preview;

  // for importing functions - monitor these files for changes
  std::vector<file_monitor> file_monitors;

  std::array<std::shared_ptr<Process>, MAX_NUM_PROC> processes_ = {};
  bool proc_initialized_{false};

  std::vector<std::shared_ptr<SoundGenerator>> sound_generators_ = {};

  std::vector<DelayedMidiEvent> _action_items =
      {};  // TODO get rid of this version
  std::vector<audio_action_queue_item> _delayed_action_items = {};

  // AbletonLink *m_ableton_link{nullptr};

  stereo_val soundgen_cur_val[MAX_NUM_SOUND_GENERATORS] = {};
  double soundgen_volume[MAX_NUM_SOUND_GENERATORS] = {};

  int soloed_sound_generator_idx{-1};

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

  void PreviewAudio(std::string filename);

  void PrintTimingInfo();
  void PrintMidiInfo();
  void PrintFuncAndGenInfo();

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
  int GenNext(float *out, int frames_per_buffer,
              const ableton::Link::SessionState sessionState,
              const double quantum,
              const std::chrono::microseconds beginHostTime);

  bool IsValidProcess(int proc_num);
  bool IsValidSoundgenNum(int soundgen_num);
  bool IsValidFx(int soundgen_num, int fx_num);

  double GetHzPerBar();
  double GetHzPerTimingUnit(unsigned int timing_unit);
  int GetTicksPerCycleUnit(unsigned int event_type);
  void CheckForAudioActionQueueMessages();
  void ProcessActionMessage(audio_action_queue_item action);

  void AddFileToMonitor(std::string filepath);
};

#endif  // MIXER_H
