#include <audio_action_queue.h>
#include <defjams.h>
#include <drum_synth.h>
#include <drumsampler.h>
#include <dxsynth.h>
#include <event_queue.h>
#include <fx/distortion.h>
#include <fx/envelope.h>
#include <fx/fx.h>
#include <fx/reverb.h>
#include <granulator.h>
#include <math.h>
#include <minisynth.h>
#include <mixer.h>
#include <obliquestrategies.h>
#include <portaudio.h>
#include <portmidi.h>
#include <soundgenerator.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>

#include <filereader.hpp>
#include <interpreter/object.hpp>
#include <interpreter/sound_cmds.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <tsqueue.hpp>

namespace {

std::vector<std::string> env_names = {"att", "dec", "rel"};

std::pair<double, double> GetBoundaries(const std::string &param) {
  if (param.find("pct") != std::string::npos) return std::make_pair(0, 100);
  if (std::any_of(env_names.begin(), env_names.end(),
                  [&param](const std::string &p) {
                    return param.find(p) != std::string::npos;
                  })) {
    return std::make_pair(1, 5000);
  }
  if (param.find("fc") != std::string::npos) return std::make_pair(80, 1800);
  if (param.find("fq") != std::string::npos) return std::make_pair(0.5, 9.7);
  if (param.find("det") != std::string::npos) return std::make_pair(-100, 100);
  if (param.find("int") != std::string::npos) return std::make_pair(-1, 1);
  if (param.find("rate") != std::string::npos) return std::make_pair(0.02, 20);
  if (param.find("ratio") != std::string::npos)
    return std::make_pair(-0.99, 0.99);
  if (param.find("rat") != std::string::npos) return std::make_pair(0.1, 26);
  if (param.find("out") != std::string::npos) return std::make_pair(0, 100);
  if (param.find("algo") != std::string::npos) return std::make_pair(0, 7);
  if (param.find("grain") != std::string::npos) return std::make_pair(0, 100);
  if (param.find("wav") != std::string::npos) return std::make_pair(0, 7);
  if (param.find("porta") != std::string::npos) return std::make_pair(0, 5000);
  if (param.find("delayms") != std::string::npos)
    return std::make_pair(0, 2000);
  if (param.find("fb") != std::string::npos) return std::make_pair(0, 99);
  if (param.find("wetmx") != std::string::npos) return std::make_pair(0, 0.99);

  return std::make_pair(0, 1);
}

}  // namespace

std::unique_ptr<Mixer> global_mixr;
extern std::shared_ptr<object::Environment> global_env;
extern Tsqueue<event_queue_item> process_event_queue;
extern Tsqueue<std::string> repl_queue;
extern Tsqueue<std::unique_ptr<AudioActionItem>> audio_queue;
extern Tsqueue<int> audio_reply_queue;
extern Tsqueue<std::string> eval_command_queue;

Action::Action(double start_val, double end_val, int time_taken_ticks,
               std::string action_to_take)
    : start_val_{start_val},
      end_val_{end_val},
      cur_val_{start_val},
      time_taken_ticks_{time_taken_ticks},
      action_to_take_{action_to_take} {
  if (time_taken_ticks_ == 0) {
    incr_ = 1;
  } else {
    double diff = std::abs(start_val_ - end_val_);
    incr_ = diff / time_taken_ticks_;
  }
  if (start_val_ > end_val_) {
    dir_ = 0;  // downward
    incr_ *= -1;
  }
  if (incr_ == 0) {
    std::cout << "Nah mate, nae zeros allowed!\n";
  } else {
    active_ = true;
  }
}

void Action::Run() {
  if (active_) {
    cur_val_ += incr_;
    if ((dir_ && (cur_val_ >= end_val_)) || (!dir_ && (cur_val_ <= end_val_))) {
      cur_val_ = end_val_;
      active_ = false;
    }
    std::string new_cmd =
        ReplaceString(action_to_take_, "%", std::to_string(cur_val_));
    eval_command_queue.try_push(new_cmd);
  }
}

Mixer::Mixer() {  // WebSocket server temporarily disabled
  UpdateBpm(DEFAULT_BPM);

  for (int i = 0; i < MAX_NUM_PROC; i++)
    processes_[i] = std::make_shared<Process>();
  proc_initialized_ = true;

  fx_[0] = std::make_shared<StereoDelay>();
  fx_[1] = std::make_shared<Reverb>();
  fx_[2] = std::make_shared<Distortion>();

  // the lifetime of these booleans is a single sample
  timing_info.cur_sample = -1;
  timing_info.midi_tick = -1;
  timing_info.sixteenth_note_tick = -1;
  timing_info.loop_beat = 0;
  timing_info.time_of_next_midi_tick = 0;
  timing_info.has_started = false;
  timing_info.is_midi_tick = true;
  timing_info.is_start_of_loop = true;
  timing_info.is_end_of_loop = false;
  timing_info.is_thirtysecond = true;
  timing_info.is_twentyfourth = true;
  timing_info.is_sixteenth = true;
  timing_info.is_twelth = true;
  timing_info.is_eighth = true;
  timing_info.is_sixth = true;
  timing_info.is_quarter = true;
  timing_info.is_third = true;

  std::string contents = ReadFileContents(kStartupConfigFile);
  eval_command_queue.push(contents);
}

std::string Mixer::StatusMixr() {
  //  clang-format off
  std::stringstream ss;

  ss << COOL_COLOR_GREEN << ":::::::::::::::: vol:" << ANSI_COLOR_WHITE
     << volume << COOL_COLOR_GREEN << " bpm:" << ANSI_COLOR_WHITE << bpm
     << COOL_COLOR_GREEN << " looplen:" << ANSI_COLOR_WHITE << 3840
     << COOL_COLOR_GREEN << " midi_device:" << ANSI_COLOR_WHITE
     << (have_midi_controller ? "true" : "false") << COOL_COLOR_GREEN
     << " websock:" << ANSI_COLOR_WHITE
     << (websocket_enabled_ ? "true" : "false") << COOL_COLOR_GREEN
     << " ::::::::::::::::::::::\n";

  if (fx_[0]) {
    ss << COOL_COLOR_GREEN << ":::::::::::::::: " << COOL_COLOR_ORANGE
       << "delay: " << fx_[0]->Status() << std::endl;
  }
  if (fx_[1]) {
    ss << COOL_COLOR_GREEN << ":::::::::::::::: " << COOL_COLOR_ORANGE
       << "reverb: " << fx_[1]->Status() << std::endl;
  }
  if (fx_[2]) {
    ss << COOL_COLOR_GREEN << ":::::::::::::::: " << COOL_COLOR_ORANGE
       << "distort: " << fx_[2]->Status() << std::endl;
  }

  if (global_env) {
    ss << COOL_COLOR_GREEN << ":::::::::::::::: " << COOL_COLOR_ORANGE
       << "xfader: " << xfader_.Status(global_env->GetSoundGeneratorsById())
       << std::endl;
  }
  ss << ANSI_COLOR_WHITE;
  // clang-format on

  return ss.str();
}

std::string Mixer::StatusProcz(bool all) {
  std::stringstream ss;
  ss << COOL_COLOR_ORANGE << "\n[" << ANSI_COLOR_WHITE << "Procz"
     << COOL_COLOR_ORANGE << "]\n";

  for (int i = 0; i < MAX_NUM_PROC; i++) {
    // Make a copy of the shared_ptr to avoid races
    auto p = processes_[i];
    if (!p) continue;  // Skip if null

    // Check if we should display this process
    if (p->active_ || all) {
      ss << ANSI_COLOR_WHITE << "p" << i << ANSI_COLOR_RESET << " "
         << p->Status() << std::endl;
    }
  }

  return ss.str();
}
std::string Mixer::StatusEnv() {
  std::stringstream ss;
  ss << COOL_COLOR_GREEN << "\n[" << ANSI_COLOR_WHITE << "Varz"
     << COOL_COLOR_GREEN << "]" << std::endl;

  if (!global_env) {
    ss << ANSI_COLOR_RED << "ERROR: global_env is null!" << ANSI_COLOR_RESET
       << std::endl;
    return ss.str();
  }

  ss << global_env->Debug();

  // Get the name->index mapping (this doesn't need the sg mutex)
  std::map<std::string, int> soundgens = global_env->GetSoundGeneratorsByName();

  // Read from cached status - no need to lock sound_generators_mutex_
  std::lock_guard<std::mutex> lock(cached_status_mutex_);

  for (auto &[var_name, sg_idx] : soundgens) {
    if (sg_idx >= 0 && sg_idx < cached_sg_count_.load()) {
      auto &cached = cached_sg_status_[sg_idx];
      ss << ANSI_COLOR_WHITE << var_name << ANSI_COLOR_RESET " = "
         << cached.status << ANSI_COLOR_RESET << std::endl;

      std::stringstream margin;
      size_t len_var = var_name.size();
      for (size_t i = 0; i < len_var; i++) {
        margin << " ";
      }
      margin << "   ";  // for the ' = '

      // Read from cached FX status
      for (int i = 0; i < cached.effects_num; i++) {
        ss << margin.str();
        if (cached.fx[i].enabled)
          ss << COOL_COLOR_YELLOW;
        else
          ss << ANSI_COLOR_RESET;

        ss << "fx" << i << " " << cached.fx[i].status << std::endl;
      }
      ss << ANSI_COLOR_RESET;
    }
  }
  return ss.str();
}

std::string Mixer::StatusSgz(bool all) {
  std::stringstream ss;

  // Read from cached status - no need to lock sound_generators_mutex_
  std::lock_guard<std::mutex> lock(cached_status_mutex_);

  int count = cached_sg_count_.load();
  if (count > 0) {
    ss << COOL_COLOR_GREEN << "\n[" << ANSI_COLOR_WHITE << "sound generators"
       << COOL_COLOR_GREEN << "]\n";

    for (int i = 0; i < count; i++) {
      auto &cached = cached_sg_status_[i];
      if ((cached.active && cached.volume > 0.0) || all) {
        ss << COOL_COLOR_GREEN << "[" << ANSI_COLOR_WHITE << "s" << i
           << COOL_COLOR_GREEN << "] " << ANSI_COLOR_RESET;
        ss << cached.info << ANSI_COLOR_RESET << "\n";

        if (cached.effects_num > 0) {
          ss << "      ";
          for (int j = 0; j < cached.effects_num; j++) {
            if (cached.fx[j].enabled)
              ss << COOL_COLOR_YELLOW;
            else
              ss << ANSI_COLOR_RESET;
            ss << "\n[fx " << i << ":" << j << cached.fx[j].status << "]";
          }
          ss << ANSI_COLOR_RESET;
        }
        ss << "\n\n";
      }
    }
  }
  return ss.str();
}

void Mixer::PrintFuncAndGenInfo() {
  std::stringstream ss;
  ss << global_env->ListFuncsAndGen();
  repl_queue.push(ss.str());
}

void Mixer::Ps(bool all) {
  std::stringstream ss;
  ss << get_string_logo();
  ss << StatusMixr();
  ss << StatusEnv();
  ss << StatusProcz();
  // ss << mixer_status_procz(mixr, all);
  ss << ANSI_COLOR_RESET;

  if (all) ss << StatusSgz(all);

  repl_queue.push(ss.str());
}

void Mixer::Now() {
  audio_reply_queue.push(timing_info.midi_tick);
}

void Mixer::Help() {
  std::string reply;
  if (rand() % 100 > 90) {
    reply = oblique_strategy();
  } else {
    std::stringstream ss;
    ss << ANSI_COLOR_WHITE;
    ss << "###### Haaaalp! ################################\n";
    ss << ANSI_COLOR_RESET;

    reply = ss.str();
  }
  repl_queue.push(reply);
}

void Mixer::EmitEvent(broadcast_event event) {
  event_queue_item ev;
  ev.type = Event::TIMING_EVENT;
  ev.timing_info = timing_info;
  process_event_queue.push(ev);

  std::shared_lock<std::shared_mutex> lock(
      sound_generators_mutex_);  // Shared lock for read
  for (int i = 0; i < sound_generators_idx_; i++) {
    auto &sg = sound_generators_[i];
    if (sg) {
      sg->EventNotify(event, timing_info);
      if (sg->effects_num > 0) {
        // Shared lock to safely read effects array
        std::shared_lock<std::shared_mutex> fx_lock(sg->effects_mutex_);
        for (int j = 0; j < sg->effects_num; j++) {
          if (sg->effects_[j]) {
            auto f = sg->effects_[j];
            f->EventNotify(event, timing_info);
          }
        }
      }
    }
  }
}

void Mixer::UpdateBpm(int new_bpm) {
  bpm = new_bpm;
  bpm_to_be_updated = new_bpm;
  timing_info.bpm = bpm;
  // timing_info.frames_per_midi_tick = (60.0 / bpm * SAMPLE_RATE) / PPQN;
  timing_info.frames_per_midi_tick = (bpm / 60.0 * SAMPLE_RATE) / PPQN;
  timing_info.loop_len_in_frames = timing_info.frames_per_midi_tick * PPBAR;
  timing_info.loop_len_in_ticks = PPBAR;

  // hmm, not sure if these are correct!
  timing_info.ms_per_midi_tick = 60000.0 / (bpm * PPQN);
  timing_info.midi_ticks_per_ms = PPQN / (60000.0 / bpm);

  timing_info.size_of_thirtysecond_note =
      (PPSIXTEENTH / 2) * timing_info.frames_per_midi_tick;
  timing_info.size_of_sixteenth_note =
      timing_info.size_of_thirtysecond_note * 2;
  timing_info.size_of_eighth_note = timing_info.size_of_sixteenth_note * 2;
  timing_info.size_of_quarter_note = timing_info.size_of_eighth_note * 2;

  EmitEvent((broadcast_event){.type = TIME_BPM_CHANGE, .sequencer_src = 0});
}

void Mixer::VolChange(float vol) {
  if (vol >= 0.0 && vol <= 1.0) {
    volume = vol;
  }
}

void Mixer::VolChange(int sg, float vol) {
  if (!IsValidSoundgenNum(sg)) {
    printf("Nah mate, returning\n");
    return;
  }
  sound_generators_[sg]->SetVolume(vol);
}

void Mixer::PanChange(int sg, float val) {
  if (!IsValidSoundgenNum(sg)) {
    printf("Nah mate, returning\n");
    return;
  }
  sound_generators_[sg]->SetPan(val);
}

void Mixer::AddSoundGenerator(std::unique_ptr<SBAudio::SoundGenerator> sg) {
  std::unique_lock<std::shared_mutex> lock(
      sound_generators_mutex_);  // Exclusive lock for write
  int soundgen_id = sound_generators_idx_;
  if (soundgen_id < MAX_NUM_SOUND_GENERATORS) {
    sg->soundgen_id_ = soundgen_id;
    sound_generators_[sound_generators_idx_++] = std::move(sg);
  } else {
    std::cout << "NAH, FULL MATe!\n";
    soundgen_id = -1;
  }
  audio_reply_queue.push(soundgen_id);
}

void Mixer::MidiTick() {
  timing_info.is_thirtysecond = false;
  timing_info.is_twentyfourth = false;
  timing_info.is_sixteenth = false;
  timing_info.is_twelth = false;
  timing_info.is_eighth = false;
  timing_info.is_sixth = false;
  timing_info.is_quarter = false;
  timing_info.is_third = false;
  timing_info.is_start_of_loop = false;
  timing_info.is_end_of_loop = false;

  int cur_tick = timing_info.midi_tick % PPBAR;
  if (cur_tick == 0) {
    timing_info.is_start_of_loop = true;
  }
  if (cur_tick == PPBAR - 1) {
    timing_info.is_end_of_loop = true;
  }

  if (timing_info.midi_tick % 120 == 0) {
    timing_info.is_thirtysecond = true;

    if (timing_info.midi_tick % 240 == 0) {
      timing_info.is_sixteenth = true;
      timing_info.sixteenth_note_tick++;

      if (timing_info.midi_tick % 480 == 0) {
        timing_info.is_eighth = true;

        if (timing_info.midi_tick % PPQN == 0) {
          timing_info.is_quarter = true;
        }
      }
    }
  }

  CheckForAudioActionQueueMessages();
  CheckForExternalMidiEvents();

  repl_queue.push("tick");
  EmitEvent((broadcast_event){.type = TIME_MIDI_TICK, .sequencer_src = 0});
  // lo_send(processing_addr, "/bpm", NULL);
  CheckForDelayedEvents();

  RunScheduledActions();

  // Update cached status on every beat for lock-free status reads
  if (timing_info.is_quarter) {
    UpdateCachedStatus();
  }
}

// GenNext implementation moved to header as template function

void Mixer::UpdateCachedStatus() {
  // Called from audio thread on every beat - updates cached status info
  // Uses try_lock to never block the audio thread
  if (!cached_status_mutex_.try_lock()) {
    return;  // Skip this update if UI thread is reading
  }

  // We have the sound_generators_mutex_ shared lock from GenNext context
  // but we're in MidiTick which doesn't hold it, so we need to acquire it
  if (!sound_generators_mutex_.try_lock_shared()) {
    cached_status_mutex_.unlock();
    return;  // Skip if we can't get the lock
  }

  int count = sound_generators_idx_.load();
  for (int i = 0; i < count; i++) {
    auto &sg = sound_generators_[i];
    if (sg) {
      cached_sg_status_[i].info = sg->Info();
      cached_sg_status_[i].status = sg->Status();
      cached_sg_status_[i].active = sg->active;
      cached_sg_status_[i].volume = sg->GetVolume();
      cached_sg_status_[i].effects_num = sg->effects_num.load();

      // Cache FX status - try to get the effects lock
      if (sg->effects_mutex_.try_lock_shared()) {
        for (int j = 0; j < sg->effects_num; j++) {
          if (sg->effects_[j]) {
            cached_sg_status_[i].fx[j].status = sg->effects_[j]->Status();
            cached_sg_status_[i].fx[j].enabled = sg->effects_[j]->enabled_;
          }
        }
        sg->effects_mutex_.unlock_shared();
      }
    }
  }
  cached_sg_count_.store(count);

  sound_generators_mutex_.unlock_shared();
  cached_status_mutex_.unlock();
}

bool Mixer::DelSoundgen(int soundgen_num) {
  std::unique_lock<std::shared_mutex> lock(
      sound_generators_mutex_);  // Exclusive lock for write
  if (IsValidSoundgenNum(soundgen_num)) {
    printf("MIXR!! Deleting SOUND GEN %d\n", soundgen_num);
    sound_generators_[soundgen_num] = nullptr;
  }
  return true;
}

bool Mixer::IsValidSoundgenNum(int sg_num) {
  if (sg_num >= 0 && sg_num < sound_generators_idx_ &&
      sg_num < static_cast<int>(sound_generators_.size()) &&
      sound_generators_[sg_num])
    return true;

  return false;
}

bool Mixer::IsValidFx(int soundgen_num, int fx_num) {
  if (IsValidSoundgenNum(soundgen_num)) {
    const auto &sg = sound_generators_[soundgen_num];
    // Shared lock to safely read effects array
    std::shared_lock<std::shared_mutex> lock(sg->effects_mutex_);
    if (fx_num >= 0 && fx_num < sg->effects_num && sg->effects_[fx_num])
      return true;
  }
  return false;
}

void Mixer::PrintDxAlgos() {
  std::stringstream ss;
  ss << ANSI_COLOR_WHITE "DX Algos:\n";
  ss << "  0.    1.    2.      3.    4.     5.        6.        7.\n\n";
  ss << "  4\n";
  ss << "  |\n";
  ss << "  3     3 4   3       4\n";
  ss << "  |     \\ |   |       |\n";
  ss << "  2       2   2 4   2 3   2  4      4            4\n";
  ss << "  |       |   |/    |/    |  |    / | \\          |\n";
  ss << "  1       1   1     1     1__3   1__2__3   1__2__3   1__2__3__4\n";
  ss << ANSI_COLOR_RESET;
  repl_queue.push(ss.str());
}
void Mixer::PrintDxRatioz() {
  std::stringstream ss;
  ss << ANSI_COLOR_WHITE "DX Ratioz:\n";
  ss << "0.5,   0.71,  0.78,  0.87,  1.0,   1.41,  1.57,  1.73,  2.,    2.82,  "
        "3.,\n";
  ss << "3.14,  3.46,  4.,    4.24,  4.71,  5.,    5.19,  5.65,  6.0,   6.28,  "
        "6.92,\n";
  ss << "7.,    7.07,  7.85,  8.0,   8.48,  8.65,  9.0,   9.42,  9.89,  10.,   "
        "10.38,\n";
  ss << "10.99, 11.0,  11.30, 12.0,  12.11, 12.56, 12.72, 13.0,  13.84, 14.0,  "
        "14.1,\n";
  ss << "14.13, 15.,   15.55, 15.37, 15.70, 16.96, 17.27, 17.30, 18.37, 18.84, "
        "19.03,\n";
  ss << "19.78, 20.41, 20.76, 21.20, 21.98, 22.49, 23.53, 24.22, 25.95\n";
  ss << ANSI_COLOR_RESET;
  repl_queue.push(ss.str());
}

void Mixer::PrintMidiInfo() {
  std::stringstream ss;
  ss << ANSI_COLOR_WHITE "Midi Notes:\n";
  ss << "- C:0  C#:1  D:2  D#:3  E:4  F:5  F#:6  G:7  G#:8  A:9  A#:10 "
        "B:11\n";
  ss << "0 C:12 C#:13 D:14 D#:15 E:16 F:17 F#:18 G:19 G#:20 A:21 A#:22 "
        "B:23\n";
  ss << "1 C:24 C#:25 D:26 D#:27 E:28 F:29 F#:30 G:31 G#:32 A:33 A#:34 "
        "B:35\n";
  ss << "2 C:36 C#:37 D:38 D#:39 E:40 F:41 F#:42 G:43 G#:44 A:45 A#:46 "
        "B:47\n";
  ss << "3 C:48 C#:49 D:50 D#:51 E:52 F:53 F#:54 G:55 G#:56 A:57 A#:58 "
        "B:59\n";
  ss << "4 C:60 C#:61 D:62 D#:63 E:64 F:65 F#:66 G:67 G#:68 A:69 A#:70 "
        "B:71\n";
  ss << "5 C:72 C#:73 D:74 D#:75 E:76 F:77 F#:78 G:79 G#:80 A:81 A#:82 "
        "B:83\n";
  ss << "Chord Progressions: I-IV-V, I-V-vi-IV, I-vi-IV-V, vi-ii-V-I "
        "vi-IV-I-V\n";
  ss << "Chord Mods: None(0), Seventh(1), Seventh Inv(2) Root Inv(3) "
        "Power(4)\n";
  ss << "Key Mods: None(0), Natural Minor(1), Harmonic Minor Inv(2) Melodic "
        "Minor(3) "
        "Phrygian(4)\n";
  ss << "Filters: LPF1, HPF1, LPF2, HPF2, BPF2, BSF2, LPF4, HPF4, BPF4\n";
  ss << "Major Scale: W W H W W W H // Minor Scale: W H W W H W W\n"
     << ANSI_COLOR_RESET;

  repl_queue.push(ss.str());
}

void Mixer::PrintTimingInfo() {
  const mixer_timing_info *info = &timing_info;
  printf("TIMING INFO!\n");
  printf("============\n");
  printf("FRAMES per midi tick:%f\n", info->frames_per_midi_tick);
  printf("MS per MIDI tick:%f\n", info->ms_per_midi_tick);
  printf("TIME of next MIDI tick:%f\n", info->time_of_next_midi_tick);
  printf("SIXTEENTH NOTE tick:%d\n", info->sixteenth_note_tick);
  printf("MIDI tick:%d\n", info->midi_tick);
  printf("CUR SAMPLE:%d\n", info->cur_sample);
  printf("Loop_len_in_ticks:%f\n", info->loop_len_in_ticks);
  printf("Has_started:%d\n", info->has_started);
  printf("Start of loop:%d\n", info->is_start_of_loop);
  printf("Is midi_tick:%d\n", info->is_midi_tick);
}

// called from Process or Repl thread
void Mixer::ScheduleAction(int when, Action item) {
  std::lock_guard<std::mutex> lock(scheduled_actions_mutex_);
  scheduled_actions_.insert(std::make_pair(timing_info.midi_tick + when, item));
}

void Mixer::RunScheduledActions() {
  int now = timing_info.midi_tick;
  if (scheduled_actions_mutex_.try_lock()) {
    for (auto it = scheduled_actions_.begin();
         it != scheduled_actions_.end();) {
      if (now >= it->first) {
        running_actions_.push_back(it->second);
        scheduled_actions_.erase(it++);
      } else {
        ++it;
      }
    }
    scheduled_actions_mutex_.unlock();
  }

  for (auto &a : running_actions_) {
    a.Run();
  }

  std::erase_if(running_actions_, [](Action a) { return !a.IsActive(); });
}

double Mixer::GetHzPerBar() {
  double hz_per_beat = (60. / bpm);
  return hz_per_beat / 4;
}

double Mixer::GetHzPerTimingUnit(unsigned int timing_unit) {
  double return_val = 0;
  double hz_per_beat = (60. / bpm);
  if (timing_unit == Quantize::Q2) return_val = hz_per_beat / 2.;
  if (timing_unit == Quantize::Q4)
    return_val = hz_per_beat;
  else if (timing_unit == Quantize::Q8)
    return_val = hz_per_beat * 2;
  else if (timing_unit == Quantize::Q16)
    return_val = hz_per_beat * 4;
  else if (timing_unit == Quantize::Q32)
    return_val = hz_per_beat * 8;

  return return_val;
}

int Mixer::GetTicksPerCycleUnit(unsigned int event_type) {
  int ticks = 0;
  switch (event_type) {
    case (TIME_START_OF_LOOP_TICK):
      ticks = timing_info.loop_len_in_ticks;
      break;
    case (TIME_MIDI_TICK):
      ticks = 1;
      break;
    case (TIME_QUARTER_TICK):
      ticks = PPQN;
      break;
    case (TIME_EIGHTH_TICK):
      ticks = PPQN / 2;
      break;
    case (TIME_SIXTEENTH_TICK):
      ticks = PPQN / 4;
      break;
    case (TIME_THIRTYSECOND_TICK):
      ticks = PPQN / 8;
      break;
  }
  return ticks;
}

void Mixer::PreviewAudio(std::unique_ptr<AudioActionItem> action) {
  if (action->buffer.size() > 0) {
    preview.filename = action->preview_filename;
    preview.audio_buffer_len = action->audio_buffer_details.buffer_length;
    preview.audio_buffer = std::move(action->buffer);
    preview.num_channels = action->audio_buffer_details.num_channels;
    preview.audio_buffer_read_idx = 0;
    preview.enabled = true;
  }
}

StereoVal PreviewBuffer::Generate() {
  StereoVal ret = {.0, .0};
  if (!enabled) return ret;

  if (audio_buffer_read_idx < static_cast<int>(audio_buffer.size())) {
    ret.left = audio_buffer[audio_buffer_read_idx];
    if (num_channels == 1)
      ret.right = ret.left;
    else
      ret.right = audio_buffer[audio_buffer_read_idx + 1];

    audio_buffer_read_idx += num_channels;
    if (audio_buffer_read_idx >= audio_buffer_len) {
      audio_buffer_read_idx = 0;
      enabled = false;
    }
  }

  return ret;
}

void Mixer::ProcessActionMessage(std::unique_ptr<AudioActionItem> action) {
  if (action->type == AudioAction::STATUS) {
    Ps(action->status_all);
  }
  if (action->type == AudioAction::MIDI_MAP)
    AddMidiMapping(action->mapped_id, action->mapped_param);
  else if (action->type == AudioAction::ENABLE_WEBSOCKET) {
    std::cout << "GENERAL VAL:" << action->general_val << " is trei>?"
              << std::endl;
    EnableWebSocket(action->general_val);
  } else if (action->type == AudioAction::MIDI_MAP_SHOW)
    PrintMidiMappings();
  else if (action->type == AudioAction::HELP)
    global_mixr->Help();
  else if (action->type == AudioAction::MONITOR) {
    AddFileToMonitor(action->filepath);
  } else if (action->type == AudioAction::ADD) {
    if (action->sg) {
      AddSoundGenerator(std::move(action->sg));
    }
  } else if (action->type == AudioAction::ADD_FX) {
    if (action->soundgen_num && IsValidSoundgenNum(action->soundgen_num)) {
      auto &sg = sound_generators_[action->soundgen_num];
      if (sg) {
        for (auto fx : action->fx) {
          sg->AddFx(fx);
        }
      }
    }
  } else if (action->type == AudioAction::ADD_BUFFER) {
    if (action->soundgen_num && IsValidSoundgenNum(action->soundgen_num)) {
      auto &sg = sound_generators_[action->soundgen_num];
      if (sg && sg->type == LOOPER_TYPE) {
        sg->AddBuffer(std::move(action->fb));
      }
    }
  } else if (action->type == AudioAction::BPM) {
    UpdateBpm(action->new_bpm);
  } else if (action->type == AudioAction::NOW) {
    Now();
  } else if (action->type == AudioAction::VOLUME) {
    VolChange(action->new_volume);
  } else if (action->type == AudioAction::MIDI_NOTE_ON ||
             action->type == AudioAction::MIDI_NOTE_ON_DELAYED) {
    if (IsValidSoundgenNum(action->soundgen_num)) {
      auto &sg = sound_generators_[action->soundgen_num];

      for (auto midinum : action->notes) {
        midi_event event_on =
            new_midi_event(MIDI_ON, midinum, action->velocity);
        event_on.source = EXTERNAL_OSC;
        event_on.dur = action->duration;

        // used later for MIDI OFF MESSAGE
        int midi_note_on_time = timing_info.midi_tick;

        if (action->type == AudioAction::MIDI_NOTE_ON_DELAYED) {
          midi_note_on_time += action->note_start_time;
          auto ev = DelayedMidiEvent(midi_note_on_time, event_on,
                                     action->soundgen_num);
          _action_items.push_back(ev);
        } else {
          sg->NoteOn(event_on);
        }
        if (action->duration) {
          int midi_off_tick = midi_note_on_time + action->duration;
          midi_event event_off =
              new_midi_event(MIDI_OFF, midinum, action->velocity);
          auto ev =
              DelayedMidiEvent(midi_off_tick, event_off, action->soundgen_num);
          _action_items.push_back(ev);
        }
      }
    }
  } else if (action->type == AudioAction::MIDI_NOTE_OFF ||
             action->type == AudioAction::MIDI_NOTE_OFF_DELAYED) {
    if (IsValidSoundgenNum(action->soundgen_num)) {
      auto &sg = sound_generators_[action->soundgen_num];

      for (auto midinum : action->notes) {
        midi_event event_off = new_midi_event(MIDI_OFF, midinum, 0);

        if (action->type == AudioAction::MIDI_NOTE_OFF_DELAYED) {
          int midi_note_off_time =
              timing_info.midi_tick + action->note_start_time;
          auto ev = DelayedMidiEvent(midi_note_off_time, event_off,
                                     action->soundgen_num);
          _action_items.push_back(ev);
        } else {
          if (event_off.data1 == 0) {
            sg->AllNotesOff();
          } else {
            sg->NoteOff(event_off);
          }
        }
      }
    }
  } else if (action->type == AudioAction::SOLO) {
    if (IsValidSoundgenNum(action->soundgen_num)) {
      soloed_sound_generator_idz.push_back(action->soundgen_num);
    }
  } else if (action->type == AudioAction::UNSOLO) {
    soloed_sound_generator_idz.clear();
  } else if (action->type == AudioAction::STOP) {
    if (IsValidSoundgenNum(action->soundgen_num)) {
      auto &sg = sound_generators_[action->soundgen_num];
      // sg->AllNotesOff();
      sg->Stop();
    }
  } else if (action->type == AudioAction::MIXER_UPDATE) {
    double param_val = std::stod(action->param_val);
    if (action->mixer_fx_id != -1) {
      fx_[action->mixer_fx_id]->SetParam(action->param_name, param_val);
    } else if (action->is_xfader) {
      xfader_.Set(action->param_name, param_val);
    }
  } else if (action->type == AudioAction::MIXER_FX_UPDATE) {
    for (const auto &soundgen_num : action->group_of_soundgens) {
      if (IsValidSoundgenNum(soundgen_num)) {
        auto &sg = sound_generators_[soundgen_num];
        if (sg) {
          sg->SetFxSend(action->mixer_fx_id, action->fx_intensity);
        }
      }
    }
  } else if (action->type == AudioAction::MIXER_XFADE_ACTION) {
    if (action->xfade_direction == 0) {
      xfader_.Set("xpos", -1);
    }
    if (action->xfade_direction == 1) {
      xfader_.Set("xpos", 1);
    }
  } else if (action->type == AudioAction::MIXER_XFADE_ASSIGN) {
    for (const auto &soundgen_num : action->group_of_soundgens) {
      if (IsValidSoundgenNum(soundgen_num)) {
        std::cout << "Adding soundgen num:" << soundgen_num << std::endl;
        xfader_.Assign(action->xfade_channel, soundgen_num);
      }
    }
  } else if (action->type == AudioAction::MIXER_XFADE_CLEAR) {
    xfader_.Clear();
  } else if (action->type == AudioAction::UPDATE) {
    if (IsValidSoundgenNum(action->mixer_soundgen_idx)) {
      double param_val = std::stod(action->param_val);

      auto &sg = sound_generators_[action->mixer_soundgen_idx];
      if (!sg) {
        std::cerr << "WHOE NELLY! Naw SG! bailing out!\n";
        return;
      }
      if (action->param_name == "volume" || action->param_name == "vol")
        sg->SetVolume(param_val);
      else if (action->param_name == "pan")
        sg->SetPan(param_val);
      else {
        // first check if we're setting an FX param
        if (action->fx_id != -1) {
          int fx_num = action->fx_id;
          if (IsValidFx(action->mixer_soundgen_idx, fx_num)) {
            // Shared lock to safely access effects array (reading to call
            // SetParam)
            std::shared_lock<std::shared_mutex> lock(sg->effects_mutex_);
            auto f = sg->effects_[fx_num];
            if (action->param_name == "active")
              f->enabled_ = param_val;
            else
              f->SetParam(action->param_name, param_val);
          } else {
            // if is Granulator - which re-uses the fx_id slot.
            if (sg->type == LOOPER_TYPE) {
              sg->SetSubParam(fx_num, action->param_name, param_val);
            }
          }
        } else  // must be a SoundGenerator param
        {
          sg->SetParam(action->param_name, param_val);
        }
      }
    }
  } else if (action->type == AudioAction ::INFO) {
    if (IsValidSoundgenNum(action->mixer_soundgen_idx)) {
      auto &sg = sound_generators_[action->mixer_soundgen_idx];
      repl_queue.push(sg->Info());
    }
  } else if (action->type == AudioAction ::SAVE_PRESET ||
             action->type == AudioAction ::RAND_PRESET ||
             action->type == AudioAction ::LIST_PRESETS) {
    interpreter_sound_cmds::ParseSynthCmd(action->args);
  } else if (action->type == AudioAction::LOAD_PRESET) {
    interpreter_sound_cmds::SynthLoadPreset(
        action->args[0], action->preset_name, action->preset);
  } else if (action->type == AudioAction::RAND) {
    sound_generators_[action->mixer_soundgen_idx]->Randomize();
  } else if (action->type == AudioAction::PREVIEW) {
    PreviewAudio(std::move(action));
  }
}

void Mixer::ResetMidiRecording() {
  recording_buffer_.fill({});
}

void Mixer::AssignSoundGeneratorToMidiController(int soundgen_id) {
  if (IsValidSoundgenNum(soundgen_id)) {
    std::cout << "MIDI Assign - " << soundgen_id << std::endl;
    midi_target = soundgen_id;
    // midi_targets.push_back(soundgen_id);
  }
}

void Mixer::RecordMidiToggle() {
  if (midi_recording && IsValidSoundgenNum(midi_target)) {
    sound_generators_[midi_target]->AllNotesOff();
  }
  midi_recording = 1 - midi_recording;
}

void Mixer::PrintMidiToggle() {
  midi_print = 1 - midi_print;
}

void Mixer::HandleMidiControlMessage(int data1, int data2) {
  if (midi_mapped_controls_.count(data1)) {
    auto param = midi_mapped_controls_[data1];
    const auto &[lo, hi] = GetBoundaries(param);
    auto val = scaleybum(0, 127, lo, hi, data2);
    std::stringstream ss;
    ss << "set " << param << " " << val;
    eval_command_queue.try_push(ss.str());
  }
}

void Mixer::CheckForExternalMidiEvents() {
  if (!have_midi_controller) return;

  auto tic = timing_info.midi_tick % PPBAR;

  PmEvent msg[32];
  if (Pm_Poll(global_mixr->midi_stream)) {
    int cnt = Pm_Read(global_mixr->midi_stream, msg, 32);
    for (int i = 0; i < cnt; i++) {
      int status = Pm_MessageStatus(msg[i].message);
      int data1 = Pm_MessageData1(msg[i].message);
      int data2 = Pm_MessageData2(msg[i].message);

      midi_event ev = new_midi_event(status, data1, data2);
      ev.original_tick = timing_info.midi_tick;

      if (midi_recording) {
        recording_buffer_[tic].push_back(ev);
      }
      if (midi_print) {
        std::cout << (timing_info.sixteenth_note_tick % 16) << " - MIDI -- "
                  << status << " " << data1 << " " << data2 << std::endl;
      }

      // TODO - THESE SHOULD BE SHARED WITH MIDI MESSAGE PARSING
      if (status == MIDI_ON || status == MIDI_OFF) {
        sound_generators_[midi_target]->ParseMidiEvent(ev, timing_info);
      }

      if (status == MIDI_CONTROL) {
        HandleMidiControlMessage(data1, data2);
      }
    }
  }
  if (midi_recording) {
    for (auto e : recording_buffer_[tic]) {
      if (e.event_type == MIDI_CONTROL) {
        HandleMidiControlMessage(e.data1, e.data2);
      } else {
        sound_generators_[midi_target]->ParseMidiEvent(e, timing_info);
      }
    }
  }
}

void Mixer::CheckForAudioActionQueueMessages() {
  while (auto opt_action = audio_queue.try_pop()) {
    auto &action = opt_action.value();
    if (action->delayed_by > 0) {
      action->start_at = timing_info.midi_tick + action->delayed_by;
      action->delayed_by = 0;
      delayed_action_items_.push_back(std::move(action));
    } else {
      ProcessActionMessage(std::move(action));
    }
  }
}

void Mixer::AddFileToMonitor(std::string filepath) {
  file_monitors.push_back(
      file_monitor{.function_file_filepath = filepath,
                   .function_file_filepath_last_write_time = {}});
}

void Mixer::CheckForDelayedEvents() {
  auto it = _action_items.begin();
  while (it != _action_items.end()) {
    if (it->target_tick == timing_info.midi_tick) {
      // TODO - push to action queue not call function
      if (IsValidSoundgenNum(it->sg_idx)) {
        auto &sg = sound_generators_[it->sg_idx];
        sg->ParseMidiEvent(it->event, timing_info);
      } else if (it->event.event_type == MIDI_CONTROL) {
        HandleMidiControlMessage(it->event.data1, it->event.data2);
      }
      // `erase()` invalidates the iterator, use returned iterator
      it = _action_items.erase(it);
    } else {
      ++it;
    }
  }

  std::vector<std::unique_ptr<AudioActionItem>> delayed_buffer;
  auto dit = delayed_action_items_.begin();
  while (dit != delayed_action_items_.end()) {
    if ((*dit)->start_at == (timing_info.midi_tick % PPBAR)) {
      if ((*dit)->event.event_type == MIDI_CONTROL) {
        HandleMidiControlMessage((*dit)->event.data1, (*dit)->event.data2);
      } else if (IsValidSoundgenNum((*dit)->mixer_soundgen_idx)) {
        auto &sg = sound_generators_[(*dit)->mixer_soundgen_idx];
        sg->ParseMidiEvent((*dit)->event, timing_info);
        if ((*dit)->event.event_type == MIDI_ON && (*dit)->event.dur > 0) {
          auto note_off = new_midi_event(MIDI_OFF, (*dit)->event.data1, 0);
          auto action = std::make_unique<AudioActionItem>(
              AudioAction::RECORDED_MIDI_EVENT);
          action->event = note_off;
          action->mixer_soundgen_idx = (*dit)->mixer_soundgen_idx;
          action->start_at = timing_info.midi_tick + (*dit)->event.dur;
          delayed_buffer.push_back(std::move(action));
        }
      }
      dit = delayed_action_items_.erase(dit);
    } else if ((*dit)->start_at == timing_info.midi_tick) {
      if ((*dit)->type == RECORDED_MIDI_EVENT) {
        if (IsValidSoundgenNum((*dit)->mixer_soundgen_idx)) {
          auto &sg = sound_generators_[(*dit)->mixer_soundgen_idx];
          sg->ParseMidiEvent((*dit)->event, timing_info);
        }
      } else {
        ProcessActionMessage(std::move(*dit));
      }
      // `erase()` invalidates the iterator, use returned iterator
      dit = delayed_action_items_.erase(dit);
    } else {
      ++dit;
    }
  }
  for (auto &d : delayed_buffer) {
    delayed_action_items_.push_back(std::move(d));
  }
}

void Mixer::PrintRecordingBuffer() {
  PrintMultiMidi(recording_buffer_);
}
void Mixer::AddMidiMapping(int id, std::string param) {
  midi_mapped_controls_[id] = param;
}

void Mixer::ResetMidiMappings() {
  midi_mapped_controls_.clear();
}
void Mixer::PrintMidiMappings() {
  for (const auto &[id, param] : midi_mapped_controls_) {
    std::cout << "ID:" << id << " // Param:" << param << std::endl;
  }
}
