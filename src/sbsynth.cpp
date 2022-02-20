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
  m_osc1.m_waveform = SINE;
  m_osc2.m_waveform = SQUARE;

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

  if (m_osc1.m_note_on) {
    m_osc2.Update();
    double mod_output = m_osc2.DoOscillate(nullptr);

    m_osc1.SetFoModExp(m_osc2_amp * OSC_FO_MOD_RANGE * mod_output);
    m_osc1.Update();

    double car_out = m_osc1.DoOscillate(nullptr);  // * m_osc1_amp;

    m_eg1.Update();
    double amp_env_val = 0.0;
    double eg_out = m_eg1.DoEnvelope(&amp_env_val);

    m_dca.SetEgMod(eg_out);
    m_dca.Update();

    double out_left = 0.0;
    double out_right = 0.0;

    m_dca.DoDCA(car_out, car_out, &out_left, &out_right);

    out = {.left = out_left * volume, .right = out_right * volume};
  }

  if (m_eg1.GetState() == OFFF) {
    m_osc1.StopOscillator();
    m_osc2.StopOscillator();
    m_eg1.StopEg();
  }

  return out;
}

void SBSynth::SetParam(std::string name, double val) {
  if (name == "osc1")
    m_osc1.m_waveform = val;
  else if (name == "osc2")
    m_osc2.m_waveform = val;
  if (name == "osc1_amp") m_osc1_amp = val;
  if (name == "osc2_amp")
    m_osc2_amp = val;
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
  ss << "SBSynth osc1:" << GetOscType(m_osc1.m_waveform)
     << " osc2:" << GetOscType(m_osc2.m_waveform) << " osc1_amp:" << m_osc1_amp
     << " osc2_amp:" << m_osc2_amp << " cm_ratio:" << cm_ratio << std::endl;

  ss << "     attack:" << m_eg1.m_attack_time_msec
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

  m_osc1.m_note_on = true;
  m_osc1.m_osc_fo = get_midi_freq(midinote);
  m_osc1.StartOscillator();

  m_osc2.m_note_on = true;
  m_osc2.m_osc_fo = get_midi_freq(midinote) * cm_ratio;
  m_osc2.StartOscillator();

  m_eg1.StartEg();
}

void SBSynth::noteOff(midi_event ev) {
  (void)ev;
  m_eg1.NoteOff();
}

};  // namespace SBAudio
