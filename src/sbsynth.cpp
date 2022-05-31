#include <sbsynth.h>

#include <iostream>
#include <sstream>

#include "midi_freq_table.h"

namespace {
std::string GetOscType(int type) {
  std::string the_type = "dunno";
  switch (type) {
    case 0:
      the_type = "SINE";
      break;
    case 1:
      the_type = "SAW1";
      break;
    case 2:
      the_type = "SAW2";
      break;
    case 3:
      the_type = "SAW3";
      break;
    case 4:
      the_type = "TRI";
      break;
    case 5:
      the_type = "SQUARE";
      break;
    case 6:
      the_type = "NOISE";
      break;
    case 7:
      the_type = "PNOISE";
      break;
  }

  return the_type;
}
}  // namespace

namespace SBAudio {

SBSynth::SBSynth() {
  m_car_osc.m_waveform = SQUARE;
  m_mod_osc.m_waveform = SINE;

  m_eg1.SetEgMode(ANALOG);
  m_eg1.SetAttackTimeMsec(20);
  m_eg1.SetDecayTimeMsec(70);
  m_eg1.SetSustainLevel(1);
  m_eg1.SetReleaseTimeMsec(2000);
  m_eg1.m_output_eg = true;

  active = true;
}

stereo_val SBSynth::GenNext(mixer_timing_info tinfo) {
  stereo_val out = {.left = 0, .right = 0};
  if (!active) return out;

  if (m_car_osc.m_note_on) {
    m_eg1.Update();
    double eg_out = m_eg1.DoEnvelope(nullptr);

    m_mod_osc.Update();
    double mod_out = m_mod_osc.DoOscillate(nullptr) * eg_out * m_mod_amp;

    double freq_dev = 5 * m_car_osc.m_osc_fo;

    m_car_osc.SetFoModExp(mod_out * freq_dev);

    m_car_osc.Update();
    double car_out = m_car_osc.DoOscillate(nullptr);

    m_dca.SetEgMod(eg_out);
    m_dca.Update();

    double out_left = 0.0;
    double out_right = 0.0;

    m_dca.DoDCA(car_out, car_out, &out_left, &out_right);

    out = {.left = out_left * volume, .right = out_right * volume};
  }

  if (m_eg1.GetState() == OFFF) {
    m_car_osc.StopOscillator();
    m_mod_osc.StopOscillator();
    m_eg1.StopEg();
  }
  if (out.left > 0) {
    std::cout << "VAL:" << out.left << std::endl;
  }

  return out;
}

void SBSynth::SetParam(std::string name, double val) {
  if (name == "car_osc")
    m_car_osc.m_waveform = val;
  else if (name == "mod_osc")
    m_mod_osc.m_waveform = val;
  if (name == "car_amp") m_car_amp = val;
  if (name == "mod_amp")
    m_mod_amp = val;
  else if (name == "attack")
    m_eg1.SetAttackTimeMsec(val);
  if (name == "decay") m_eg1.SetDecayTimeMsec(val);
  if (name == "sustain") m_eg1.SetSustainLevel(val);
  if (name == "release") m_eg1.SetReleaseTimeMsec(val);
  if (name == "cm_ratio") cm_ratio = val;
}

std::string SBSynth::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << ANSI_COLOR_CYAN;
  ss << "SBSynth car_osc:" << GetOscType(m_car_osc.m_waveform)
     << " mod_osc:" << GetOscType(m_mod_osc.m_waveform)
     << " car_amp:" << m_car_amp << " mod_amp:" << m_mod_amp
     << " cm_ratio:" << cm_ratio << std::endl;

  ss << "     cf:" << m_car_osc.m_osc_fo << " mf:" << m_mod_osc.m_osc_fo
     << " attack:" << m_eg1.m_attack_time_msec
     << " decay:" << m_eg1.m_decay_time_msec
     << " sustain:" << m_eg1.m_sustain_level
     << " release:" << m_eg1.m_release_time_msec << std::endl;

  return ss.str();
}

std::string SBSynth::Info() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << ANSI_COLOR_CYAN;
  ss << "SBSynth~!";

  return ss.str();
}

void SBSynth::start() {
  if (active) return;  // no-op
  active = true;
}
void SBSynth::stop() { active = false; }

void SBSynth::noteOn(midi_event ev) {
  unsigned int midinote = ev.data1;
  unsigned int velocity = ev.data2;

  m_car_osc.m_note_on = true;
  m_car_osc.m_osc_fo = get_midi_freq(midinote);
  m_car_osc.StartOscillator();

  m_mod_osc.m_note_on = true;
  m_mod_osc.m_osc_fo = get_midi_freq(midinote) * cm_ratio;
  m_mod_osc.StartOscillator();

  m_eg1.StartEg();
}

void SBSynth::noteOff(midi_event ev) {
  (void)ev;
  m_eg1.NoteOff();
}

};  // namespace SBAudio
