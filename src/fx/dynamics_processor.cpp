#include <fx/dynamics_processor.h>
#include <mixer.h>
#include <stdlib.h>
#include <utils.h>

#include <sstream>

extern Mixer *mixr;

const char *dynamics_processor_type_to_char[] = {"COMP", "LIMIT", "EXPAND",
                                                 "GATE"};

DynamicsProcessor::DynamicsProcessor() {
  type_ = COMPRESSOR;
  enabled_ = true;

  m_inputgain_db_ = 0;       // -12 - 20
  m_threshold_ = 0;          // -60 - 0
  m_attack_ms_ = 20;         // 1 - 300
  m_release_ms_ = 1000;      // 20 - 5000
  m_ratio_ = 1;              // 1 - 20
  m_outputgain_db_ = 0;      // 0 - 20
  m_knee_width_ = 0;         // 0 - 20
  m_processor_type_ = COMP;  // 0-3 COMP, LIMIT, EXPAND GATE
  m_time_constant_ = 0;      // digital, analog
  m_external_source_ = -99;  //

  Init();
}

void DynamicsProcessor::Init() {
  if (m_time_constant_ == 1)  // digital
  {
    envelope_detector_init(&m_left_detector_, m_attack_ms_, m_release_ms_,
                           false, DETECT_MODE_RMS, true);
    envelope_detector_init(&m_right_detector_, m_attack_ms_, m_release_ms_,
                           false, DETECT_MODE_RMS, true);
  } else {
    envelope_detector_init(&m_left_detector_, m_attack_ms_, m_release_ms_, true,
                           DETECT_MODE_RMS, true);
    envelope_detector_init(&m_right_detector_, m_attack_ms_, m_release_ms_,
                           true, DETECT_MODE_RMS, true);
  }
  delay_init(&m_left_delay_, 300);
  delay_init(&m_right_delay_, 300);
  delay_reset_delay(&m_left_delay_);
  delay_reset_delay(&m_right_delay_);
}

double DynamicsProcessor::CalcCompressionGain(double detector_val,
                                              double threshold, double rratio,
                                              double kneewidth, bool limit) {
  double cs = 1.0 - 1.0 / rratio;
  if (limit) cs = 1;

  if (kneewidth > 0 && detector_val > (threshold - kneewidth / 2.0) &&
      detector_val < threshold + kneewidth / 2.0) {
    double x[2];
    double y[2];
    x[0] = threshold - kneewidth / 2.0;
    x[1] = threshold + kneewidth / 2.0;
    x[1] = min(0, x[1]);
    y[0] = 0;
    y[1] = cs;
    cs = lagrpol(&x[0], &y[0], 2, detector_val);
  }
  double yg = cs * (threshold - detector_val);
  yg = min(0, yg);
  return pow(10.0, yg / 20.0);
}

double DynamicsProcessor::CalcDownwardExpanderGain(double detector_val,
                                                   double threshold,
                                                   double rratio,
                                                   double kneewidth,
                                                   bool gate) {
  double es = 1.0 / rratio - 1;
  if (gate) es = -1;
  if (kneewidth > 0 && detector_val > (threshold - kneewidth / 2.0) &&
      detector_val < threshold + kneewidth / 2.0) {
    double x[2];
    double y[2];
    x[0] = threshold - kneewidth / 2.0;
    x[1] = threshold + kneewidth / 2.0;
    x[1] = min(0, x[1]);
    y[0] = es;
    y[1] = 0;

    es = lagrpol(&x[0], &y[0], 2, detector_val);
  }
  double yg = es * (threshold - detector_val);
  yg = min(0, yg);
  return pow(10.0, yg / 20.0);
}

void DynamicsProcessor::SetInputGainDb(double val) {
  if (val >= -12 && val <= 20)
    m_inputgain_db_ = val;
  else
    printf("Val must be between -12 and 20\n");
}

void DynamicsProcessor::SetThreshold(double val) {
  if (val >= -60 && val <= 0)
    m_threshold_ = val;
  else
    printf("Val must be between -60 and 0\n");
}

void DynamicsProcessor::SetAttackMs(double val) {
  if (val >= 1 && val <= 300) {
    m_attack_ms_ = val;
    envelope_detector_setattacktime(&m_left_detector_, val);
    envelope_detector_setattacktime(&m_right_detector_, val);
  } else
    printf("Val must be between 1 and 300\n");
}

void DynamicsProcessor::SetReleaseMs(double val) {
  if (val >= 20 && val <= 5000) {
    m_release_ms_ = val;
    envelope_detector_setreleasetime(&m_left_detector_, val);
    envelope_detector_setreleasetime(&m_right_detector_, val);
  } else
    printf("Val must be between 20 and 5000\n");
}

void DynamicsProcessor::SetRatio(double val) {
  if (val >= 1 && val <= 20)
    m_ratio_ = val;
  else
    printf("Val must be between 1 and 20\n");
}

void DynamicsProcessor::SetOutputGainDb(double val) {
  if (val >= 0 && val <= 20)
    m_outputgain_db_ = val;
  else
    printf("Val must be between 0 and 20\n");
}

void DynamicsProcessor::SetKneeWidth(double val) {
  if (val >= 0 && val <= 20)
    m_knee_width_ = val;
  else
    printf("Val must be between 0 and 20\n");
}

void DynamicsProcessor::SetLookaheadDelayMs(double val) {
  if (val >= 0 && val <= 300)
    m_lookahead_delay_ms_ = val;
  else
    printf("Val must be between 0 and 300\n");
}

void DynamicsProcessor::SetStereoLink(unsigned int val) {
  if (val < 2)
    m_stereo_link_ = val;
  else
    printf("Val must be 0 or 1\n");
}

void DynamicsProcessor::SetProcessorType(unsigned int val) {
  if (val < 4)
    m_processor_type_ = val;
  else
    printf("Val must be between 0 and 3\n");
}

void DynamicsProcessor::SetTimeConstant(unsigned int val) {
  if (val < 2) {
    m_time_constant_ = val;
    if (val == 0) {  // digital
      envelope_detector_settcmodeanalog(&m_left_detector_, false);
      envelope_detector_settcmodeanalog(&m_right_detector_, false);
    } else {
      envelope_detector_settcmodeanalog(&m_left_detector_, true);
      envelope_detector_settcmodeanalog(&m_right_detector_, true);
    }
  } else
    printf("Val must be between 0 or 1\n");
}

std::string DynamicsProcessor::Status() {
  std::stringstream ss;
  ss << "inputgain:" << m_inputgain_db_;
  ss << " threshold:" << m_threshold_;
  ss << " attackms:" << m_attack_ms_;
  ss << " releasems:" << m_release_ms_;
  ss << " ratio:\n" << m_ratio_;

  ss << "outputgain:" << m_outputgain_db_;
  ss << " kneewidth:" << m_knee_width_;
  ss << " lookahead:" << m_lookahead_delay_ms_;
  ss << " sterolink:" << (m_stereo_link_ ? "ON" : "OFF") << "\n";
  ss << "type:" << dynamics_processor_type_to_char[m_processor_type_];
  ss << " mode:" << (m_time_constant_ ? "DIGITAL" : "ANALOG") << " ("
     << m_time_constant_ << ")";
  ss << " extsource:" << (m_external_source_ < 0 ? "off" : "on") << "("
     << m_external_source_ << ")";

  return ss.str();
}

StereoVal DynamicsProcessor::Process(StereoVal input) {
  // double inputgain = pow(10.0, m_inputgain_db / 20.0);
  double outputgain = pow(10.0, m_outputgain_db_ / 20.0);

  // used in stereo output
  // double xn_l = inputgain * input;
  // double xn_r = 0.;

  double left_detector_input = input.left;
  if (m_external_source_ >= 0)
    left_detector_input = mixr->soundgen_cur_val_[m_external_source_].left;

  double left_detector =
      envelope_detector_detect(&m_left_detector_, left_detector_input);
  double right_detector = left_detector;

  // TODO - something useful with stero

  double link_detector = left_detector;
  double gn = 1.;

  if (m_stereo_link_ == 1) {
    link_detector = 0.5 * (pow(10.0, left_detector / 20.0) +
                           pow(10.0, right_detector / 20.0));
    link_detector = 20.0 * log10(link_detector);
  }

  if (m_processor_type_ == COMP) {
    gn = CalcCompressionGain(link_detector, m_threshold_, m_ratio_,
                             m_knee_width_, false);
  } else if (m_processor_type_ == LIMIT)
    gn = CalcCompressionGain(link_detector, m_threshold_, m_ratio_,
                             m_knee_width_, true);
  else if (m_processor_type_ == EXPAND)
    gn = CalcDownwardExpanderGain(link_detector, m_threshold_, m_ratio_,
                                  m_knee_width_, false);
  else if (m_processor_type_ == GATE)
    gn = CalcDownwardExpanderGain(link_detector, m_threshold_, m_ratio_,
                                  m_knee_width_, true);

  double lookahead_left = 0;
  double lookahead_right = 0;
  delay_process_audio(&m_left_delay_, &input.left, &lookahead_left);
  delay_process_audio(&m_right_delay_, &input.right, &lookahead_right);

  input.left = gn * lookahead_left * outputgain;
  input.right = gn * lookahead_right * outputgain;
  return input;
}

void DynamicsProcessor::SetExternalSource(unsigned int val) {
  if (val < 99) {
    if (mixr->IsValidSoundgenNum(val)) m_external_source_ = val;
  } else
    m_external_source_ = -99;  // TODO less shitty mechanism!
}

void DynamicsProcessor::SetDefaultSidechainParams() {
  SetThreshold(-36);
  SetAttackMs(1);
  SetReleaseMs(55);
  SetRatio(2.5);
}
void DynamicsProcessor::SetParam(std::string name, double val) {
  if (name == "inputgain")
    SetInputGainDb(val);
  else if (name == "threshold")
    SetThreshold(val);
  else if (name == "attackms")
    SetAttackMs(val);
  else if (name == "releasems")
    SetReleaseMs(val);
  else if (name == "ratio")
    SetRatio(val);
  else if (name == "outputgain")
    SetOutputGainDb(val);
  else if (name == "kneewidth")
    SetKneeWidth(val);
  else if (name == "lookahead")
    SetLookaheadDelayMs(val);
  else if (name == "stereolink")
    SetStereoLink(val);
  else if (name == "type")
    SetProcessorType(val);
  else if (name == "mode")
    SetTimeConstant(val);
  else if (name == "extsource")
    SetExternalSource(val);
}
