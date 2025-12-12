#include <fx/modular_delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <sstream>

namespace {
std::string GetAlgorithmName(ModDelayAlgorithm alg) {
  switch (alg) {
    case (ModDelayAlgorithm::kFlanger):
      return "Flanger";
    case (ModDelayAlgorithm::kChorus):
      return "Chorus";
    case (ModDelayAlgorithm::kVibrato):
    default:
      return "Vibrato";
  }
}
}  // namespace

ModDelay::ModDelay()
    : m_ddl_left_(100.0),  // 100ms buffer (chorus can reach ~60ms with offset)
      m_ddl_right_(100.0) {
  type_ = fx_type::MODDELAY;

  m_min_delay_msec_ = 0.0;
  m_max_delay_msec_ = 0.0;
  m_chorus_offset_ = 0;

  m_ddl_left_.m_use_external_feedback = false;
  m_ddl_left_.m_delay_ms = 0.;
  m_ddl_right_.m_use_external_feedback = false;
  m_ddl_right_.m_delay_ms = 0.;

  // m_lfo.polarity = 1;
  // m_lfo.mode = 0;

  // "gui"
  m_mod_depth_pct_ = 50;                      // percent
  m_mod_freq_ = 0.18;                         // range: 0.02 - 5
  m_feedback_percent_ = 0;                    //  range: -100 - 100
  m_mod_type_ = ModDelayAlgorithm::kFlanger;  // FLANGER, VIBRATO,
                                              // CHORUS
  m_lfo_type_ = 0;  // TRI or SINE // these don't match other OSC enums

  Update();
  m_lfo_.StartOscillator();

  enabled_ = true;
}

bool ModDelay::Update() {
  CookModType();
  UpdateLfo();
  UpdateDdl();
  return true;
}

void ModDelay::CookModType() {
  switch (m_mod_type_) {
    case ModDelayAlgorithm::kVibrato: {
      m_min_delay_msec_ = 0;
      m_max_delay_msec_ = 7;
      m_ddl_left_.m_wet_level_pct = 100.0;
      m_ddl_left_.m_feedback_pct = 0.0;
      m_ddl_right_.m_wet_level_pct = 100.0;
      m_ddl_right_.m_feedback_pct = 0.0;
      break;
    }
    case ModDelayAlgorithm::kChorus: {
      m_min_delay_msec_ = 5;
      m_max_delay_msec_ = 30;
      m_ddl_left_.m_wet_level_pct = 50.0;
      m_ddl_left_.m_feedback_pct = m_feedback_percent_;
      m_ddl_right_.m_wet_level_pct = 50.0;
      m_ddl_right_.m_feedback_pct = m_feedback_percent_;
      break;
    }
    case ModDelayAlgorithm::kFlanger:
    default: {
      m_min_delay_msec_ = 0;
      m_max_delay_msec_ = 7;
      m_ddl_left_.m_wet_level_pct = 50.0;
      m_ddl_left_.m_feedback_pct = m_feedback_percent_;
      m_ddl_right_.m_wet_level_pct = 50.0;
      m_ddl_right_.m_feedback_pct = m_feedback_percent_;
      break;
    }
  }
}

void ModDelay::UpdateLfo() {
  m_lfo_.m_osc_fo = m_mod_freq_;
  m_lfo_.m_waveform =
      m_lfo_type_ == 0 ? 3 : 0;  // tri or sine // dumb and mixed up
  m_lfo_.Update();
}

void ModDelay::UpdateDdl() {
  if (m_mod_type_ != ModDelayAlgorithm::kVibrato) {
    m_ddl_left_.m_feedback_pct = m_feedback_percent_;
    m_ddl_right_.m_feedback_pct = m_feedback_percent_;
  }
  m_ddl_left_.Update();
  m_ddl_right_.Update();
}

double ModDelay::CalculateDelayOffset(double lfo_sample) {
  if (m_mod_type_ == ModDelayAlgorithm::kFlanger ||
      m_mod_type_ == ModDelayAlgorithm::kVibrato) {
    return (m_mod_depth_pct_ / 100.0) *
               (lfo_sample * (m_max_delay_msec_ - m_min_delay_msec_)) +
           m_min_delay_msec_;
  } else if (m_mod_type_ == ModDelayAlgorithm::kChorus) {
    double start = m_min_delay_msec_ + m_chorus_offset_;
    return (m_mod_depth_pct_ / 100.0) *
               (lfo_sample * (m_max_delay_msec_ - m_min_delay_msec_)) +
           start;
  }
  return 0.0;  // shouldn't happen
}

StereoVal ModDelay::ProcessAudio(StereoVal input) {
  double yn = 0;
  double yqn = 0;
  yn = m_lfo_.DoOscillate(&yqn);
  // yn = scaleybum(-1.0, 1.0, 0, 1.0, yn);

  double delay = 0.0;
  // QUAD
  if (m_lfo_phase_ == 1)  // quad
    delay = CalculateDelayOffset(yqn);
  else
    delay = CalculateDelayOffset(yn);

  // Clamp delay to safe range (prevent buffer overrun)
  double max_safe_delay = 95.0;  // Leave 5ms headroom
  if (delay > max_safe_delay) delay = max_safe_delay;
  if (delay < 0) delay = 0;

  m_ddl_left_.m_delay_ms = delay;
  m_ddl_left_.Update();
  m_ddl_right_.m_delay_ms = delay;
  m_ddl_right_.Update();

  // Read delayed outputs for feedback calculation
  double left_delay_out = m_ddl_left_.ReadDelay();
  double right_delay_out = m_ddl_right_.ReadDelay();

  // Calculate feedback: input + (delayed_output * feedback)
  double left_delay_in = input.left + (left_delay_out * m_ddl_left_.m_feedback);
  double right_delay_in =
      input.right + (right_delay_out * m_ddl_right_.m_feedback);

  StereoVal out;
  m_ddl_left_.WriteDelayAndInc(left_delay_in);
  m_ddl_right_.WriteDelayAndInc(right_delay_in);

  // Mix dry and wet signals
  // m_wet_mix_ = 0% -> all dry, 100% -> all wet
  double wet_amount = m_wet_mix_ / 100.0;
  double dry_amount = 1.0 - wet_amount;

  out.left = (input.left * dry_amount) + (left_delay_out * wet_amount);
  out.right = (input.right * dry_amount) + (right_delay_out * wet_amount);

  return out;
}

std::string ModDelay::Status() {
  std::stringstream ss;
  ss << "depth:" << m_mod_depth_pct_;
  ss << " rate:" << m_mod_freq_;
  ss << " fb:" << m_feedback_percent_;
  ss << " offset:" << m_chorus_offset_;
  ss << " mix:" << m_wet_mix_;
  ss << " type:" << GetAlgorithmName(m_mod_type_);
  ss << " lfo:" << (m_lfo_type_ ? "SIN" : "TRI");

  return ss.str();
}

StereoVal ModDelay::Process(StereoVal input) {
  return ProcessAudio(input);
}

void ModDelay::SetDepth(double val) {
  if (val >= 0 && val <= 100)
    m_mod_depth_pct_ = val;
  else
    printf("Val has to be between 0 and 100\n");
  Update();
}

void ModDelay::SetRate(double val) {
  if (val >= 0.02 && val <= 5)
    m_mod_freq_ = val;
  else
    printf("Val has to be between 0.02 and 5\n");
  Update();
}

void ModDelay::SetFeedbackPercent(double val) {
  if (val >= -100 && val <= 100)
    m_feedback_percent_ = val;
  else
    printf("Val has to be between -100 and 100\n");
  Update();
}

void ModDelay::SetChorusOffset(double val) {
  if (val >= 0 && val <= 30)
    m_chorus_offset_ = val;
  else
    printf("Val has to be between 0 and 30\n");
  Update();
}

void ModDelay::SetModType(unsigned int val) {
  switch (val) {
    case (0):
      m_mod_type_ = ModDelayAlgorithm::kFlanger;
      m_min_delay_msec_ = 0;
      m_max_delay_msec_ = 7;
      m_ddl_left_.m_wet_level = 50;
      m_ddl_left_.m_feedback = m_feedback_percent_;
      m_ddl_right_.m_wet_level = 50;
      m_ddl_right_.m_feedback = m_feedback_percent_;
      break;
    case (1):
      m_mod_type_ = ModDelayAlgorithm::kChorus;
      m_min_delay_msec_ = 5;
      m_max_delay_msec_ = 30;
      m_ddl_left_.m_wet_level = 50;
      m_ddl_right_.m_wet_level = 50;
      break;
    case (2):
      m_mod_type_ = ModDelayAlgorithm::kVibrato;
      m_min_delay_msec_ = 0;
      m_max_delay_msec_ = 7;
      m_ddl_left_.m_wet_level = 100;
      m_ddl_left_.m_feedback = 0.0;
      m_ddl_right_.m_wet_level = 100;
      m_ddl_right_.m_feedback = 0.0;
      break;
  }
  Update();
}

void ModDelay::SetLfoType(unsigned int val) {
  if (val < 2)
    m_lfo_type_ = val;
  else
    printf("Val has to be 0 or 1\n");
  Update();
}

void ModDelay::SetWetMix(double val) {
  if (val >= 0 && val <= 100)
    m_wet_mix_ = val;
  else
    printf("Val has to be between 0 and 100\n");
}

void ModDelay::SetParam(std::string name, double val) {
  if (name == "depth") {
    SetDepth(val);
  } else if (name == "rate") {
    SetRate(val);
  } else if (name == "fb") {
    SetFeedbackPercent(val);
  } else if (name == "offset") {
    SetChorusOffset(val);
  } else if (name == "mix") {
    SetWetMix(val);
  } else if (name == "type") {
    SetModType((unsigned int)val);
  } else if (name == "lfo") {
    SetLfoType((unsigned int)val);
  } else {
    printf("Unknown parameter '%s' for ModDelay\n", name.c_str());
  }
}
