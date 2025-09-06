#include <defjams.h>
#include <fx/basicfilterpass.h>
#include <math.h>
#include <stdlib.h>
#include <utils.h>

#include <iostream>
#include <sstream>

FilterPass ::FilterPass() {
  type_ = fx_type::BASICFILTER;
  // m_filter_ = std::make_unique<MoogLadder>();
  // filter_left_ = std::make_unique<FilterSem>();
  filter_left_ = std::make_unique<CKThreeFive>();
  filter_left_->SetFcControl(10000);
  filter_left_->SetQControlGUI(7);
  // filter_right_ = std::make_unique<FilterSem>();
  filter_right_ = std::make_unique<CKThreeFive>();
  filter_right_->SetFcControl(10000);
  filter_right_->SetQControlGUI(7);

  m_lfo1_.m_waveform = SINE;
  m_lfo1_.m_lfo_mode = LFOSYNC;
  m_lfo1_.m_fo = DEFAULT_LFO_RATE;

  m_lfo2_.m_waveform = SINE;
  m_lfo2_.m_lfo_mode = LFOSYNC;
  m_lfo2_.m_fo = DEFAULT_LFO_RATE;

  Update();
  m_lfo1_.StartOscillator();
  m_lfo2_.StartOscillator();

  std::cout << "LFO1 freq is " << m_lfo1_.m_osc_fo << std::endl;
  enabled_ = true;
}

void FilterPass::Update() {
  filter_left_->Update();
  filter_right_->Update();
  m_lfo1_.Update();
  m_lfo2_.Update();
}

std::string FilterPass::Status() {
  std::stringstream ss;
  ss << "freq:" << filter_left_->m_fc_control;
  ss << " q:" << filter_left_->m_q_control;
  ss << " k:" << filter_left_->m_k;
  ss << " type:" << k_filter_type_names[filter_left_->m_filter_type];
  ss << " nlp:" << filter_left_->m_nlp;
  ss << " sat:" << filter_left_->m_saturation;
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
  // double lfo1_val = 0.0;
  // double lfo2_val = 0.0;

  // if (m_lfo1_active_) {
  //   m_lfo1_.Update();
  //   lfo1_val = m_lfo1_.DoOscillate(NULL);
  //   m_filter_->SetFcMod(lfo1_val * FILTER_FC_MOD_RANGE);
  // }

  // if (m_lfo2_active_) {
  //   m_lfo2_.Update();
  //   lfo2_val = m_lfo2_.DoOscillate(NULL);
  //   m_filter_->SetQControl(bipolar_to_unipolar(lfo2_val));
  // }

  StereoVal output;
  filter_left_->Update();
  filter_right_->Update();
  output.left = filter_right_->DoFilter(input.left);
  output.right = filter_right_->DoFilter(input.right);

  return output;
}

void FilterPass::SetParam(std::string name, double val) {
  if (name == "freq") {
    filter_left_->SetFcControl(val);
    filter_right_->SetFcControl(val);
  } else if (name == "q") {
    filter_left_->SetQControlGUI(val);
    filter_right_->SetQControlGUI(val);
  } else if (name == "type") {
    filter_left_->SetType(val);
    filter_right_->SetType(val);
  } else if (name == "nlp") {
    filter_left_->m_nlp = val;
    filter_right_->m_nlp = val;
  } else if (name == "sat") {
    filter_left_->m_saturation = val;
    filter_right_->m_saturation = val;
  } else if (name == "lfo1_active")
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

  Update();
}

void FilterPass::SetLfoType(int lfo_num, unsigned int type) {
  if (type >= MAX_LFO_OSC) {
    std::cout << "Val out of range - must be < " << MAX_LFO_OSC << std::endl;
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
      std::cout << "Only got two LFO's mate - what do you think i am?\n";
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
      std::cout << "Only got two LFO's mate - what do you think i am?\n";
  }
}

void FilterPass::SetLfoRate(int lfo_num, double val) {
  if (val < MIN_LFO_RATE || val > MAX_LFO_RATE) {
    std::cout << "Val out of range - must be between " << MIN_LFO_RATE
              << " and " << MAX_LFO_RATE << std::endl;
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
      std::cout << "Only got two LFO's mate - what do you think i am?\n";
  }
}

void FilterPass::SetLfoAmp(int lfo_num, double val) {
  if (val < 0. || val > 1.) {
    std::cout << "Val out of range - must be between 0 and 1" << std::endl;
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
      std::cout << "Only got two LFO's mate - what do you think i am?\n";
  }
}
