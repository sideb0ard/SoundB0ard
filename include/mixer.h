#ifndef MIXER_H
#define MIXER_H

#include <audio_action_queue.h>
#include <defjams.h>
#include <dxsynth.h>
#include <fx/fx.h>
#include <minisynth.h>
#include <portaudio.h>
#include <portmidi.h>
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
  // void ImportFile(std::string filename);
};

struct file_monitor {
  std::string function_file_filepath;
  std::time_t function_file_filepath_last_write_time{0};
};

struct DelayedMidiEvent {
  DelayedMidiEvent() = default;
  DelayedMidiEvent(int target_tick, midi_event event,
                   std::shared_ptr<SBAudio::SoundGenerator> sg)
      : target_tick{target_tick}, event{event}, sg{sg} {}
  int target_tick{0};
  midi_event event{};
  std::shared_ptr<SBAudio::SoundGenerator> sg{};
};

struct Mixer {
 public:
  Mixer();

  PreviewBuffer preview;

  // for importing functions - monitor these files for changes
  std::vector<file_monitor> file_monitors;

  std::array<std::shared_ptr<Process>, MAX_NUM_PROC> processes_ = {};
  bool proc_initialized_{false};

  std::vector<std::shared_ptr<SBAudio::SoundGenerator>> sound_generators_ = {};

  std::vector<DelayedMidiEvent> _action_items =
      {};  // TODO get rid of this version
  std::vector<audio_action_queue_item> _delayed_action_items = {};

  stereo_val soundgen_cur_val[MAX_NUM_SOUND_GENERATORS] = {};
  double soundgen_volume[MAX_NUM_SOUND_GENERATORS] = {};

  int soloed_sound_generator_idx{-1};

  bool debug_mode{false};

  double bpm{140};
  double bpm_to_be_updated{0};

  mixer_timing_info timing_info = {};
  std::chrono::microseconds mTimeAtLastClick{0};

  double volume{0.7};

  PortMidiStream *midi_stream;
  bool have_midi_controller;
  std::string midi_controller_name{};
  int midi_target;
  // std::vector<int> midi_targets{};  // sound_generators_ idx
  bool midi_recording = {false};

  void AssignSoundGeneratorToMidiController(int soundgen_id);
  void RecordMidiToggle();

  void CheckForDelayedEvents();
  void CheckForExternalMidiEvents();

  void Help();
  void Ps(bool all);

  std::string StatusEnv();
  std::string StatusMixr();
  std::string StatusProcz(bool all = false);
  std::string StatusSgz(bool all);

  void PrintRecordingBuffer();
  MultiEventMidiPattern RecordingBuffer() { return recording_buffer_; }
  MultiEventMidiPattern recording_buffer_;

  void UpdateBpm(int bpm);
  void UpdateTimeUnit(unsigned int time_type, int val);
  void MidiTick();
  void EmitEvent(broadcast_event event);
  bool DelSoundgen(int soundgen_num);

  void PreviewAudio(audio_action_queue_item action);

  void PrintTimingInfo();
  void PrintMidiInfo();
  void PrintFuncAndGenInfo();

  void AddSoundGenerator(std::shared_ptr<SBAudio::SoundGenerator> sg);

  void VolChange(float vol);
  void VolChange(int sig, float vol);
  void PanChange(int sig, float vol);

  void UpdateTimingInfo(long long int frame_time);
  int GenNext(float *out, int frames_per_buffer,
              ableton::Link::SessionState &sessionState, const double quantum,
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
