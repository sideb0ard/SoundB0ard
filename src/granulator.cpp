#include "granulator.h"

#include <audioutils.h>
#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>

#include <iostream>

#include "defjams.h"
#include "mixer.h"

namespace SBAudio {

namespace {

const std::array<std::string, 3> kLoopModeNames = {"LOOP", "STATIC", "SMUDGE"};

void ClearPattern(std::array<int, 16> &pattern) {
  for (int i = 0; i < 16; ++i) {
    pattern[i] = 0;
  }
}
void StutterPattern(std::array<int, 16> &pattern) {
  ClearPattern(pattern);

  int idx = 0;
  for (int i = 0; i < 16; ++i) {
    if (rand() % 100 > 70) idx++;
    pattern[i] = idx;
  }
}

void ScramblePattern(std::array<int, 16> &pattern) {
  ClearPattern(pattern);

  for (int i = 0; i < 16; ++i) {
    pattern[i] = i;
    if (i % 4 > 0) {
      pattern[i] = rand() % 16;
    }
    if (rand() % 100 > 90) {
      pattern[i] = 0;
    }
  }
  pattern[15] = 0;
}

}  // namespace

void Granulator::Reset() {
  file_buffer_->audio_buffer_read_idx_ = 0;
  active_grain_ = grain_a_.get();
  incoming_grain_ = grain_b_.get();
  reverse_mode_ = false;
  SetGrainDensity(15);
  eg_.SetAttackTimeMsec(5);
  eg_.SetDecayTimeMsec(0);
  eg_.SetSustainLevel(1);
  eg_.SetReleaseTimeMsec(50);
  eg_.SetRampMode(false);
  eg_.Update();
  eg_.StartEg();

  degrade_by_ = 0;

  active = true;
  started_ = false;
  stop_pending_ = false;
}

Granulator::Granulator() { grain_type_ = SoundGrainType::Signal; }

Granulator::Granulator(std::string filename, unsigned int loop_mode) {
  type = LOOPER_TYPE;
  grain_type_ = SoundGrainType::Sample;
  grain_a_ = std::make_unique<SoundGrainSample>();
  grain_b_ = std::make_unique<SoundGrainSample>();

  file_buffer_ = std::make_unique<FileBuffer>(filename);
  SetLoopMode(loop_mode);
  Start();
}

Granulator::~Granulator() {
  // TODO delete file
}

void Granulator::AddBuffer(std::unique_ptr<FileBuffer> fb) {
  file_buffer_ = std::move(fb);
}

void Granulator::EventNotify(broadcast_event event, mixer_timing_info tinfo) {
  (void)event;

  if (!started_ && tinfo.is_start_of_loop) {
    LaunchGrain(active_grain_, tinfo);
    eg_.StartEg();
    started_ = true;
  }
  if (!started_) return;

  std::vector<double> *audio_buffer = file_buffer_->GetAudioBuffer();

  double decimal_percent_of_loop = 0;
  if (tinfo.is_midi_tick) {
    file_buffer_->cur_midi_idx_ =
        fmodf(file_buffer_->cur_midi_idx_ + file_buffer_->incr_speed_,
              PPBAR * file_buffer_->loop_len_);
    if (file_buffer_->loop_mode_ == LoopMode::loop_mode) {
      decimal_percent_of_loop =
          file_buffer_->cur_midi_idx_ / (PPBAR * file_buffer_->loop_len_);
      double new_read_idx = decimal_percent_of_loop * audio_buffer->size();
      if (reverse_mode_)
        new_read_idx = (audio_buffer->size() - 1) - new_read_idx;

      new_read_idx =
          fmodf((fmodf(new_read_idx, file_buffer_->size_of_sixteenth_ *
                                         file_buffer_->plooplen_ *
                                         file_buffer_->loop_len_) +
                 file_buffer_->poffset_ * file_buffer_->size_of_sixteenth_),
                audio_buffer->size());

      // this ensures new_read_idx is even
      if (file_buffer_->num_channels_ == 2)
        new_read_idx -= ((int)new_read_idx & 1);

      if (new_read_idx < 0 || new_read_idx > audio_buffer->size() - 1) {
        new_read_idx = 0;
        std::cout << "OH YA:" << new_read_idx
                  << " bufflen:" << (audio_buffer->size() - 1) << std::endl;
      }

      file_buffer_->audio_buffer_read_idx_ = new_read_idx;

      int rel_pos_within_a_sixteenth =
          fmod(file_buffer_->audio_buffer_read_idx_,
               file_buffer_->size_of_sixteenth_);

      if (file_buffer_->stutter_mode_ || file_buffer_->scramble_mode_) {
        file_buffer_->audio_buffer_read_idx_ = fmodf(
            (file_buffer_->scrambled_pattern_[file_buffer_->cur_sixteenth_] *
             file_buffer_->size_of_sixteenth_) +
                rel_pos_within_a_sixteenth,
            audio_buffer->size());
      }
    }
  }

  if (tinfo.is_end_of_loop) {
    if (stop_count_pending_) {
      stop_countr_++;
      if (stop_countr_ >= stop_len_) {
        Stop();
        stop_count_pending_ = false;
      }
    }
  }
  if (tinfo.is_start_of_loop) {
    if (file_buffer_->scramble_pending_) {
      file_buffer_->scramble_mode_ = true;
      file_buffer_->scramble_pending_ = false;
    } else
      file_buffer_->scramble_mode_ = false;

    if (file_buffer_->stutter_pending_) {
      file_buffer_->stutter_mode_ = true;
      file_buffer_->stutter_pending_ = false;
    } else
      file_buffer_->stutter_mode_ = false;

    if (reverse_pending_) {
      reverse_mode_ = true;
      reverse_pending_ = false;
    } else
      reverse_mode_ = false;
  }

  if (tinfo.is_sixteenth) {
    file_buffer_->cur_sixteenth_ = tinfo.sixteenth_note_tick % 16;
  }
}

// for debugging only
int launch_count = 0;
int samp_diff = 0;
int midi_diff = 0;

void Granulator::LaunchGrain(SoundGrain *grain, mixer_timing_info tinfo) {
  int duration_frames = grain_duration_frames_;
  if (quasi_grain_fudge_ != 0)
    duration_frames += rand() % (int)(quasi_grain_fudge_ * 44.1);

  std::unique_ptr<FileBuffer> &file_buffer = file_buffer_;
  std::vector<double> *audio_buffer = file_buffer->GetAudioBuffer();

  int grain_idx = file_buffer->audio_buffer_read_idx_;
  if (granular_spray_frames_ > 0)
    grain_idx += (rand() % granular_spray_frames_) % audio_buffer->size();

  SoundGrainParams params = {
      .grain_type = SoundGrainType::Sample,
      .dur_frames = duration_frames,
      .starting_idx = grain_idx,
      .reverse_mode = reverse_mode_,
      .num_channels = file_buffer->num_channels_,
      .degrade_by = degrade_by_,
      .audio_buffer = audio_buffer,
  };

  grain->Initialize(params);

  xfade_time_in_frames_ = duration_frames / 100. * 20;
  grain_spacing_frames_ = duration_frames - xfade_time_in_frames_;

  next_grain_launch_sample_time_ = tinfo.cur_sample + grain_spacing_frames_;
  //  if (launch_count < 10) {
  //    std::cout << launch_count << " Mixer Samp#:" << tinfo.cur_sample
  //              << " dur:" << duration_frames << " GRain IDX:" << grain_idx
  //              << " NEXT LAYNC :" << next_grain_launch_sample_time_
  //              << " MIDI:" << tinfo.midi_tick << " %16 " << tinfo.midi_tick %
  //              16
  //              << std::endl;
  //    launch_count++;
  //  }

  start_xfade_at_frame_time_ = next_grain_launch_sample_time_;
  if (started_)
    stop_xfade_at_frame_time_ = tinfo.cur_sample + xfade_time_in_frames_;
}

void Granulator::SwitchXFadeGrains() {
  if (active_grain_ == grain_a_.get()) {
    active_grain_ = grain_b_.get();
    incoming_grain_ = grain_a_.get();
  } else {
    active_grain_ = grain_a_.get();
    incoming_grain_ = grain_b_.get();
  }
}

StereoVal Granulator::GenNext(mixer_timing_info tinfo) {
  StereoVal val = {0., 0.};
  if (!started_ || !active) {
    return val;
  }
  file_buffer_->CheckPendingRepitch();

  if (stop_pending_ && eg_.m_state == OFFF) active = false;

  if (tinfo.cur_sample == start_xfade_at_frame_time_) {
    xfader_active_ = true;
  }
  if (tinfo.cur_sample == stop_xfade_at_frame_time_) {
    SwitchXFadeGrains();
    xfader_active_ = false;
    xfader_.Reset(xfade_time_in_frames_);
  }

  if (tinfo.cur_sample >= next_grain_launch_sample_time_) {  // new grain time
    LaunchGrain(incoming_grain_, tinfo);
  }

  StereoVal active_val = active_grain_->Generate();
  StereoVal incoming_val = incoming_grain_->Generate();

  if (xfader_active_) {
    double incoming_vol = xfader_.Generate();
    double active_vol = 1.0 - incoming_vol;

    val.left = active_val.left * active_vol + incoming_val.left * incoming_vol;
    val.right =
        active_val.right * active_vol + incoming_val.right * incoming_vol;
  } else {
    val.left = active_val.left;
    val.right = active_val.right;
  }

  eg_.Update();
  double eg_amp = eg_.DoEnvelope(NULL);

  pan = fmin(pan, 1.0);
  pan = fmax(pan, -1.0);
  double pan_left = 0.707;
  double pan_right = 0.707;
  calculate_pan_values(pan, &pan_left, &pan_right);

  val.left = val.left * volume * eg_amp * pan_left;
  val.right = val.right * volume * eg_amp * pan_right;

  val = Effector(val);

  return val;
}

std::string Granulator::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << ANSI_COLOR_RED;
  ss << "SBPlayer // vol:" << volume << " pan:" << pan
     << " pitch:" << file_buffer_->pitch_ratio_ << "\n";
  int idx = 0;
  ss << "      " << idx++ << " " << file_buffer_->filename_ << " idx:"
     << (int)(100. / file_buffer_->GetAudioBuffer()->size() *
              file_buffer_->audio_buffer_read_idx_)
     << " mode:" << kLoopModeNames[file_buffer_->loop_mode_] << "("
     << file_buffer_->loop_mode_ << ")"
     << " len:" << file_buffer_->loop_len_ << "\n";
  ss << ANSI_COLOR_RESET;
  return ss.str();
}

std::string Granulator::Info() {
  char *INSTRUMENT_COLOR = (char *)ANSI_COLOR_RESET;
  if (active) INSTRUMENT_COLOR = (char *)ANSI_COLOR_RED;

  std::stringstream ss;
  ss << INSTRUMENT_COLOR << "\nSBPlayer // vol:" << volume << " pan:" << pan
     << " pitch:" << file_buffer_->pitch_ratio_
     << "\ngrain_dur_ms:" << grain_duration_frames_
     << " grains_per_sec:" << grains_per_sec_
     << " quasi_grain_fudge:" << quasi_grain_fudge_
     << " grain_spray_ms:" << granular_spray_frames_ / 44.1
     << "\nattack:" << eg_.m_attack_time_msec
     << " decay:" << eg_.m_decay_time_msec
     << " release:" << eg_.m_release_time_msec
     << " grain_ramp_time:" << grain_ramp_time_ << "\n";

  int idx = 0;
  ss << ANSI_COLOR_WHITE << idx++ << " " << file_buffer_->filename_
     << " speed:" << file_buffer_->incr_speed_
     << " mode:" << kLoopModeNames[file_buffer_->loop_mode_]
     << " poffset:" << file_buffer_->poffset_
     << " plooplen:" << file_buffer_->plooplen_
     << " pinc:" << file_buffer_->pinc_;

  return ss.str();
}

void Granulator::Start() { Reset(); }

void Granulator::Stop() {
  eg_.Release();
  stop_pending_ = true;
}

void Granulator::SetGrainDuration(int dur) {
  grains_per_sec_ = 1000. / dur;
  grain_duration_frames_ = (double)SAMPLE_RATE / grains_per_sec_;
}

void Granulator::SetGrainDensity(int gps) {
  grains_per_sec_ = gps;
  grain_duration_frames_ = (double)SAMPLE_RATE / grains_per_sec_;
  grain_ramp_time_ = grain_duration_frames_ / 100. * 10;
  xfade_time_in_frames_ = grain_duration_frames_ / 100. * 20;
  xfader_.Reset(xfade_time_in_frames_);
  grain_spacing_frames_ = grain_duration_frames_ - xfade_time_in_frames_;
}

void Granulator::SetGranularSpray(int spray_ms) {
  granular_spray_frames_ = spray_ms / 1000 * SAMPLE_RATE;
}

void Granulator::SetQuasiGrainFudge(int fudgefactor) {
  quasi_grain_fudge_ = fudgefactor;
}

void Granulator::SetPitch(double pitch_ratio) {
  file_buffer_->SetPitch(pitch_ratio);
}

void Granulator::SetIncrSpeed(double speed) {
  file_buffer_->incr_speed_ = speed;
}

void Granulator::SetReverseMode(bool b) { reverse_mode_ = b; }

void Granulator::SetLoopMode(unsigned int m) {
  volume = 0.2;
  std::unique_ptr<FileBuffer> &buffer = file_buffer_;
  switch (m) {
    case (0):
      buffer->loop_mode_ = LoopMode::loop_mode;
      quasi_grain_fudge_ = 0;
      granular_spray_frames_ = 0;
      volume = 1;
      break;
    case (1):
      buffer->loop_mode_ = LoopMode::static_mode;
      quasi_grain_fudge_ = 0;
      granular_spray_frames_ = 0;
      break;
    case (2):
    default:
      buffer->loop_mode_ = LoopMode::smudge_mode;
      quasi_grain_fudge_ = 220;
      granular_spray_frames_ = 441;  // 10ms * (44100/1000)
  }
}
void Granulator::SetScramblePending() {
  std::unique_ptr<FileBuffer> &buffer = file_buffer_;
  buffer->scramble_pending_ = true;
  ScramblePattern(buffer->scrambled_pattern_);
}

void Granulator::SetStopPending(int loops) {
  stop_count_pending_ = true;
  stop_len_ = loops;
  stop_countr_ = 0;
}

void Granulator::SetStutterPending() {
  file_buffer_->stutter_pending_ = true;
  StutterPattern(file_buffer_->scrambled_pattern_);
}
void Granulator::SetReversePending() { reverse_pending_ = true; }

void Granulator::SetLoopLen(double bars) {
  if (bars != 0) {
    file_buffer_->SetLoopLen(bars);
  }
}

void Granulator::SetDegradeBy(int degradation) {
  if (degradation >= 0 && degradation <= 100) degrade_by_ = degradation;
}

void Granulator::NoteOn(midi_event ev) {
  started_ = true;
  int sixteenth_pos = ev.data1 % 16;
  file_buffer_->cur_midi_idx_ = sixteenth_pos * 240;
  int decimal_percent_of_loop =
      file_buffer_->cur_midi_idx_ / (PPBAR * file_buffer_->loop_len_);

  auto audio_buffer = file_buffer_->GetAudioBuffer();
  double new_read_idx = decimal_percent_of_loop * audio_buffer->size();
  grain_a_->SetReadIdx(new_read_idx);
  grain_b_->SetReadIdx(new_read_idx);
  next_grain_launch_sample_time_ = 0;
  eg_.StartEg();
}

void Granulator::AllNotesOff() {
  started_ = true;
  eg_.NoteOff();
}

void Granulator::NoteOff(midi_event ev) {
  (void)ev;
  started_ = true;
  eg_.NoteOff();
}

void Granulator::SetPidx(int val) {
  file_buffer_->poffset_ = abs(val - file_buffer_->cur_sixteenth_) % 16;
}

void Granulator::SetPOffset(int p) {
  if (p >= 0 && p <= 15) {
    file_buffer_->poffset_ = p;
  }
}

void Granulator::SetPlooplen(int plooplen) {
  if (plooplen > 0 && plooplen <= 16) {
    file_buffer_->plooplen_ = plooplen;
  }
}
void Granulator::SetPinc(int pinc) { file_buffer_->pinc_ = pinc; }

void Granulator::SetParam(std::string name, double val) {
  std::unique_ptr<FileBuffer> &buffer = file_buffer_;
  if (name == "active") {
    Start();
  } else if (name == "on") {
    eg_.StartEg();
  } else if (name == "off") {
    eg_.NoteOff();
  } else if (name == "speed")
    SetIncrSpeed(val);
  else if (name == "mode") {
    SetLoopMode(val);
  } else if (name == "idx") {
    if (val <= 100) {
      double pos = val / 100. * buffer->GetAudioBuffer()->size();
      buffer->SetAudioBufferReadIdx(pos);
    }
  } else if (name == "len")
    SetLoopLen(val);
  else if (name == "pidx")
    SetPidx(val);
  else if (name == "poffset")
    SetPOffset(val);
  else if (name == "plooplen")
    SetPlooplen(val);
  else if (name == "pinc")
    SetPinc(val);
  else if (name == "scramble")
    SetScramblePending();
  else if (name == "stutter")
    SetStutterPending();
  else if (name == "stop_in")
    SetStopPending(val);
  else if (name == "reverse")
    SetReversePending();
  else if (name == "grain_dur_ms")
    SetGrainDuration(val);
  else if (name == "grains_per_sec")
    SetGrainDensity(val);
  else if (name == "quasi_grain_fudge")
    SetQuasiGrainFudge(val);
  else if (name == "grain_spray_ms")
    SetGranularSpray(val);
  else if (name == "attack")
    eg_.SetAttackTimeMsec(val);
  else if (name == "decay")
    eg_.SetDecayTimeMsec(val);
  else if (name == "release")
    eg_.SetReleaseTimeMsec(val);
  else if (name == "pitch") {
    SetPitch(val);
  }
}

void Granulator::SetSubParam(int id, std::string name, double val) {
  file_buffer_->SetParam(name, val);
}

}  // namespace SBAudio
