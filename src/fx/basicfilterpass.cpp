#include <defjams.h>
#include <filter_moogladder.h>
#include <fx/basicfilterpass.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <sstream>

const char *filtertype_to_name[] = {"LPF1", "HPF1", "LPF2", "HPF2", "BPF2",
                                    "BSF2", "LPF4", "HPF4", "BPF4"};

FilterPass ::FilterPass() {
  type_ = BASICFILTER;
  enabled_ = true;

  m_filter_.m_aux_control = 0.0;

  m_lfo1_.StartOscillator();
  m_lfo2_.StartOscillator();

  printf("LFO1 freq is %.2f\n", m_lfo1_.m_osc_fo);
}

std::string FilterPass::Status() {
  std::stringstream ss;
  ss << "freq:" << m_filter_.m_fc_control;
  ss << " q:" << m_filter_.m_q_control;
  ss << " type:" << filtertype_to_name[m_filter_.m_filter_type];
  ss << " lfo1_active:" << m_lfo1_active_;
  ss << " lfo1_type:" << m_lfo1_.m_waveform;
  ss << " lfo1_amp:" << m_lfo1_.m_amplitude << "\n";
  ss << "     lfo1_rate:" << m_lfo1_.m_osc_fo;
  ss << " lfo2_active:" << m_lfo2_active_;
  ss << " lfo2_type:" << m_lfo2_.m_waveform;
  ss << " lfo2_amp:" << m_lfo2_.m_amplitude;
  ss << " lfo2_rate:" << m_lfo2_.m_osc_fo;
  return ss.str();
}

StereoVal FilterPass::Process(StereoVal input) {
  double lfo1_val = 0.0;
  double lfo2_val = 0.0;

  if (m_lfo1_active_) {
    m_lfo1_.Update();
    lfo1_val = m_lfo1_.DoOscillate(NULL);
    m_filter_.SetFcMod(lfo1_val * FILTER_FC_MOD_RANGE);
  }

  if (m_lfo2_active_) {
    m_lfo2_.Update();
    lfo2_val = m_lfo2_.DoOscillate(NULL);
    m_filter_.SetQControl(bipolar_to_unipolar(lfo2_val));
  }

  m_filter_.Update();
  input.left = m_filter_.DoFilter(input.left);
  input.right = m_filter_.DoFilter(input.right);

  return input;
}

void FilterPass::SetParam(std::string name, double val) {
  if (name == "freq")
    m_filter_.SetFcControl(val);
  else if (name == "q")
    m_filter_.SetQControl(val);
  else if (name == "type")
    m_filter_.SetType(val);
  else if (name == "lfo1_active")
    SetLfoActive(1, val);
  else if (name == "lfo1_type")
    SetLfoType(1, val);
  else if (name == "lfo1_amp")
    SetLfoAmp(1, val);
  else if (name == "lfo1_rate")
    SetLfoRate(1, val);
  else if (name == "lfo2_active")
    SetLfoActive(2, val);
  else if (name == "lfo2_type")
    SetLfoType(2, val);
  else if (name == "lfo2_amp")
    SetLfoAmp(2, val);
  else if (name == "lfo2_rate")
    SetLfoRate(2, val);
}


void FilterPass::SetLfoType(int lfo_num, unsigned int type) {
  if (type >= MAX_LFO_OSC) {
    printf("Val out of range - must be < %d\n", MAX_LFO_OSC);
    return;
  }
  switch (lfo_num) {
    case (1):
      m_lfo1_.m_waveform = type;
      break;
    case (2):
      m_lfo2_.m_waveform = type;
      break;
    default:
      printf("Only got two LFO's mate - what do you think i am?\n");
  }
}

void FilterPass::SetLfoActive(int lfo_num, bool b) {
  switch (lfo_num) {
    case (1):
      m_lfo1_active_ = b;
      break;
    case (2):
      m_lfo2_active_ = b;
      break;
    default:
      printf("Only got two LFO's mate - what do you think i am?\n");
  }
}

void FilterPass::SetLfoRate(int lfo_num, double val) {
  if (val < MIN_LFO_RATE || val > MAX_LFO_RATE) {
    printf("Val out of range - must be between %f and %f\n", MIN_LFO_RATE,
           MAX_LFO_RATE);
    return;
  }
  switch (lfo_num) {
    case (1):
      m_lfo1_.m_osc_fo = val;
      break;
    case (2):
      m_lfo2_.m_osc_fo = val;
      break;
    default:
      printf("Only got two LFO's mate - what do you think i am?\n");
  }
}

void FilterPass::SetLfoAmp(int lfo_num, double val) {
  if (val < 0. || val > 1.) {
    printf("Val out of range - must be between 0 and 1\n");
    return;
  }
  switch (lfo_num) {
    case (1):
      m_lfo1_.m_amplitude = val;
      break;
    case (2):
      m_lfo2_.m_amplitude = val;
      break;
    default:
      printf("Only got two LFO's mate - what do you think i am?\n");
  }
}
