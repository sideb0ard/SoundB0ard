#include <fx/modular_delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

constexpr char const *mod_type_as_string[] = {"FLANGER", "VIBRATO", "CHORUS"};

ModDelay::ModDelay() {
  type_ = MODDELAY;
  enabled_ = true;

  m_min_delay_msec_ = 0.0;
  m_max_delay_msec_ = 0.0;
  m_chorus_offset_ = 0;

  m_ddl_.m_use_external_feedback = false;
  m_ddl_.m_delay_ms = 0.;

  // m_lfo.polarity = 1;
  // m_lfo.mode = 0;

  // "gui"
  m_mod_depth_pct_ = 50;    // percent
  m_mod_freq_ = 0.18;       // range: 0.02 - 5
  m_feedback_percent_ = 0;  //  range: -100 - 100
  m_mod_type_ = FLANGER;    // FLANGER, VIBRATO, CHORUS
  m_lfo_type_ = 0;          // TRI or SINE // these don't match other OSC enums

  Update();
  m_lfo_.StartOscillator();
}

bool ModDelay::Update() {
  CookModType();
  UpdateLfo();
  UpdateDdl();
  return true;
}

void ModDelay::CookModType() {
  switch (m_mod_type_) {
    case VIBRATO: {
      m_min_delay_msec_ = 0;
      m_max_delay_msec_ = 7;
      m_ddl_.m_wet_level_pct = 100.0;
      m_ddl_.m_feedback_pct = 0.0;
      break;
    }
    case CHORUS: {
      m_min_delay_msec_ = 5;
      m_max_delay_msec_ = 30;
      m_ddl_.m_wet_level_pct = 50.0;
      m_ddl_.m_feedback_pct = m_feedback_percent_;
      break;
    }
    case FLANGER:
    default: {
      m_min_delay_msec_ = 0;
      m_max_delay_msec_ = 7;
      m_ddl_.m_wet_level_pct = 50.0;
      m_ddl_.m_feedback_pct = m_feedback_percent_;
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
  if (m_mod_type_ != VIBRATO) m_ddl_.m_feedback_pct = m_feedback_percent_;
  m_ddl_.Update();
}

double ModDelay::CalculateDelayOffset(double lfo_sample) {
  if (m_mod_type_ == FLANGER || m_mod_type_ == VIBRATO) {
    return (m_mod_depth_pct_ / 100.0) *
               (lfo_sample * (m_max_delay_msec_ - m_min_delay_msec_)) +
           m_min_delay_msec_;
  } else if (m_mod_type_ == CHORUS) {
    double start = m_min_delay_msec_ + m_chorus_offset_;
    return (m_mod_depth_pct_ / 100.0) *
               (lfo_sample * (m_max_delay_msec_ - m_min_delay_msec_)) +
           start;
  }
  return 0.0;  // shouldn't happen
}

bool ModDelay::ProcessAudio(double *input_left, double *input_right,
                            double *output_left, double *output_right) {
  (void)input_right;
  (void)output_right;

  double yn = 0;
  double yqn = 0;
  yn = m_lfo_.DoOscillate(&yqn);
  // yn = scaleybum(-1.0, 1.0, 0, 1.0, yn);

  double delay = 0.0;
  // QUAD
  if (m_lfo_phase_ == 1)  // quad
    delay = CalculateDelayOffset(yn);
  else
    delay = CalculateDelayOffset(yqn);

  m_ddl_.m_delay_ms = delay;
  m_ddl_.Update();

  m_ddl_.ProcessAudio(input_left, output_left);

  return true;
}

void ModDelay::Status(char *status_string) {
  snprintf(status_string, MAX_STATIC_STRING_SZ,
           "depth:%.2f rate:%.2f "
           "fb:%.2f offset:%.2f "
           "type:%s lfo:%s",
           m_mod_depth_pct_, m_mod_freq_, m_feedback_percent_, m_chorus_offset_,
           mod_type_as_string[m_mod_type_], m_lfo_type_ ? "SIN" : "TRI");
}

stereo_val ModDelay::Process(stereo_val input) {
  stereo_val out = {};
  ProcessAudio(&input.left, &input.right, &out.left, &out.right);
  return out;
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
  if (val < MAX_MOD_TYPE)
    m_mod_type_ = val;
  else
    printf("Val has to be between 0 and %d\n", MAX_MOD_TYPE - 1);
  Update();
}

void ModDelay::SetLfoType(unsigned int val) {
  if (val < 2)
    m_lfo_type_ = val;
  else
    printf("Val has to be 0 or 1\n");
  Update();
}
void ModDelay::SetParam(std::string name, double val) {}
double ModDelay::GetParam(std::string name) { return 0; }
