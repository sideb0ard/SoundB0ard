#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>
#include <portmidi.h>
#include <pthread.h>

#include <ableton/Link.hpp>
#include <array>
#include <chrono>
#include <algorithm>

// Forward declarations to avoid namespace issues
namespace ableton { class Link; }
class WebsocketServer;
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
// #include "websocket/web_socket_server.h"  // TODO: Fix websocket compilation issues
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
  bool IsActive() {
    return active_;
  }

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
  Mixer();  // WebSocket server temporarily disabled

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

  // WebsocketServer &websocket_server_;  // Temporarily disabled

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
  MultiEventMidiPattern RecordingBuffer() {
    return recording_buffer_;
  }
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
  
  // Template function implementation must be in header
  static constexpr auto MIDI_TICK_FRAC_OF_BEAT = 1. / 960;
  
  template<typename SessionState>
  int GenNext(float *out, int frames_per_buffer,
              SessionState &sessionState, const double quantum,
              const std::chrono::microseconds beginHostTime) {
    using namespace std::chrono;
    
    // The number of microseconds that elapse between samples
    constexpr auto microsPerSample = 1e6 / SAMPLE_RATE;

    int return_bpm = 0;
    if (bpm_to_be_updated > 0) {
      return_bpm = bpm_to_be_updated;
      bpm_to_be_updated = 0;
    }
    xfader_.Update(timing_info);

    for (int i = 0, j = 0; i < frames_per_buffer; i++, j += 2) {
      double output_left = 0.0;
      double output_right = 0.0;

      timing_info.cur_sample++;

      if (preview.enabled) {
        StereoVal preview_audio = preview.Generate();
        output_left += preview_audio.left * 0.6;
        output_right += preview_audio.right * 0.6;
      }

      const auto hostTime =
          beginHostTime +
          microseconds(llround(static_cast<double>(i) * microsPerSample));

      timing_info.is_midi_tick = false;
      auto beat_time = sessionState.beatAtTime(hostTime, quantum);
      if (beat_time >= 0.) {
        if (beat_time >= timing_info.time_of_next_midi_tick) {
          timing_info.time_of_next_midi_tick =
              (double)((int)beat_time) +
              ((timing_info.midi_tick % PPQN) * Mixer::MIDI_TICK_FRAC_OF_BEAT);
          timing_info.is_midi_tick = true;
          MidiTick();
          timing_info.midi_tick++;
        }
      }

      StereoVal fx_delay_send{};
      StereoVal fx_reverb_send{};
      StereoVal fx_distort_send{};
      // Cache the size to avoid race conditions with other threads modifying it
      int safe_generators_count =
          (sound_generators_idx_ < static_cast<int>(sound_generators_.size()))
              ? sound_generators_idx_
              : static_cast<int>(sound_generators_.size());

      for (int k = 0; k < safe_generators_count; k++) {
        // Additional bounds check
        if (k >= sound_generators_.size()) break;

        auto &sg = sound_generators_[k];
        // Null check before dereferencing
        if (!sg) {
          soundgen_cur_val_[k] = StereoVal{};
          continue;
        }
        soundgen_cur_val_[k] = sg->GenNext(timing_info);

        bool collect_value = false;
        // if nothing is soloed, or this sg is in the solo group,
        // collect its output value
        if (soloed_sound_generator_idz.empty() ||
            std::find(soloed_sound_generator_idz.begin(),
                      soloed_sound_generator_idz.end(),
                      k) != soloed_sound_generator_idz.end()) {
          collect_value = true;
        }

        if (collect_value) {
          output_left += soundgen_cur_val_[k].left * xfader_.GetValueFor(k);
          output_right += soundgen_cur_val_[k].right * xfader_.GetValueFor(k);

          fx_delay_send += soundgen_cur_val_[k] * sg->mixer_fx_send_intensity_[0];
          fx_reverb_send +=
              soundgen_cur_val_[k] * sg->mixer_fx_send_intensity_[1];
          fx_distort_send +=
              soundgen_cur_val_[k] * sg->mixer_fx_send_intensity_[2];
        }
      }

      auto delay_val = fx_[0]->Process(fx_delay_send);
      auto reverb_val = fx_[1]->Process(fx_reverb_send);
      auto distort_val = fx_[2]->Process(fx_distort_send);
      output_left += (delay_val.left + reverb_val.left + distort_val.left);
      output_right += (delay_val.right + reverb_val.right + distort_val.right);

      out[j] = volume * output_left;
      out[j + 1] = volume * output_right;
    }

    // WebSocket temporarily disabled
    // if (websocket_enabled_) {
    //   websocket_server_.sendData(out, 2 * frames_per_buffer * sizeof(float));
    // }

    return return_bpm;
  }

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
  void EnableWebSocket(bool en) {
    websocket_enabled_ = en;
  }
};

#endif  // MIXER_H
