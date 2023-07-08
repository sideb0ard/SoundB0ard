#include <defjams.h>
#include <granulator.h>
#include <libgen.h>
#include <mixer.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>

#include <iostream>

namespace SBAudio {

const std::array<std::string, 3> kLoopModeNames = {"LOOP", "STATIC", "SMUDGE"};

void Granulator::Reset() {
  audio_buffer_read_idx_ = 0;
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
  filename_ = filename;
  ImportFile(filename_);
  SetLoopMode(loop_mode);
  Start();
}

void Granulator::EventNotify(broadcast_event event, mixer_timing_info tinfo) {
  (void)event;
  double decimal_percent_of_loop = 0;
  if (tinfo.is_midi_tick) {
    if (started_)
      cur_midi_idx_ = fmodf(cur_midi_idx_ + incr_speed_, PPBAR * loop_len_);
    if (loop_mode_ == LoopMode::loop_mode) {
      decimal_percent_of_loop = cur_midi_idx_ / (PPBAR * loop_len_);
      double new_read_idx = decimal_percent_of_loop * audio_buffer_.size();
      if (reverse_mode_)
        new_read_idx = (audio_buffer_.size() - 1) - new_read_idx;

      // std::cout << "READIDX: " << new_read_idx << std::endl;
      new_read_idx = fmodf(
          (fmodf(new_read_idx, size_of_sixteenth_ * plooplen_ * loop_len_) +
           poffset_ * size_of_sixteenth_),
          audio_buffer_.size());
      // std::cout << "PLOOPED READIDX: " << new_read_idx << std::endl;

      // this ensures new_read_idx is even
      if (num_channels_ == 2) new_read_idx -= ((int)new_read_idx & 1);

      audio_buffer_read_idx_ = new_read_idx;

      int rel_pos_within_a_sixteenth =
          fmod(audio_buffer_read_idx_, size_of_sixteenth_);

      if (scramble_mode_ || pinc_ != 1) {
        audio_buffer_read_idx_ = (current_sixteenth_ * size_of_sixteenth_) +
                                 rel_pos_within_a_sixteenth;
      } else if (stutter_mode_) {
        audio_buffer_read_idx_ =
            (stutter_idx_ * size_of_sixteenth_) + rel_pos_within_a_sixteenth;
      }
    }
  }

  if (!started_ && tinfo.is_start_of_loop) {
    eg_.StartEg();
    started_ = true;
    LaunchGrain(active_grain_, tinfo);
  }
  if (!started_) return;

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
    loop_counter_++;

    if (scramble_pending_) {
      scramble_mode_ = true;
      scramble_pending_ = false;
    } else
      scramble_mode_ = false;

    if (stutter_pending_) {
      stutter_mode_ = true;
      stutter_pending_ = false;
    } else
      stutter_mode_ = false;
    if (reverse_pending_) {
      reverse_mode_ = true;
      reverse_pending_ = false;
    } else
      reverse_mode_ = false;
  }

  // used to track which 16th we're on if loop != 1 bar
  float loop_num = fmod(loop_counter_, loop_len_);
  if (loop_num < 0) loop_num = 0;

  if (tinfo.is_sixteenth) {
    if (pinc_ != 1) {
      current_sixteenth_ = (current_sixteenth_ + pinc_) % 16;
    } else {
      current_sixteenth_ = tinfo.sixteenth_note_tick % 16;
    }
    if (scramble_mode_) {
      if (current_sixteenth_ % 2 != 0) {
        int randy = rand() % 100;
        if (randy < 25)  // repeat the third 16th
          current_sixteenth_ = 3;
        else if (randy > 25 && randy < 50)  // repeat the 4th sixteenth
          current_sixteenth_ = 4;
        else if (randy > 50 && randy < 75)  // repeat the 7th sixteenth
          current_sixteenth_ = 7;
      }
    }
    if (stutter_mode_) {
      if (rand() % 100 > 75) stutter_idx_++;
      if (stutter_idx_ == 16) stutter_idx_ = 0;
    }
  }
}

// for debugging only
int launch_count = 0;

void Granulator::LaunchGrain(SoundGrain *grain, mixer_timing_info tinfo) {
  int duration_frames = grain_duration_frames_;
  if (quasi_grain_fudge_ != 0)
    duration_frames += rand() % (int)(quasi_grain_fudge_ * 44.1);

  int grain_idx = audio_buffer_read_idx_;
  if (granular_spray_frames_ > 0) grain_idx += rand() % granular_spray_frames_;

  SoundGrainParams params = {
      .grain_type = SoundGrainType::Sample,
      .dur_frames = duration_frames,
      .starting_idx = grain_idx,
      .reverse_mode = reverse_mode_,
      .pitch = grain_pitch_,
      .num_channels = num_channels_,
      .degrade_by = degrade_by_,
      .audio_buffer = &audio_buffer_,
  };

  grain->Initialize(params);

  xfade_time_in_frames_ = duration_frames / 100. * 20;
  grain_spacing_frames_ = duration_frames - xfade_time_in_frames_;

  next_grain_launch_sample_time_ = tinfo.cur_sample + grain_spacing_frames_;

  start_xfade_at_frame_time_ = next_grain_launch_sample_time_;
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

  if (stop_pending_ && eg_.m_state == OFFF) active = false;

  if (loop_mode_ == LoopMode::loop_mode) {
    float loop_num = fmod(loop_counter_, loop_len_);
    if (loop_num < 0) loop_num = 0;
  }

  if (tinfo.cur_sample == start_xfade_at_frame_time_) {
    xfader_active_ = true;
  }
  if (tinfo.cur_sample == stop_xfade_at_frame_time_) {
    SwitchXFadeGrains();
    xfader_active_ = false;
    xfader_.Reset(xfade_time_in_frames_);
  }

  if (tinfo.cur_sample == next_grain_launch_sample_time_) {  // new grain time
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
  ss << filename_ << " vol:" << volume << " pan:" << pan
     << " pitch:" << grain_pitch_
     << " idx:" << (int)(100. / audio_buffer_.size() * audio_buffer_read_idx_)
     << " mode:" << kLoopModeNames[loop_mode_] << "(" << loop_mode_ << ")"
     << " len:" << loop_len_ << ANSI_COLOR_RESET;
  return ss.str();
}

std::string Granulator::Info() {
  char *INSTRUMENT_COLOR = (char *)ANSI_COLOR_RESET;
  if (active) INSTRUMENT_COLOR = (char *)ANSI_COLOR_RED;

  std::stringstream ss;
  ss << ANSI_COLOR_WHITE << filename_ << INSTRUMENT_COLOR << " vol:" << volume
     << " pan:" << pan << " pitch:" << grain_pitch_ << " speed:" << incr_speed_
     << " mode:" << kLoopModeNames[loop_mode_]
     << "\ngrain_dur_ms:" << grain_duration_frames_
     << " grains_per_sec:" << grains_per_sec_
     << " quasi_grain_fudge:" << quasi_grain_fudge_ << " poffset:" << poffset_
     << " plooplen:" << plooplen_ << " pinc:" << pinc_
     << "\ngrain_spray_ms:" << granular_spray_frames_ / 44.1
     << " attack:" << eg_.m_attack_time_msec
     << " decay:" << eg_.m_decay_time_msec
     << " release:" << eg_.m_release_time_msec
     << " grain_ramp_time:" << grain_ramp_time_;

  return ss.str();
}

void Granulator::Start() { Reset(); }

void Granulator::Stop() {
  eg_.Release();
  stop_pending_ = true;
}

Granulator::~Granulator() {
  // TODO delete file
}

void Granulator::ImportFile(std::string filename) {
  AudioBufferDetails deetz = ImportFileContents(audio_buffer_, filename);
  num_channels_ = deetz.num_channels;
  SetLoopLen(1);
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

void Granulator::SetAudioBufferReadIdx(size_t pos) {
  if (pos < 0 || pos >= audio_buffer_.size()) {
    return;
  }
  audio_buffer_read_idx_ = pos;
}

void Granulator::SetGranularSpray(int spray_ms) {
  granular_spray_frames_ = spray_ms / 1000 * SAMPLE_RATE;
}

void Granulator::SetQuasiGrainFudge(int fudgefactor) {
  quasi_grain_fudge_ = fudgefactor;
}

void Granulator::SetGrainPitch(double pitch) { grain_pitch_ = pitch; }
void Granulator::SetIncrSpeed(double speed) { incr_speed_ = speed; }
void Granulator::SetReverseMode(bool b) { reverse_mode_ = b; }

void Granulator::SetLoopMode(unsigned int m) {
  volume = 0.2;
  switch (m) {
    case (0):
      loop_mode_ = LoopMode::loop_mode;
      quasi_grain_fudge_ = 0;
      granular_spray_frames_ = 0;
      volume = 1;
      break;
    case (1):
      loop_mode_ = LoopMode::static_mode;
      quasi_grain_fudge_ = 0;
      granular_spray_frames_ = 0;
      break;
    case (2):
    default:
      loop_mode_ = LoopMode::smudge_mode;
      quasi_grain_fudge_ = 220;
      granular_spray_frames_ = 441;  // 10ms * (44100/1000)
  }
}
void Granulator::SetScramblePending() { scramble_pending_ = true; }

void Granulator::SetStopPending(int loops) {
  stop_count_pending_ = true;
  stop_len_ = loops;
  stop_countr_ = 0;
}

void Granulator::SetStutterPending() { stutter_pending_ = true; }
void Granulator::SetReversePending() { reverse_pending_ = true; }

void Granulator::SetLoopLen(double bars) {
  if (bars != 0) {
    loop_len_ = bars;
    size_of_sixteenth_ = audio_buffer_.size() / 16 / bars;
  }
}

void Granulator::SetDegradeBy(int degradation) {
  if (degradation >= 0 && degradation <= 100) degrade_by_ = degradation;
}

void Granulator::NoteOn(midi_event ev) {
  (void)ev;
  eg_.StartEg();
}

void Granulator::NoteOff(midi_event ev) {
  (void)ev;
  eg_.NoteOff();
}

void Granulator::SetPOffset(int poffset) {
  if (poffset >= 0 && poffset <= 15) {
    poffset_ = poffset;
  }
}

void Granulator::SetPlooplen(int plooplen) {
  if (plooplen > 0 && plooplen <= 16) {
    plooplen_ = plooplen;
  }
}
void Granulator::SetPinc(int pinc) { pinc_ = pinc; }

void Granulator::SetParam(std::string name, double val) {
  if (name == "active") {
    Start();
  } else if (name == "on") {
    eg_.StartEg();
  } else if (name == "off") {
    eg_.NoteOff();
  } else if (name == "pitch")
    SetGrainPitch(val);
  else if (name == "speed")
    SetIncrSpeed(val);
  else if (name == "mode") {
    SetLoopMode(val);
  } else if (name == "idx") {
    if (val <= 100) {
      double pos = audio_buffer_.size() / 100. * val;
      SetAudioBufferReadIdx(pos);
    }
  } else if (name == "len")
    SetLoopLen(val);
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
}

}  // namespace SBAudio
