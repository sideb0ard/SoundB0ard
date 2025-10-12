#include "drum_synth_modules.h"

#include <cassert>
#include <iostream>

namespace SBAudio {

BassDrum::BassDrum() {
  // TRANSIENT
  noise_ = std::make_unique<QBLimitedOscillator>();
  noise_->m_waveform = NOISE;
  noise_->m_amplitude = 0.6;
  noise_->Update();

  noise_eg_.SetRampMode(true);
  noise_eg_.m_reset_to_zero = true;
  noise_eg_.SetEgMode(DIGITAL);
  noise_eg_.SetAttackTimeMsec(5);
  noise_eg_.SetDecayTimeMsec(207);
  noise_eg_.Update();

  // noise_filter_ = std::make_unique<CKThreeFive>();
  noise_filter_ = std::make_unique<FilterSem>();
  noise_filter_->SetType(LPF2);
  noise_filter_->SetFcControl(10000);
  noise_filter_->SetQControlGUI(5);
  noise_filter_->Update();

  // PITCH
  osc1_ = std::make_unique<QBLimitedOscillator>();
  osc1_->m_waveform = SINE;
  osc1_->m_osc_fo = frequency_;
  osc1_->m_amplitude = 0.9;
  osc1_->Update();

  osc2_ = std::make_unique<QBLimitedOscillator>();
  osc2_->m_waveform = TRI;
  osc2_->m_osc_fo = frequency_;
  osc2_->m_amplitude = 0.9;
  osc2_->Update();

  eg_.SetRampMode(true);
  eg_.m_reset_to_zero = true;
  eg_.SetEgMode(DIGITAL);
  eg_.SetAttackTimeMsec(1);
  eg_.SetDecayTimeMsec(180);
  eg_.Update();

  distortion_.SetParam("threshold", 0.5);
  delay_ = std::make_unique<StereoDelay>();

  out_filter_ = std::make_unique<CKThreeFive>();
  out_filter_->SetType(LPF2);
  out_filter_->SetFcControl(10000);
  out_filter_->SetQControlGUI(1);
  out_filter_->Update();
}

void BassDrum::NoteOn(double vel) {
  velocity_ = vel;

  click_.Trigger();

  noise_->StartOscillator();
  noise_eg_.StartEg();

  osc1_->m_osc_fo = frequency_;
  osc1_->StartOscillator();

  osc2_->m_osc_fo = frequency_;
  osc2_->StartOscillator();

  eg_.StartEg();
}

StereoVal BassDrum::Generate() {
  StereoVal out = {.left = 0, .right = 0};
  if (osc1_->m_note_on || noise_->m_note_on) {
    // Transient
    noise_->Update();
    double noise_eg_out = noise_eg_.DoEnvelope(nullptr);
    double noise_out = noise_->DoOscillate(nullptr) * noise_eg_out * 0.2;
    noise_filter_->Update();
    noise_out = noise_filter_->DoFilter(noise_out);

    // OSCILLATORS

    double biased_eg_out = 0;
    double amp_eg_out = eg_.DoEnvelope(&biased_eg_out);

    double eg_osc_mod = OSC_FO_MOD_RANGE * biased_eg_out;
    osc1_->SetFoModExp(eg_osc_mod);
    osc1_->Update();

    osc2_->SetFoModExp(eg_osc_mod);
    osc2_->Update();

    double osc1_out = osc1_->DoOscillate(nullptr) * amp_eg_out;
    if (osc1_->just_wrapped) osc2_->StartOscillator();
    double osc2_out = osc2_->DoOscillate(nullptr) * amp_eg_out;

    double osc_mix = click_.GenNext() + osc1_out + osc2_out;
    if (noise_enabled_) osc_mix += noise_out;

    //// OUTPUT //////////////////////////

    // FILTER ////////////////////
    out_filter_->Update();
    double osc_out = out_filter_->DoFilter(osc_mix);

    double out_left = 0.0;
    double out_right = 0.0;

    dca_.Update();
    dca_.DoDCA(osc_out, osc_out, &out_left, &out_right);

    out = {.left = out_left * velocity_, .right = out_right * velocity_};
    if (use_distortion_) out = distortion_.Process(out);
  }

  if (eg_.GetState() == OFFF) {
    osc1_->StopOscillator();
    osc2_->StopOscillator();
    eg_.StopEg();
  }

  if (noise_eg_.GetState() == OFFF) {
    noise_->StopOscillator();
    noise_eg_.StopEg();
  }

  if (use_delay_) out = delay_->Process(out);
  return out;
}

SnareDrum::SnareDrum() {
  // TRANSIENT
  noise_ = std::make_unique<QBLimitedOscillator>();
  noise_->m_waveform = NOISE;
  noise_->m_amplitude = 0.6;
  noise_->Update();

  noise_eg_.SetRampMode(true);
  noise_eg_.m_reset_to_zero = true;
  noise_eg_.SetEgMode(DIGITAL);
  noise_eg_.SetAttackTimeMsec(1);
  noise_eg_.SetDecayTimeMsec(27);
  noise_eg_.Update();

  noise_filter_ = std::make_unique<CKThreeFive>();
  noise_filter_->SetType(HPF2);
  noise_filter_->SetFcControl(1000);
  noise_filter_->SetQControlGUI(1);
  noise_filter_->Update();

  // PITCH
  lo_osc_ = std::make_unique<QBLimitedOscillator>();
  lo_osc_->m_waveform = SINE;
  lo_osc_->m_osc_fo = low_freq_;
  lo_osc_->m_amplitude = 1;
  lo_osc_->Update();

  hi_osc_ = std::make_unique<QBLimitedOscillator>();
  hi_osc_->m_waveform = SINE;
  hi_osc_->m_osc_fo = high_freq_;
  hi_osc_->m_amplitude = 1;
  hi_osc_->Update();

  eg_.SetRampMode(true);
  eg_.m_reset_to_zero = true;
  eg_.SetEgMode(ANALOG);
  eg_.SetAttackTimeMsec(1);
  eg_.SetDecayTimeMsec(100);
  eg_.Update();

  distortion_.SetParam("threshold", 0.5);
  delay_ = std::make_unique<StereoDelay>();
}

void SnareDrum::NoteOn(double vel) {
  velocity_ = vel;
  noise_->StartOscillator();
  noise_eg_.StartEg();

  lo_osc_->StartOscillator();
  hi_osc_->StartOscillator();

  eg_.StartEg();
}

StereoVal SnareDrum::Generate() {
  StereoVal out = {.left = 0, .right = 0};
  if (lo_osc_->m_note_on) {
    // Transient
    noise_->Update();
    double noise_eg_out = noise_eg_.DoEnvelope(nullptr);
    double noise_out = noise_->DoOscillate(nullptr) * noise_eg_out;
    noise_filter_->Update();
    noise_out = noise_filter_->DoFilter(noise_out);

    // OSCILLATORS

    lo_osc_->Update();
    hi_osc_->Update();

    double lo_osc_out = lo_osc_->DoOscillate(nullptr);
    double hi_osc_out = hi_osc_->DoOscillate(nullptr);

    double osc_out = 0.5 * lo_osc_out + 0.5 * hi_osc_out + noise_out;

    //// OUTPUT //////////////////////////

    // FILTER ////////////////////

    double out_left = 0.0;
    double out_right = 0.0;

    double amp_eg_out = eg_.DoEnvelope(nullptr);
    double dca_mod_val = amp_eg_out;
    dca_.SetEgMod(dca_mod_val);
    dca_.Update();
    dca_.DoDCA(osc_out, osc_out, &out_left, &out_right);

    out = {.left = out_left * velocity_, .right = out_right * velocity_};

    out = distortion_.Process(out);
  }

  if (eg_.GetState() == OFFF) {
    lo_osc_->StopOscillator();
    hi_osc_->StopOscillator();
    noise_->StopOscillator();

    eg_.StopEg();
    noise_eg_.StopEg();
  }

  if (use_delay_) out = delay_->Process(out);
  return out;
}

///// CLAP /////////////////////////////

HandClap::HandClap() {
  noise_ = std::make_unique<QBLimitedOscillator>();
  noise_->m_waveform = NOISE;
  noise_->m_amplitude = 0.6;
  noise_->Update();

  noise_eg_.SetRampMode(true);
  noise_eg_.m_reset_to_zero = true;
  noise_eg_.SetEgMode(DIGITAL);
  noise_eg_.SetAttackTimeMsec(10);
  noise_eg_.SetDecayTimeMsec(207);
  noise_eg_.Update();

  noise_filter_ = std::make_unique<FilterSem>();
  noise_filter_->SetType(BPF2);
  noise_filter_->SetFcControl(1000);
  noise_filter_->SetQControlGUI(5);
  noise_filter_->Update();

  eg_.SetRampMode(true);
  eg_.m_reset_to_zero = true;
  eg_.SetEgMode(ANALOG);
  eg_.SetAttackTimeMsec(10);
  eg_.SetDecayTimeMsec(100);
  eg_.SetSustainLevel(0.3);
  eg_.SetReleaseTimeMsec(200);
  eg_.Update();

  lfo_ = std::make_unique<LFO>();
  lfo_->m_waveform = usaw;
  lfo_->m_osc_fo = 7;
  lfo_->Update();

  distortion_.SetParam("threshold", 0.5);
  delay_ = std::make_unique<StereoDelay>();
}

void HandClap::NoteOn(double vel) {
  velocity_ = vel;
  noise_->StartOscillator();
  lfo_->StartOscillator();
  noise_eg_.StartEg();
  eg_.StartEg();
}

StereoVal HandClap::Generate() {
  StereoVal out = {.left = 0, .right = 0};
  if (noise_->m_note_on) {
    noise_->Update();
    double noise_eg_out = noise_eg_.DoEnvelope(nullptr);
    double noise_out = noise_->DoOscillate(nullptr) * noise_eg_out;
    noise_filter_->Update();
    double filter_out = noise_filter_->DoFilter(noise_out);

    lfo_->Update();
    double lfo_out = lfo_->DoOscillate(nullptr) * noise_out;

    //// OUTPUT //////////////////////////

    double osc_out = lfo_out + filter_out;
    // double osc_out = lfo_out;
    // double osc_out = filter_out;

    // FILTER ////////////////////

    double out_left = 0.0;
    double out_right = 0.0;

    double amp_eg_out = eg_.DoEnvelope(nullptr);
    double dca_mod_val = amp_eg_out;
    dca_.SetEgMod(dca_mod_val);
    dca_.Update();
    dca_.DoDCA(osc_out, osc_out, &out_left, &out_right);

    out = {.left = out_left * velocity_, .right = out_right * velocity_};

    out = distortion_.Process(out);
  }

  if (eg_.GetState() == OFFF) {
    noise_->StopOscillator();
    lfo_->StopOscillator();

    eg_.StopEg();
    noise_eg_.StopEg();
  }

  out = delay_->Process(out);
  return out;
}

SquareOscillatorBank::SquareOscillatorBank() {
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
bool SquareOscillatorBank::IsNoteOn() {
  assert(oscillators_.size() > 0);
  return oscillators_[0]->m_note_on;
}
void SquareOscillatorBank::SetAmplitude(double amp) {
  for (int i = 0; i < kNumOscillators; i++) {
    oscillators_[i]->m_amplitude = amp;
    oscillators_[i]->Update();
  }
}

double SquareOscillatorBank::DoGenerate() {
  double out = 0;
  for (int i = 0; i < kNumOscillators; i++) {
    if (kDefaultOscConfig[i]) {
      oscillators_[i]->Update();
      out += oscillators_[i]->DoOscillate(nullptr);
    }
  }
  return out;
}

HiHat::HiHat() {
  // mid_filter_ = std::make_unique<FilterSem>();
  mid_filter_ = std::make_unique<MoogLadder>();
  mid_filter_->SetType(BPF2);
  mid_filter_->SetFcControl(10000);
  mid_filter_->SetQControlGUI(1);
  mid_filter_->Update();

  high_filter_ = std::make_unique<FilterSem>();
  // ahigh_filter_ = std::make_unique<MoogLadder>();
  high_filter_->SetType(HPF2);
  high_filter_->SetFcControl(7000);
  high_filter_->SetQControlGUI(1);
  high_filter_->Update();

  eg_.SetRampMode(true);
  eg_.m_reset_to_zero = true;
  eg_.SetEgMode(ANALOG);
  eg_.SetAttackTimeMsec(20);
  eg_.SetDecayTimeMsec(10);
  eg_.SetSustainLevel(0.3);
  eg_.SetReleaseTimeMsec(270);
  eg_.Update();

  distortion_.SetParam("threshold", 0.5);
  delay_ = std::make_unique<StereoDelay>();
}

void HiHat::SetAmplitude(double val) {
  osc_bank_.SetAmplitude(val);
}

void HiHat::NoteOn(double vel) {
  velocity_ = vel;
  osc_bank_.Start();
  eg_.StartEg();
}

StereoVal HiHat::Generate() {
  StereoVal out = {.left = 0, .right = 0};
  if (osc_bank_.IsNoteOn()) {
    double square_out = osc_bank_.DoGenerate();
    // std::cout << "SUQOUT:" << square_out << std::endl;
    mid_filter_->Update();
    double mid_out = mid_filter_->DoFilter(square_out);
    // std::cout << "MIDSUQOUT:" << mid_out << std::endl;

    high_filter_->Update();
    double hi_out = high_filter_->DoFilter(mid_out);
    // std::cout << "HIHGUQOUT:" << hi_out << std::endl;

    double eg_out = eg_.DoEnvelope(nullptr);

    double out_left = 0.0;
    double out_right = 0.0;

    dca_.SetEgMod(eg_out);
    dca_.Update();
    dca_.DoDCA(hi_out, hi_out, &out_left, &out_right);
    // dca_.DoDCA(mid_out, mid_out, &out_left, &out_right);

    // std::cout << "OUT:" << out_left << " VEL<OCL:" << velocity_ << std::endl;
    out = {.left = out_left * velocity_, .right = out_right * velocity_};

    out = distortion_.Process(out);
  }

  if (eg_.GetState() == OFFF) {
    osc_bank_.Stop();
    eg_.StopEg();
  }

  out = delay_->Process(out);
  return out;
}

void PulseTrigger::Trigger() {
  pulse_counter_ = 0;
}

double PulseTrigger::GenNext() {
  if (pulse_counter_ < pulse_length_) {
    pulse_counter_++;
    return 1.0 * amplitude_;
  }
  return 0;
}

///// FM DRUM /////////////////////////////

FMDrum::FMDrum() {
  carrier_ = std::make_unique<QBLimitedOscillator>();
  carrier_->m_waveform = SINE;
  carrier_->m_osc_fo = 220;
  carrier_->m_amplitude = 1;
  carrier_->Update();

  eg_.SetRampMode(true);
  eg_.m_reset_to_zero = true;
  eg_.SetEgMode(DIGITAL);
  eg_.SetAttackTimeMsec(10);
  eg_.SetDecayTimeMsec(207);
  eg_.Update();

  modulator_ = std::make_unique<QBLimitedOscillator>();
  modulator_->m_waveform = SINE;
  modulator_->m_osc_fo = carrier_->m_osc_fo * modulator_freq_ratio_;
  modulator_->m_amplitude = 0.6;
  modulator_->Update();

  modulator_eg_.SetRampMode(true);
  modulator_eg_.m_reset_to_zero = true;
  modulator_eg_.SetEgMode(DIGITAL);
  modulator_eg_.SetAttackTimeMsec(8);
  modulator_eg_.SetDecayTimeMsec(180);
  modulator_eg_.Update();

  distortion_.SetParam("threshold", 0.5);
  delay_ = std::make_unique<StereoDelay>();
}

void FMDrum::NoteOn(double vel) {
  velocity_ = vel;

  carrier_->StartOscillator();
  eg_.StartEg();

  modulator_->StartOscillator();
  modulator_eg_.StartEg();
}

StereoVal FMDrum::Generate() {
  StereoVal out = {.left = 0, .right = 0};

  if (carrier_->m_note_on) {
    modulator_->Update();
    double mod_eg_out = modulator_eg_.DoEnvelope(nullptr);
    double mod_out =
        modulator_->DoOscillate(nullptr) * mod_eg_out * modulator_->m_amplitude;

    carrier_->SetPhaseMod(mod_out);
    carrier_->Update();
    double carrier_eg_out = eg_.DoEnvelope(nullptr);
    double carrier_out = carrier_->DoOscillate(nullptr) * carrier_eg_out;

    //// OUTPUT //////////////////////////

    double out_left = 0.0;
    double out_right = 0.0;

    dca_.Update();
    dca_.DoDCA(carrier_out, carrier_out, &out_left, &out_right);

    out = {.left = out_left * velocity_, .right = out_right * velocity_};

    out = distortion_.Process(out);
  }

  if (modulator_eg_.GetState() == OFFF) {
    modulator_->StopOscillator();
    modulator_eg_.StopEg();
  }

  if (eg_.GetState() == OFFF) {
    carrier_->StopOscillator();
    eg_.StopEg();
  }

  out = delay_->Process(out);
  return out;
}

///// LAZER  /////////////////////////////

Lazer::Lazer() {
  eg_.SetRampMode(true);
  eg_.m_reset_to_zero = true;
  eg_.SetEgMode(DIGITAL);
  eg_.SetAttackTimeMsec(10);
  eg_.SetDecayTimeMsec(180);
  eg_.Update();

  osc1_ = std::make_unique<QBLimitedOscillator>();
  osc1_->m_waveform = SINE;
  osc1_->m_osc_fo = 220;
  osc1_->m_amplitude = 1;
  osc1_->Update();
}

void Lazer::NoteOn(double vel) {
  velocity_ = vel;

  osc1_->StartOscillator();
  eg_.StartEg();
}

StereoVal Lazer::Generate() {
  StereoVal out = {.left = 0, .right = 0};
  if (osc1_->m_note_on) {
    double biased_eg_out = 0;
    double amp_eg_out = eg_.DoEnvelope(&biased_eg_out);

    double eg_osc_mod = pitch_osc_range_ * biased_eg_out;
    osc1_->SetFoModExp(eg_osc_mod);
    osc1_->Update();

    double osc_out = osc1_->DoOscillate(nullptr) * amp_eg_out;

    //// OUTPUT //////////////////////////

    double out_left = 0.0;
    double out_right = 0.0;

    dca_.Update();
    dca_.DoDCA(osc_out, osc_out, &out_left, &out_right);

    out = {.left = out_left * velocity_, .right = out_right * velocity_};
  }

  if (eg_.GetState() == OFFF) {
    osc1_->StopOscillator();
    eg_.StopEg();
  }

  return out;
}
}  // namespace SBAudio
