#include "drum_synth_modules.h"

#include <iostream>

namespace SBAudio {

BassDrum::BassDrum() {
  // TRANSIENT
  noise_ = std::make_unique<QBLimitedOscillator>();
  noise_->m_waveform = NOISE;
  noise_->m_amplitude = 0.3;
  noise_->Update();

  noise_eg_.SetRampMode(true);
  noise_eg_.m_reset_to_zero = true;
  noise_eg_.SetEgMode(DIGITAL);
  noise_eg_.SetAttackTimeMsec(1);
  noise_eg_.SetDecayTimeMsec(7);
  noise_eg_.Update();

  noise_filter_ = std::make_unique<CKThreeFive>();
  noise_filter_->SetType(LPF2);
  noise_filter_->SetFcControl(5000);
  noise_filter_->SetQControlGUI(1);
  noise_filter_->Update();

  // PITCH
  osc1_ = std::make_unique<QBLimitedOscillator>();
  osc1_->m_waveform = SINE;
  osc1_->m_osc_fo = frequency_;
  osc1_->m_amplitude = 1;
  osc1_->Update();

  osc2_ = std::make_unique<QBLimitedOscillator>();
  osc1_->m_waveform = SINE;
  osc1_->m_osc_fo = frequency_;
  osc2_->m_amplitude = 1;
  osc2_->Update();

  eg_.SetRampMode(true);
  eg_.m_reset_to_zero = true;
  eg_.SetEgMode(ANALOG);
  eg_.SetAttackTimeMsec(1);
  eg_.SetDecayTimeMsec(1000);
  eg_.Update();

  distortion_.SetParam("threshold", 0.5);
  out_filter_ = std::make_unique<CKThreeFive>();
  out_filter_->SetType(LPF2);
  out_filter_->SetFcControl(10000);
  out_filter_->SetQControlGUI(1);
  out_filter_->Update();
}

void BassDrum::NoteOn(double amp) {
  noise_->StartOscillator();
  noise_eg_.StartEg();

  osc1_->m_amplitude = amp;
  osc1_->m_osc_fo = frequency_;
  osc1_->StartOscillator();

  osc2_->m_amplitude = amp;
  osc2_->m_osc_fo = frequency_;
  osc2_->StartOscillator();

  eg_.StartEg();
}

StereoVal BassDrum::Generate() {
  StereoVal out = {.left = 0, .right = 0};
  if (osc1_->m_note_on) {
    // Transient
    noise_->Update();
    double noise_eg_out = noise_eg_.DoEnvelope(nullptr);
    double noise_out = noise_->DoOscillate(nullptr) * noise_eg_out;
    noise_filter_->Update();
    noise_out = noise_filter_->DoFilter(noise_out);

    // OSCILLATORS

    double biased_eg_out = 0;
    double amp_eg_out = eg_.DoEnvelope(&biased_eg_out);

    double eg_osc_mod = OSC_FO_MOD_RANGE * biased_eg_out;
    double osc_mod_val = eg_osc_mod;

    osc1_->SetFoModExp(osc_mod_val);
    osc1_->Update();

    osc2_->SetFoModExp(osc_mod_val);
    osc2_->Update();

    double osc1_out = osc1_->DoOscillate(nullptr);
    if (osc1_->just_wrapped) osc2_->StartOscillator();
    double osc2_out = osc2_->DoOscillate(nullptr);

    double osc_mix = 0.5 * osc1_out + 0.5 * osc2_out + noise_out;

    //// OUTPUT //////////////////////////

    // FILTER ////////////////////
    out_filter_->Update();
    double osc_out = out_filter_->DoFilter(osc_mix);

    double out_left = 0.0;
    double out_right = 0.0;

    double dca_mod_val = amp_eg_out;
    dca_.SetEgMod(dca_mod_val);
    dca_.Update();
    dca_.DoDCA(osc_out, osc_out, &out_left, &out_right);

    out = {.left = out_left, .right = out_right};
    out = distortion_.Process(out);
  }

  if (eg_.GetState() == OFFF) {
    osc1_->StopOscillator();
    osc2_->StopOscillator();
    noise_->StopOscillator();

    eg_.StopEg();
    noise_eg_.StopEg();
  }

  return out;
}

const float kSquareOscAmplitude = 0.3;

SquareOscillatorBank::SquareOscillatorBank() {
  std::cout << "YO SQUARE OSC BANK!\n";
  for (const auto &f : kOscFrequencies) {
    auto osc = std::make_unique<QBLimitedOscillator>();
    osc->m_osc_fo = f;
    osc->m_waveform = SQUARE;
    osc->m_amplitude = kSquareOscAmplitude;
    oscillators_.push_back(std::move(osc));
  }
}

void SquareOscillatorBank::Start() {
  for (const auto &o : oscillators_) {
    o->StartOscillator();
  }
}

void SquareOscillatorBank::Stop() {
  for (const auto &o : oscillators_) {
    o->StopOscillator();
  }
}

double SquareOscillatorBank::DoOutput() {
  double out = 0;
  for (int i = 0; i < kNumOscillators; i++) {
    if (kDefaultOscConfig[i]) out += oscillators_[i]->DoOscillate(nullptr);
  }
  return out;
}

}  // namespace SBAudio
