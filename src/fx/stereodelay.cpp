#include <fx/stereodelay.h>
#include <mixer.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <memory>
#include <sstream>

extern std::unique_ptr<Mixer> global_mixr;

StereoDelay::StereoDelay() {
  enabled_ = true;
}

void StereoDelay::Reset() {
  m_left_delay_.ResetDelay();
  m_right_delay_.ResetDelay();
}

void StereoDelay::SetDelayTimeMs(double delay_ms) {
  if (delay_ms >= 0 && delay_ms <= kMaxDelayLenSecs * 1000) {
    m_delay_time_ms_ = delay_ms;
    Update();
  }
}

void StereoDelay::SetFeedbackPercent(double feedback_percent) {
  if (feedback_percent >= -100 && feedback_percent <= 100) {
    m_feedback_percent_ = feedback_percent;
    Update();
  }
}

void StereoDelay::SetDelayRatio(double delay_ratio) {
  if (delay_ratio > -1 && delay_ratio < 1) {
    m_delay_ratio_ = delay_ratio;
    Update();
  }
}

void StereoDelay::SetWetMix(double wet_mix) {
  if (wet_mix >= 0 && wet_mix <= 1) {
    m_wet_mix_ = wet_mix;
    Update();
  }
}

void StereoDelay::Update() {
  if (m_mode_ == DelayMode::tap1 || m_mode_ == DelayMode::tap2) {
    if (m_delay_ratio_ < 0) {
      m_tap2_left_delay_time_ms_ = -m_delay_ratio_ * m_delay_time_ms_;
      m_tap2_right_delay_time_ms_ = (1.0 + m_delay_ratio_) * m_delay_time_ms_;
    } else if (m_delay_ratio_ > 0) {
      m_tap2_left_delay_time_ms_ = (1.0 - m_delay_ratio_) * m_delay_time_ms_;
      m_tap2_right_delay_time_ms_ = m_delay_ratio_ * m_delay_time_ms_;
    } else {
      m_tap2_left_delay_time_ms_ = 0.0;
      m_tap2_right_delay_time_ms_ = 0.0;
    }
    m_left_delay_.SetDelayMs(m_delay_time_ms_);
    m_right_delay_.SetDelayMs(m_delay_time_ms_);

    return;
  }

  // else
  m_tap2_left_delay_time_ms_ = 0.0;
  m_tap2_right_delay_time_ms_ = 0.0;

  if (m_delay_ratio_ < 0) {
    m_left_delay_.SetDelayMs(-m_delay_ratio_ * m_delay_time_ms_);
    m_right_delay_.SetDelayMs(m_delay_time_ms_);
  } else if (m_delay_ratio_ > 0) {
    m_left_delay_.SetDelayMs(m_delay_time_ms_);
    m_right_delay_.SetDelayMs(m_delay_ratio_ * m_delay_time_ms_);
  } else {
    m_left_delay_.SetDelayMs(m_delay_time_ms_);
    m_right_delay_.SetDelayMs(m_delay_time_ms_);
  }
}

bool StereoDelay::ProcessAudio(double *input_left, double *input_right,
                               double *output_left, double *output_right) {
  double left_delay_out = m_left_delay_.ReadDelay();
  double right_delay_out = m_right_delay_.ReadDelay();

  double left_delay_in =
      *input_left + left_delay_out * (m_feedback_percent_ / 100.0);
  double right_delay_in =
      *input_right + right_delay_out * (m_feedback_percent_ / 100.0);

  double left_tap2_out = 0.0;
  double right_tap2_out = 0.0;

  switch (m_mode_) {
    case DelayMode::tap1: {
      left_tap2_out = m_left_delay_.ReadDelayAt(m_tap2_left_delay_time_ms_);
      right_tap2_out = m_right_delay_.ReadDelayAt(m_tap2_right_delay_time_ms_);
      break;
    }
    case DelayMode::tap2: {
      left_tap2_out = m_left_delay_.ReadDelayAt(m_tap2_left_delay_time_ms_);
      right_tap2_out = m_right_delay_.ReadDelayAt(m_tap2_right_delay_time_ms_);
      left_delay_in =
          *input_left + (0.5 * left_delay_out + 0.5 * left_tap2_out) *
                            (m_feedback_percent_ / 100.0);
      right_delay_in =
          *input_right + (0.5 * right_delay_out + 0.5 * right_tap2_out) *
                             (m_feedback_percent_ / 100.0);
      break;
    }
    case DelayMode::pingpong: {
      left_delay_in =
          *input_right + right_delay_out * (m_feedback_percent_ / 100.0);
      right_delay_in =
          *input_left + left_delay_out * (m_feedback_percent_ / 100.0);
      break;
    }
    default: {
    }
  }

  double left_out = 0.0;
  double right_out = 0.0;

  m_left_delay_.ProcessAudio(&left_delay_in, &left_out);
  m_right_delay_.ProcessAudio(&right_delay_in, &right_out);

  *output_left = *input_left * (1.0 - m_wet_mix_) +
                 m_wet_mix_ * (left_out + left_tap2_out);
  *output_right = *input_right * (1.0 - m_wet_mix_) +
                  m_wet_mix_ * (right_out + right_tap2_out);

  return true;
}

std::string StereoDelay::Status() {
  std::string mode{};
  switch (m_mode_) {
    case DelayMode::norm:
      mode = "norm";
      break;
    case DelayMode::tap1:
      mode = "tap1";
      break;
    case DelayMode::tap2:
      mode = "tap2";
      break;
    case DelayMode::pingpong:
      mode = "pingpong";
  }
  std::string sync_len{};
  switch (sync_len_) {
    case DelaySyncLen::NO_DELAY:
      sync_len = "none";
      break;
    case DelaySyncLen::QUARTER:
      sync_len = "1/4";
      break;
    case DelaySyncLen::EIGHTH:
      sync_len = "1/8";
      break;
    case DelaySyncLen::SIXTEENTH:
      sync_len = "1/16";
  }
  std::stringstream ss;
  ss << "Delay! ms:" << m_delay_time_ms_;
  ss << " fb:" << m_feedback_percent_;
  ss << " rat:" << m_delay_ratio_;
  ss << " mx:" << m_wet_mix_;
  ss << " mode:" << mode;
  ss << " sync:" << sync_;
  ss << " sync_len:" << sync_len;

  return ss.str();
}

StereoVal StereoDelay::Process(StereoVal input) {
  StereoVal output = {};
  ProcessAudio(&input.left, &input.right, &output.left, &output.right);
  return output;
}

void StereoDelay::SetParam(std::string name, double val) {
  if (name == "ms")
    SetDelayTimeMs(val);
  else if (name == "fb")
    SetFeedbackPercent(val);
  else if (name == "rat")
    SetDelayRatio(val);
  else if (name == "mx")
    SetWetMix(val);
  else if (name == "mode")
    SetMode(val);
  else if (name == "sync")
    SetSync(val);
  else if (name == "sync_len")
    SetSyncLen(val);
}
void StereoDelay::SetMode(unsigned mode) {
  switch (mode) {
    case (0):
      m_mode_ = DelayMode::norm;
      break;
    case (1):
      m_mode_ = DelayMode::tap1;
      break;
    case (2):
      m_mode_ = DelayMode::tap2;
      break;
    case (3):
      m_mode_ = DelayMode::pingpong;
      break;
  }
}

void StereoDelay::SyncTempo() {
  double delay_time_quarter_note_ms = 60 / global_mixr->bpm * 1000;
  if (sync_len_ == DelaySyncLen::QUARTER)
    m_delay_time_ms_ = delay_time_quarter_note_ms;
  else if (sync_len_ == DelaySyncLen::EIGHTH)
    m_delay_time_ms_ = delay_time_quarter_note_ms * 0.5;
  else if (sync_len_ == DelaySyncLen::SIXTEENTH)
    m_delay_time_ms_ = delay_time_quarter_note_ms * 0.25;

  Update();
}

void StereoDelay::SetSync(bool b) {
  sync_ = b;
  if (b) SyncTempo();
}

void StereoDelay::SetSyncLen(unsigned int len) {
  switch (len) {
    case 0:
      sync_len_ = DelaySyncLen::NO_DELAY;
      break;
    case 1:
      sync_len_ = DelaySyncLen::QUARTER;
      break;
    case 2:
      sync_len_ = DelaySyncLen::EIGHTH;
      break;
    case 3:
      sync_len_ = DelaySyncLen::SIXTEENTH;
      break;
  }
  if (sync_) SyncTempo();
}
