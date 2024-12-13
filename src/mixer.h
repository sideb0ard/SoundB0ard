#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>
#include <portmidi.h>
#include <pthread.h>

#include <ableton/Link.hpp>
#include <array>
#include <filesystem>
#include <process.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "audio_action_queue.h"
#include "defjams.h"
#include "dxsynth.h"
#include "fx/fx.h"
#include "minisynth.h"
#include "soundgenerator.h"
#include "websocket/web_socket_server.h"
#include "xfader.h"

struct PreviewBuffer {
  std::string filename{};
  std::vector<double> audio_buffer{};
  int num_channels{};
  int audio_buffer_len{};
  int audio_buffer_read_idx{};
  bool enabled{};

  StereoVal Generate();
  // void ImportFile(std::string filename);
};

struct file_monitor {
  std::string function_file_filepath;
  std::filesystem::file_time_type function_file_filepath_last_write_time;
};

struct DelayedMidiEvent {
  DelayedMidiEvent() = default;
  DelayedMidiEvent(int target_tick, midi_event event, int sg_idx)
      : target_tick{target_tick}, event{event}, sg_idx{sg_idx} {}
  int target_tick{0};
  midi_event event{};
  int sg_idx{-1};
};

struct Action {
  Action(double start_val, double end_val, int time_taken_ticks,
         std::string action_to_take);
  ~Action() = default;

  void Run();
  bool IsActive() { return active_; }

  double start_val_{0};
  double end_val_{0};
  double cur_val_{0};
  double incr_{0};
  int dir_{1};  // 1 going forward, 0, going downard
  int time_taken_ticks_{0};
  bool has_started_{false};
  bool active_{false};
  std::string action_to_take_{""};
};

struct Mixer {
 public:
  Mixer(WebsocketServer &server);

  PreviewBuffer preview;

  // for importing functions - monitor these files for changes
  std::vector<file_monitor> file_monitors;

  std::array<std::shared_ptr<Process>, MAX_NUM_PROC> processes_ = {};
  bool proc_initialized_{false};

  int sound_generators_idx_{0};
  std::array<std::unique_ptr<SBAudio::SoundGenerator>, MAX_NUM_SOUND_GENERATORS>
      sound_generators_ = {};
  std::array<StereoVal, MAX_NUM_SOUND_GENERATORS> soundgen_cur_val_{};

  std::array<std::shared_ptr<Fx>, kMixerNumSendFx> fx_;

  std::vector<DelayedMidiEvent> _action_items =
      {};  // TODO get rid of this version
  std::vector<std::unique_ptr<AudioActionItem>> delayed_action_items_ = {};

  XFader xfader_;

  std::mutex scheduled_actions_mutex_;
  std::vector<Action> running_actions_{};
  std::multimap<int, Action> scheduled_actions_{};

  std::vector<int> soloed_sound_generator_idz{};

  bool debug_mode{false};

  bool websocket_enabled_{false};

  double bpm{140};
  double bpm_to_be_updated{0};

  mixer_timing_info timing_info = {};
  std::chrono::microseconds mTimeAtLastClick{0};

  double volume{1};

  PortMidiStream *midi_stream;
  bool have_midi_controller;
  std::string midi_controller_name{};
  int midi_target;
  // std::vector<int> midi_targets{};  // sound_generators_ idx
  bool midi_recording = {false};
  bool midi_print = {false};
  std::unordered_map<int, std::string> midi_mapped_controls_ = {};

  WebsocketServer &websocket_server_;

  void AddMidiMapping(int id, std::string param);
  void ResetMidiMappings();
  void PrintMidiMappings();
  void HandleMidiControlMessage(int data1, int data2);

  void AssignSoundGeneratorToMidiController(int soundgen_id);
  void RecordMidiToggle();
  void PrintMidiToggle();
  void ResetMidiRecording();

  void CheckForDelayedEvents();
  void CheckForExternalMidiEvents();

  void ScheduleAction(int when, Action item);
  void RunScheduledActions();

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

  void PreviewAudio(std::unique_ptr<AudioActionItem> action);

  void PrintTimingInfo();
  void PrintDxAlgos();
  void PrintDxRatioz();
  void PrintMidiInfo();
  void PrintFuncAndGenInfo();

  void AddSoundGenerator(std::unique_ptr<SBAudio::SoundGenerator> sg);

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

  void Now();  // time now in midi ticks
  double GetHzPerBar();
  double GetHzPerTimingUnit(unsigned int timing_unit);
  int GetTicksPerCycleUnit(unsigned int event_type);
  void CheckForAudioActionQueueMessages();
  void ProcessActionMessage(std::unique_ptr<AudioActionItem> action);

  void AddFileToMonitor(std::string filepath);

  // for sending websocket to p5.js
  void EnableWebSocket(bool en) { websocket_enabled_ = en; }
};

#endif  // MIXER_H
