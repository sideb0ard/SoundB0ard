#include "minisynth.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <iomanip>
#include <iostream>

#include "midi_freq_table.h"
#include "mixer.h"
#include "utils.h"

namespace SBAudio {

extern const char *s_source_enum_to_name[];
extern const char *s_dest_enum_to_name[];

const char *S_VOICES[] = {"SAW3",    "SQR3",    "SAW2SQR",
                          "TRI2SAW", "TRI2SQR", "SIN2SQR"};

char *s_waveform_names[] = {
    (char *)"SINE",  (char *)"SAW1",   (char *)"SAW2",
    (char *)"SAW3",  (char *)"TRI",    (char *)"SQUARE",
    (char *)"NOISE", (char *)"PNOISE", (char *)"MAX_OSC"};
// defined in oscillator.h
const char *s_lfo_mode_names[] = {"sync", "shot", "free"};
const char *s_lfo_wave_names[] = {"sine", "usaw", "dsaw", "tri ",
                                  "squa", "expo", "rsh ", "qrsh"};

const char *s_filter_type_names[] = {"lpf1", "hpf1", "lpf2", "hpf2", "bpf2",
                                     "bsf2", "lpf4", "hpf4", "bpf4"};

MiniSynth::MiniSynth() {
  type = MINISYNTH_TYPE;
  std::cout << "Added, MiniMoog, yo!\n";

  for (int i = 0; i < MAX_VOICES; i++) {
    voices_[i] = std::make_shared<MiniSynthVoice>();
    voices_[i]->InitGlobalParameters(&m_global_synth_params);
  }

  // use first voice to setup global.
  voices_[0]->InitializeModMatrix(&modmatrix);

  for (auto v : voices_) v->SetModMatrixCore(modmatrix.GetModMatrixCore());

  PrepareForPlay();
  LoadDefaults();

  for (auto v : voices_) v->m_dca.m_mod_source_eg = DEST_DCA_EG;

  volume = 1;
  Update();

  active = true;
}

stereo_val MiniSynth::GenNext(mixer_timing_info tinfo) {
  (void)tinfo;
  if (!active) return (stereo_val){0, 0};

  double accum_out_left = 0.0;
  double accum_out_right = 0.0;

  float mix = 0.25;

  double out_left = 0.0;
  double out_right = 0.0;

  for (auto v : voices_) {
    v->DoVoice(&out_left, &out_right);

    accum_out_left += mix * out_left;
    accum_out_right += mix * out_right;
  }

  pan = fmin(pan, 1.0);
  pan = fmax(pan, -1.0);
  double pan_left = 0.707;
  double pan_right = 0.707;
  calculate_pan_values(pan, &pan_left, &pan_right);

  stereo_val out = {.left = accum_out_left * volume * pan_left,
                    .right = accum_out_right * volume * pan_right};

  out = Effector(out);
  return out;
}

std::string MiniSynth::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << ANSI_COLOR_CYAN;
  ss << "Moog(" << m_settings.m_settings_name << ")"
     << " vol:" << volume << " pan:" << pan
     << " voice:" << S_VOICES[m_settings.m_voice_mode] << "("
     << m_settings.m_voice_mode << ")" << ANSI_COLOR_RESET;

  return ss.str();
}

std::string MiniSynth::Info() {
  std::stringstream ss;
  ss << std::setprecision(2) << std::fixed;

  ss << COOL_COLOR_GREEN << "\nMOOG(" << ANSI_COLOR_WHITE
     << m_settings.m_settings_name << COOL_COLOR_GREEN << ")"
     << " vol:" << volume << " pan:" << pan << "\n"
     << COOL_COLOR_YELLOW << "voice:" << S_VOICES[m_settings.m_voice_mode]
     << "(" << m_settings.m_voice_mode << ")\n"
     << "osc1:" << s_waveform_names[m_global_synth_params.osc1_params.waveform]
     << "(" << m_global_synth_params.osc1_params.waveform << ")"
     << " o1amp:" << m_global_synth_params.osc1_params.amplitude
     << " o1oct:" << m_global_synth_params.osc1_params.octave
     << " o1semi:" << m_global_synth_params.osc1_params.semitones
     << " o1cents:" << m_global_synth_params.osc1_params.cents << "\n"

     << "osc2:" << s_waveform_names[m_global_synth_params.osc2_params.waveform]
     << "(" << m_global_synth_params.osc2_params.waveform << ")"
     << " o2amp:" << m_global_synth_params.osc2_params.amplitude
     << " o2oct:" << m_global_synth_params.osc2_params.octave
     << " o2semi:" << m_global_synth_params.osc2_params.semitones
     << " o2cents:" << m_global_synth_params.osc2_params.cents << "\n"

     << "osc3:" << s_waveform_names[m_global_synth_params.osc3_params.waveform]
     << "(" << m_global_synth_params.osc3_params.waveform << ")"
     << " o3amp:" << m_global_synth_params.osc3_params.amplitude
     << " o3oct:" << m_global_synth_params.osc3_params.octave
     << " o3semi:" << m_global_synth_params.osc3_params.semitones
     << " o3cents:" << m_global_synth_params.osc3_params.cents << "\n"

     << "osc4:" << s_waveform_names[m_global_synth_params.osc4_params.waveform]
     << "(" << m_global_synth_params.osc4_params.waveform << ")"
     << " o4amp:" << m_global_synth_params.osc4_params.amplitude
     << " o4oct:" << m_global_synth_params.osc4_params.octave
     << " o4semi:" << m_global_synth_params.osc4_params.semitones
     << " o4cents:" << m_global_synth_params.osc4_params.cents << "\n"

     << ANSI_COLOR_DEEP_RED << "mono:" << m_settings.m_monophonic
     << " hard_sync:" << m_settings.hard_sync
     << " detune:" << m_settings.m_detune_cents
     << " legato:" << m_settings.m_legato_mode
     << " kt:" << m_settings.m_filter_keytrack
     << " ndscale:" << m_settings.m_note_number_to_decay_scaling << "\n"

     << "noisedb:" << m_settings.m_noise_osc_db
     << " octave:" << m_settings.m_octave
     << " pitchrange:" << m_settings.m_pitchbend_range
     << " porta:" << m_settings.m_portamento_time_msec
     << " pw:" << m_settings.m_pulse_width_pct
     << "\nsubosc:" << m_settings.m_sub_osc_db
     << " vascale:" << m_settings.m_velocity_to_attack_scaling
     << " zero:" << m_settings.m_reset_to_zero << "\n"

     << COOL_COLOR_YELLOW
     << "l1wave:" << s_lfo_wave_names[m_settings.m_lfo1_waveform] << "("
     << m_settings.m_lfo1_waveform << ")"
     << " l1mode:" << s_lfo_mode_names[m_settings.m_lfo1_mode] << "("
     << m_settings.m_lfo1_mode << ")"
     << " l1rate:" << m_settings.m_lfo1_rate
     << " l1amp:" << m_settings.m_lfo1_amplitude << COOL_COLOR_ORANGE
     << "\nl1_filter_en:" << m_settings.m_lfo1_filter_fc_enabled
     << "       l1_filter_int:" << m_settings.m_lfo1_filter_fc_intensity
     << "\nl1_filterq_en:" << m_settings.m_lfo1_filter_q_enabled
     << "      l1_filterq_int:" << m_settings.m_lfo1_filter_q_intensity
     << "\nl1_osc_en:" << m_settings.m_lfo1_osc_pitch_enabled
     << "          l1_osc_int:" << m_settings.m_lfo1_osc_pitch_intensity
     << "\nl1_pan_en:" << m_settings.m_lfo1_pan_enabled
     << "          l1_pan_int:" << m_settings.m_lfo1_pan_intensity
     << "\nl1_amp_en:" << m_settings.m_lfo1_amp_enabled
     << "          l1_amp_int:" << m_settings.m_lfo1_amp_intensity
     << "\nl1_pw_en:" << m_settings.m_lfo1_pulsewidth_enabled
     << "           l1_pw_int:" << m_settings.m_lfo1_pulsewidth_intensity
     << "\n"

     << COOL_COLOR_YELLOW
     << "l2wave:" << s_lfo_wave_names[m_settings.m_lfo2_waveform] << "("
     << m_settings.m_lfo2_waveform << ")"
     << " l2mode:" << s_lfo_mode_names[m_settings.m_lfo2_mode] << "("
     << m_settings.m_lfo2_mode << ")"
     << " l2rate:" << m_settings.m_lfo2_rate
     << " l2amp:" << m_settings.m_lfo2_amplitude << COOL_COLOR_PINK
     << "\nl2_filter_en:" << m_settings.m_lfo2_filter_fc_enabled
     << "       l2_filter_int:" << m_settings.m_lfo2_filter_fc_intensity
     << "\nl2_filterq_en:" << m_settings.m_lfo2_filter_q_enabled
     << "      l2_filterq_int:" << m_settings.m_lfo2_filter_q_intensity
     << "\nl2_osc_en:" << m_settings.m_lfo2_osc_pitch_enabled
     << "          l2_osc_int:" << m_settings.m_lfo2_osc_pitch_intensity
     << "\nl2_pan_en:" << m_settings.m_lfo2_pan_enabled
     << "          l2_pan_int:" << m_settings.m_lfo2_pan_intensity
     << "\nl2_amp_en:" << m_settings.m_lfo2_amp_enabled
     << "          l2_amp_int:" << m_settings.m_lfo2_amp_intensity
     << "\nl2_pw_en:" << m_settings.m_lfo2_pulsewidth_enabled
     << "           l2_pw_int:" << m_settings.m_lfo2_pulsewidth_intensity
     << "\n"

     << COOL_COLOR_YELLOW << "eg1_attack:" << m_settings.m_eg1_attack_time_msec
     << " eg1_decay:" << m_settings.m_eg1_decay_time_msec
     << " eg1_release:" << m_settings.m_eg1_release_time_msec
     << " eg1_sus:" << m_settings.m_eg1_sustain_level << COOL_COLOR_ORANGE
     << "\neg1_filter_en:" << m_settings.m_eg1_filter_enabled
     << "      eg1_filter_int:" << m_settings.m_eg1_filter_intensity
     << "\neg1_osc_en:" << m_settings.m_eg1_osc_enabled
     << "         eg1_osc_int:" << m_settings.m_eg1_osc_intensity
     << "\neg1_dca_en:" << m_settings.m_eg1_dca_enabled
     << "         eg1_dca_int:" << m_settings.m_eg1_dca_intensity
     << "\neg1_sustain_override:" << m_settings.m_eg1_sustain_override << "\n"

     << COOL_COLOR_YELLOW << "eg2_attack:" << m_settings.m_eg2_attack_time_msec
     << " eg2_decay:" << m_settings.m_eg2_decay_time_msec
     << " eg2_release:" << m_settings.m_eg2_release_time_msec
     << " eg2_sus:" << m_settings.m_eg2_sustain_level << "\n"
     << COOL_COLOR_PINK << "eg2_filter_en:" << m_settings.m_eg2_filter_enabled
     << "      eg2_filter_int:" << m_settings.m_eg2_filter_intensity
     << "\neg2_osc_en:" << m_settings.m_eg2_osc_enabled
     << "         eg2_osc_int:" << m_settings.m_eg2_osc_intensity
     << "\neg2_dca_en:" << m_settings.m_eg2_dca_enabled
     << "         eg2_dca_int:" << m_settings.m_eg2_dca_intensity
     << "\neg2_sustain_override:" << m_settings.m_eg2_sustain_override << "\n"

     << COOL_COLOR_ORANGE
     << "filter:" << s_filter_type_names[m_settings.m_filter_type] << "("
     << m_settings.m_filter_type << ") fc:" << m_settings.m_fc_control
     << " fq:" << m_settings.m_q_control
     << " kt_int:" << m_settings.m_filter_keytrack_intensity
     << " sat:" << m_settings.m_filter_saturation
     << " nlp:" << m_settings.m_nlp;

  return ss.str();
}
void MiniSynth::start() { active = true; }

void MiniSynth::stop() {
  active = false;
  allNotesOff();
}

void MiniSynth::LoadDefaults() {
  strncpy(m_settings.m_settings_name, "default", 7);

  m_settings.m_monophonic = false;
  m_settings.m_voice_mode = Sqr3;
  m_settings.m_detune_cents = 0.0;

  // OSC1
  m_settings.osc1_wave = SQUARE;
  m_settings.osc1_amp = 1;
  m_settings.osc1_oct = 1;
  m_settings.osc1_semis = 0;
  m_settings.osc1_cents = 0;
  // OSC2
  m_settings.osc2_wave = SQUARE;
  m_settings.osc2_amp = 1;
  m_settings.osc2_oct = 1;
  m_settings.osc2_semis = 0;
  m_settings.osc2_cents = 0;
  // OSC3
  m_settings.osc3_wave = SQUARE;
  m_settings.osc3_amp = 0;
  m_settings.osc3_oct = 0;
  m_settings.osc3_semis = 0;
  m_settings.osc3_cents = 0;
  // OSC4
  m_settings.osc4_wave = NOISE;
  m_settings.osc4_amp = 0;
  m_settings.osc4_oct = 0;
  m_settings.osc4_semis = 0;
  m_settings.osc4_cents = 0;

  // LFO1
  m_settings.m_lfo1_waveform = 0;
  m_settings.m_lfo1_rate = DEFAULT_LFO_RATE;
  m_settings.m_lfo1_amplitude = 1.0;

  // LFO1 routings
  m_settings.m_lfo1_osc_pitch_intensity = 0.7;
  m_settings.m_lfo1_osc_pitch_enabled = false;

  m_settings.m_lfo1_filter_fc_intensity = 0.7;
  m_settings.m_lfo1_filter_fc_enabled = false;

  m_settings.m_lfo1_filter_q_intensity = 0.7;
  m_settings.m_lfo1_filter_q_enabled = false;

  m_settings.m_lfo1_amp_intensity = 0.7;
  m_settings.m_lfo1_amp_enabled = false;

  m_settings.m_lfo1_pan_intensity = 0.7;
  m_settings.m_lfo1_pan_enabled = false;

  m_settings.m_lfo1_pulsewidth_intensity = 0.7;
  m_settings.m_lfo1_pulsewidth_enabled = false;

  // LFO2
  m_settings.m_lfo2_waveform = 0;
  m_settings.m_lfo2_rate = DEFAULT_LFO_RATE;
  m_settings.m_lfo2_amplitude = 1.0;

  // LFO2 routings
  m_settings.m_lfo2_osc_pitch_intensity = 0.7;
  m_settings.m_lfo2_osc_pitch_enabled = false;

  m_settings.m_lfo2_filter_fc_intensity = 0.7;
  m_settings.m_lfo2_filter_fc_enabled = false;

  m_settings.m_lfo2_filter_q_intensity = 0.7;
  m_settings.m_lfo2_filter_q_enabled = false;

  m_settings.m_lfo2_amp_intensity = 0.7;
  m_settings.m_lfo2_amp_enabled = false;

  m_settings.m_lfo2_pan_intensity = 0.7;
  m_settings.m_lfo2_pan_enabled = false;

  m_settings.m_lfo2_pulsewidth_intensity = 0.7;
  m_settings.m_lfo2_pulsewidth_enabled = false;

  m_settings.m_fc_control = FILTER_FC_MAX;
  m_settings.m_q_control = FILTER_Q_DEFAULT;

  /// EG1 //////////////////////////////////////////
  m_settings.m_eg1_osc_intensity = 0.7;
  m_settings.m_eg1_osc_enabled = false;
  m_settings.m_eg1_filter_intensity = 0.7;
  m_settings.m_eg1_filter_enabled = false;
  m_settings.m_eg1_dca_intensity = 1.0;
  m_settings.m_eg1_dca_enabled = true;

  m_settings.m_eg1_attack_time_msec = 5;
  m_settings.m_eg1_decay_time_msec = 300;
  m_settings.m_eg1_release_time_msec = 300;

  m_settings.m_eg1_sustain_level = 0.9;
  m_settings.m_eg1_sustain_override = false;

  /// EG2 //////////////////////////////////////////
  m_settings.m_eg2_osc_intensity = 0.7;
  m_settings.m_eg2_osc_enabled = false;
  m_settings.m_eg2_filter_intensity = 0.7;
  m_settings.m_eg2_filter_enabled = false;
  m_settings.m_eg2_dca_intensity = 1.0;
  m_settings.m_eg2_dca_enabled = false;

  m_settings.m_eg2_attack_time_msec = 52;
  m_settings.m_eg2_decay_time_msec = 30;
  m_settings.m_eg2_release_time_msec = 35;

  m_settings.m_eg2_sustain_level = 0.9;
  m_settings.m_eg2_sustain_override = false;
  ///////////////////////////////////////////////

  m_settings.m_pulse_width_pct = OSC_PULSEWIDTH_DEFAULT;
  m_settings.m_portamento_time_msec = DEFAULT_PORTAMENTO_TIME_MSEC;
  m_settings.m_sub_osc_db = -96.000000;

  m_settings.m_noise_osc_db = -96.000000;
  m_settings.m_legato_mode = DEFAULT_LEGATO_MODE;
  m_settings.m_pitchbend_range = 1;
  m_settings.m_reset_to_zero = DEFAULT_RESET_TO_ZERO;
  m_settings.m_filter_keytrack = DEFAULT_FILTER_KEYTRACK;
  m_settings.m_filter_type = FILTER_TYPE_DEFAULT;
  m_settings.m_filter_keytrack_intensity = DEFAULT_FILTER_KEYTRACK_INTENSITY;

  m_settings.m_velocity_to_attack_scaling = 0;
  m_settings.m_note_number_to_decay_scaling = 0;

  m_settings.m_generate_active = false;
  m_settings.m_generate_src = -99;
}

void MiniSynth::control(midi_event ev) {
  double scaley_val = 0;
  switch (ev.data1) {
    case (1):
      scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, ev.data2);
      m_settings.m_eg1_attack_time_msec = scaley_val;
      break;
    case (2):
      scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, ev.data2);
      m_settings.m_eg1_decay_time_msec = scaley_val;
      break;
    case (3):
      scaley_val = scaleybum(0, 127, 0, 1, ev.data2);
      m_settings.m_eg1_sustain_level = scaley_val;
      break;
    case (4):
      scaley_val = scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, ev.data2);
      m_settings.m_eg1_release_time_msec = scaley_val;
      break;
    case (5):
      // printf("LFO rate\n");
      scaley_val = scaleybum(0, 128, MIN_LFO_RATE, MAX_LFO_RATE, ev.data2);
      m_settings.m_lfo1_rate = scaley_val;
      break;
    case (6):
      // printf("LFO amp\n");
      scaley_val = scaleybum(0, 128, 0.0, 1.0, ev.data2);
      m_settings.m_lfo1_amplitude = scaley_val;
      break;
    case (7):
      // printf("Filter CutOff\n");
      scaley_val = scaleybum(0, 127, FILTER_FC_MIN, FILTER_FC_MAX, ev.data2);
      m_settings.m_fc_control = scaley_val;
      break;
    case (8):
      // printf("Filter Q\n");
      scaley_val = scaleybum(0, 127, 0.02, 10, ev.data2);
      m_settings.m_q_control = scaley_val;
      break;
    default:
      printf("nah\n");
  }
  Update();
}

void MiniSynth::noteOn(midi_event ev) {
  unsigned int midinote = ev.data1;
  unsigned int velocity = ev.data2;

  if (m_settings.m_monophonic) {
    auto msv = voices_[0];
    msv->NoteOn(midinote, velocity, get_midi_freq(midinote),
                m_last_note_frequency);
    m_last_note_frequency = get_midi_freq(midinote);
    return;
  }

  bool steal_note = true;
  int vx = 0;
  for (auto v : voices_) {
    if (!v->m_note_on) {
      IncrementVoiceTimestamps();
      v->NoteOn(midinote, velocity, get_midi_freq(midinote),
                m_last_note_frequency);

      m_last_note_frequency = get_midi_freq(midinote);
      steal_note = false;
      break;
    }
    ++vx;
  }

  if (steal_note) {
    auto msv = GetOldestVoice();
    if (msv) {
      IncrementVoiceTimestamps();
      msv->NoteOn(midinote, velocity, get_midi_freq(midinote),
                  m_last_note_frequency);
    }
    m_last_note_frequency = get_midi_freq(midinote);
  }
}

void MiniSynth::allNotesOff() {
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices_[i]) voices_[i]->NoteOff(-1);
  }
}

void MiniSynth::noteOff(midi_event ev) {
  for (int i = 0; i < MAX_VOICES; i++) {
    auto msv = GetOldestVoiceWithNote(ev.data1);
    if (msv) msv->NoteOff(ev.data1);
  }
}

void MiniSynth::pitchBend(midi_event ev) {
  unsigned int data1 = ev.data1;
  unsigned int data2 = ev.data2;
  // printf("Pitch bend, babee: %d %d\n", data1, data2);
  int actual_pitch_bent_val = (int)((data1 & 0x7F) | ((data2 & 0x7F) << 7));

  if (actual_pitch_bent_val != 8192) {
    double normalized_pitch_bent_val =
        (float)(actual_pitch_bent_val - 0x2000) / (float)(0x2000);
    double scaley_val =
        // scaleybum(0, 16383, -100, 100, normalized_pitch_bent_val);
        scaleybum(0, 16383, -600, 600, actual_pitch_bent_val);
    // printf("Cents to bend - %f\n", scaley_val);
    for (int i = 0; i < MAX_VOICES; i++) {
      voices_[i]->m_osc1->m_cents = scaley_val;
      voices_[i]->m_osc2->m_cents = scaley_val + 2.5;
      voices_[i]->m_osc3->m_cents = scaley_val;
      voices_[i]->m_osc4->m_cents = scaley_val + 2.5;
      voices_[i]->modmatrix.sources[SOURCE_PITCHBEND] =
          normalized_pitch_bent_val;
    }
  } else {
    for (int i = 0; i < MAX_VOICES; i++) {
      voices_[i]->m_osc1->m_cents = 0;
      voices_[i]->m_osc2->m_cents = 2.5;
      voices_[i]->m_osc3->m_cents = 0;
      voices_[i]->m_osc4->m_cents = 2.5;
    }
  }
}

////////////////////////////////////

bool MiniSynth::PrepareForPlay() {
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices_[i]) voices_[i]->PrepareForPlay();
  }

  m_last_note_frequency = -1.0;

  return true;
}

void MiniSynth::Update() {
  m_global_synth_params.voice_params.hard_sync = m_settings.hard_sync;
  m_global_synth_params.voice_params.voice_mode = m_settings.m_voice_mode;
  m_global_synth_params.voice_params.portamento_time_msec =
      m_settings.m_portamento_time_msec;

  m_global_synth_params.voice_params.osc_fo_pitchbend_mod_range =
      m_settings.m_pitchbend_range;

  // --- intensities
  m_global_synth_params.voice_params.filter_keytrack_intensity =
      m_settings.m_filter_keytrack_intensity;

  m_global_synth_params.voice_params.lfo1_filter1_mod_intensity =
      m_settings.m_lfo1_filter_fc_intensity;
  m_global_synth_params.voice_params.lfo1_filter1_q_mod_intensity =
      m_settings.m_lfo1_filter_q_intensity;
  m_global_synth_params.voice_params.lfo1_osc_mod_intensity =
      m_settings.m_lfo1_osc_pitch_intensity;
  m_global_synth_params.voice_params.lfo1_dca_amp_mod_intensity =
      m_settings.m_lfo1_amp_intensity;
  m_global_synth_params.voice_params.lfo1_dca_pan_mod_intensity =
      m_settings.m_lfo1_pan_intensity;

  m_global_synth_params.voice_params.lfo2_filter1_mod_intensity =
      m_settings.m_lfo2_filter_fc_intensity;
  m_global_synth_params.voice_params.lfo2_filter1_q_mod_intensity =
      m_settings.m_lfo2_filter_q_intensity;
  m_global_synth_params.voice_params.lfo2_osc_mod_intensity =
      m_settings.m_lfo2_osc_pitch_intensity;
  m_global_synth_params.voice_params.lfo2_dca_amp_mod_intensity =
      m_settings.m_lfo2_amp_intensity;
  m_global_synth_params.voice_params.lfo2_dca_pan_mod_intensity =
      m_settings.m_lfo2_pan_intensity;

  m_global_synth_params.voice_params.eg1_osc_mod_intensity =
      m_settings.m_eg1_osc_intensity;
  m_global_synth_params.voice_params.eg1_filter1_mod_intensity =
      m_settings.m_eg1_filter_intensity;
  m_global_synth_params.voice_params.eg1_dca_amp_mod_intensity =
      m_settings.m_eg1_dca_intensity;

  // --- oscillators:
  m_global_synth_params.osc1_params.amplitude = m_settings.osc1_amp;
  m_global_synth_params.osc1_params.octave = m_settings.osc1_oct;
  m_global_synth_params.osc1_params.semitones = m_settings.osc1_semis;

  m_global_synth_params.osc2_params.amplitude = m_settings.osc2_amp;
  m_global_synth_params.osc2_params.octave = m_settings.osc2_oct;
  m_global_synth_params.osc2_params.semitones = m_settings.osc2_semis;

  double noise_amplitude = m_settings.m_noise_osc_db == -96.0
                               ? 0.0
                               : pow(10.0, m_settings.m_noise_osc_db / 20.0);
  double sub_amplitude = m_settings.m_sub_osc_db == -96.0
                             ? 0.0
                             : pow(10.0, m_settings.m_sub_osc_db / 20.0);

  // --- osc3 is sub osc
  m_global_synth_params.osc3_params.amplitude = sub_amplitude;
  // m_global_synth_params.osc3_params.amplitude = m_settings.osc3_amp;
  m_global_synth_params.osc3_params.octave = m_settings.osc3_oct;
  m_global_synth_params.osc3_params.semitones = m_settings.osc3_semis;

  // --- osc4 is noise osc
  m_global_synth_params.osc4_params.amplitude = noise_amplitude;
  // m_global_synth_params.osc4_params.amplitude = m_settings.osc4_amp;
  m_global_synth_params.osc4_params.octave = m_settings.osc4_oct;
  m_global_synth_params.osc4_params.semitones = m_settings.osc4_semis;

  // --- pulse width
  m_global_synth_params.osc1_params.pulse_width_control =
      m_settings.m_pulse_width_pct;
  m_global_synth_params.osc2_params.pulse_width_control =
      m_settings.m_pulse_width_pct;
  m_global_synth_params.osc3_params.pulse_width_control =
      m_settings.m_pulse_width_pct;

  // --- octave
  m_global_synth_params.osc1_params.octave = m_settings.m_octave;
  m_global_synth_params.osc2_params.octave = m_settings.m_octave;
  m_global_synth_params.osc3_params.octave =
      m_settings.m_octave - 1;  // sub-oscillator

  // --- detuning for minisynth
  m_global_synth_params.osc1_params.cents = m_settings.m_detune_cents;
  m_global_synth_params.osc2_params.cents = -m_settings.m_detune_cents;
  // no detune on 3rd oscillator

  // --- filter:
  m_global_synth_params.filter1_params.fc_control = m_settings.m_fc_control;
  m_global_synth_params.filter1_params.q_control = m_settings.m_q_control;
  m_global_synth_params.filter1_params.filter_type = m_settings.m_filter_type;
  m_global_synth_params.filter1_params.saturation =
      m_settings.m_filter_saturation;
  m_global_synth_params.filter1_params.nlp = m_settings.m_nlp;

  // --- lfo1:
  m_global_synth_params.lfo1_params.waveform = m_settings.m_lfo1_waveform;
  m_global_synth_params.lfo1_params.amplitude = m_settings.m_lfo1_amplitude;
  m_global_synth_params.lfo1_params.osc_fo = m_settings.m_lfo1_rate;
  m_global_synth_params.lfo1_params.lfo_mode = m_settings.m_lfo1_mode;

  // --- lfo2:
  m_global_synth_params.lfo2_params.waveform = m_settings.m_lfo2_waveform;
  m_global_synth_params.lfo2_params.amplitude = m_settings.m_lfo2_amplitude;
  m_global_synth_params.lfo2_params.osc_fo = m_settings.m_lfo2_rate;
  m_global_synth_params.lfo2_params.lfo_mode = m_settings.m_lfo2_mode;

  // --- eg1:
  m_global_synth_params.eg1_params.attack_time_msec =
      m_settings.m_eg1_attack_time_msec;
  m_global_synth_params.eg1_params.decay_time_msec =
      m_settings.m_eg1_decay_time_msec;
  m_global_synth_params.eg1_params.sustain_level =
      m_settings.m_eg1_sustain_level;
  m_global_synth_params.eg1_params.release_time_msec =
      m_settings.m_eg1_release_time_msec;
  m_global_synth_params.eg1_params.reset_to_zero =
      (bool)m_settings.m_reset_to_zero;
  m_global_synth_params.eg1_params.legato_mode = (bool)m_settings.m_legato_mode;
  m_global_synth_params.eg1_params.sustain_override =
      (bool)m_settings.m_eg1_sustain_override;

  // --- dca:
  m_global_synth_params.dca_params.amplitude_db = volume;

  // --- enable/disable mod matrix stuff
  // LFO1 routings
  if (m_settings.m_lfo1_osc_pitch_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_ALL_OSC_FO,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_ALL_OSC_FO, false);

  if (m_settings.m_lfo1_filter_fc_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_ALL_FILTER_FC,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_ALL_FILTER_FC, false);

  if (m_settings.m_lfo1_filter_q_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_ALL_FILTER_Q,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_ALL_FILTER_Q, false);

  if (m_settings.m_lfo1_amp_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_DCA_AMP,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_DCA_AMP, false);

  if (m_settings.m_lfo1_pan_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_DCA_PAN,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_DCA_PAN, false);

  if (m_settings.m_lfo1_pulsewidth_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_ALL_OSC_PULSEWIDTH,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO1, DEST_ALL_OSC_PULSEWIDTH, false);

  // LFO2
  if (m_settings.m_lfo2_osc_pitch_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_ALL_OSC_FO,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_ALL_OSC_FO, false);

  if (m_settings.m_lfo2_filter_fc_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_ALL_FILTER_FC,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_ALL_FILTER_FC, false);

  if (m_settings.m_lfo2_filter_q_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_ALL_FILTER_Q,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_ALL_FILTER_Q, false);

  if (m_settings.m_lfo2_amp_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_DCA_AMP,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_DCA_AMP, false);

  if (m_settings.m_lfo2_pan_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_DCA_PAN,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_DCA_PAN, false);

  if (m_settings.m_lfo2_pulsewidth_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_ALL_OSC_PULSEWIDTH,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_LFO2, DEST_ALL_OSC_PULSEWIDTH, false);

  // EG1 routings
  if (m_settings.m_eg1_osc_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_BIASED_EG1, DEST_ALL_OSC_FO,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_BIASED_EG1, DEST_ALL_OSC_FO, false);

  if (m_settings.m_eg1_filter_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_BIASED_EG1, DEST_ALL_FILTER_FC,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_BIASED_EG1, DEST_ALL_FILTER_FC, false);

  if (m_settings.m_eg1_dca_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_EG1, DEST_DCA_EG,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_EG1, DEST_DCA_EG, false);

  // EG2 routings
  if (m_settings.m_eg2_osc_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_BIASED_EG2, DEST_ALL_OSC_FO,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_BIASED_EG2, DEST_ALL_OSC_FO, false);

  if (m_settings.m_eg2_filter_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_BIASED_EG2, DEST_ALL_FILTER_FC,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_BIASED_EG2, DEST_ALL_FILTER_FC, false);

  if (m_settings.m_eg2_dca_enabled == 1)
    modmatrix.EnableMatrixRow(SOURCE_EG2, DEST_DCA_EG,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_EG2, DEST_DCA_EG, false);

  // Velocity to Attack
  if (m_settings.m_velocity_to_attack_scaling == 1)
    modmatrix.EnableMatrixRow(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_VELOCITY, DEST_ALL_EG_ATTACK_SCALING,
                              false);

  // Note Number to Decay
  if (m_settings.m_note_number_to_decay_scaling == 1)
    modmatrix.EnableMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_EG_DECAY_SCALING,
                              false);

  // Filter Keytrack
  if (m_settings.m_filter_keytrack == 1)
    modmatrix.EnableMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK,
                              true);  // enable
  else
    modmatrix.EnableMatrixRow(SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK,
                              false);

  for (int i = 0; i < MAX_VOICES; i++)
    if (voices_[i]) voices_[i]->Update();
}

void MiniSynth::ResetVoices() {
  for (int i = 0; i < MAX_VOICES; i++) voices_[i]->Reset();
}

void MiniSynth::IncrementVoiceTimestamps() {
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices_[i]->m_note_on) voices_[i]->m_timestamp++;
  }
}

std::shared_ptr<MiniSynthVoice> MiniSynth::GetOldestVoice() {
  int timestamp = -1;
  std::shared_ptr<MiniSynthVoice> found_voice = NULL;
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices_[i]) {
      if (voices_[i]->m_note_on && (int)voices_[i]->m_timestamp > timestamp) {
        found_voice = voices_[i];
        timestamp = (int)voices_[i]->m_timestamp;
      }
    }
  }
  return found_voice;
}

std::shared_ptr<MiniSynthVoice> MiniSynth::GetOldestVoiceWithNote(
    int midi_note) {
  int timestamp = -1;
  std::shared_ptr<MiniSynthVoice> found_voice = NULL;
  for (auto v : voices_) {
    if (v->CanNoteOff() && v->m_timestamp > timestamp &&
        v->m_midi_note_number == midi_note) {
      found_voice = v;
      timestamp = (int)v->m_timestamp;
    }
  }
  return found_voice;
}

void MiniSynth::randomize() {
  // printf("Randomizing SYNTH!\n");

  strncpy(m_settings.m_settings_name, "-- random UNSAVED--", 256);
  m_settings.m_voice_mode = rand() % MAX_VOICE_CHOICE;
  m_settings.m_monophonic = rand() % 2;

  m_settings.m_lfo1_waveform = rand() % MAX_LFO_OSC;
  m_settings.m_lfo1_rate =
      ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) + MIN_LFO_RATE;
  m_settings.m_lfo1_amplitude = ((float)rand()) / RAND_MAX;
  m_settings.m_lfo1_osc_pitch_intensity =
      (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
  m_settings.m_lfo1_osc_pitch_enabled = rand() % 2;
  m_settings.m_lfo1_filter_fc_intensity =
      (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
  m_settings.m_lfo1_filter_fc_enabled = rand() % 2;
  m_settings.m_filter_type = rand() % NUM_FILTER_TYPES;
  m_settings.m_lfo1_amp_intensity = ((float)rand() / (float)(RAND_MAX));
  // m_settings.m_lfo1_amp_enabled = rand() % 2;
  // m_settings.m_lfo1_pan_intensity = ((float)rand() /
  //(float)(RAND_MAX));
  // m_settings.m_lfo1_pan_enabled = rand() % 2;
  m_settings.m_lfo1_pulsewidth_intensity = ((float)rand() / (float)(RAND_MAX));
  m_settings.m_lfo1_pulsewidth_enabled = rand() % 2;

  m_settings.m_lfo2_waveform = rand() % MAX_LFO_OSC;
  m_settings.m_lfo2_rate =
      ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) + MIN_LFO_RATE;
  m_settings.m_lfo2_amplitude = ((float)rand()) / RAND_MAX;
  m_settings.m_lfo2_osc_pitch_intensity =
      (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
  m_settings.m_lfo2_osc_pitch_enabled = rand() % 2;
  m_settings.m_lfo2_filter_fc_intensity =
      (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
  m_settings.m_lfo2_filter_fc_enabled = rand() % 2;
  m_settings.m_lfo2_amp_intensity = ((float)rand() / (float)(RAND_MAX));
  // m_settings.m_lfo2_amp_enabled = rand() % 2;
  // m_settings.m_lfo2_pan_intensity = ((float)rand() /
  //(float)(RAND_MAX));
  // m_settings.m_lfo2_pan_enabled = rand() % 2;
  m_settings.m_lfo2_pulsewidth_intensity = ((float)rand() / (float)(RAND_MAX));
  m_settings.m_lfo2_pulsewidth_enabled = rand() % 2;

  m_settings.m_detune_cents = (rand() % 200) - 100;

  m_settings.m_fc_control =
      ((float)rand()) / RAND_MAX * (FILTER_FC_MAX - FILTER_FC_MIN) +
      FILTER_FC_MIN;
  m_settings.m_q_control = (rand() % 8) + 1;

  m_settings.m_eg1_attack_time_msec = (rand() % 700) + 5;
  m_settings.m_eg1_decay_time_msec = (rand() % 700) + 5;
  m_settings.m_eg1_release_time_msec = (rand() % 600) + 5;
  m_settings.m_pulse_width_pct = (rand() % 99) + 1;

  // m_settings.m_sustain_level = ((float)rand()) / RAND_MAX;
  m_settings.m_octave = rand() % 3 + 1;

  m_settings.m_portamento_time_msec = rand() % 5000;

  m_settings.m_sub_osc_db = -1.0 * (rand() % 96);
  // m_settings.m_eg1_osc_intensity =
  //    (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
  m_settings.m_eg1_filter_intensity =
      (((float)rand() / (float)(RAND_MAX)) * 2) - 1;
  m_settings.m_noise_osc_db = -1.0 * (rand() % 96);

  //////// m_settings.m_volume_db = 1.0;
  m_settings.m_legato_mode = rand() % 2;
  // m_settings.m_pitchbend_range = rand() % 12;
  m_settings.m_reset_to_zero = rand() % 2;
  m_settings.m_filter_keytrack = rand() % 2;
  m_settings.m_filter_keytrack_intensity =
      (((float)rand() / (float)(RAND_MAX)) * 9) + 0.51;
  m_settings.m_velocity_to_attack_scaling = rand() % 2;
  m_settings.m_note_number_to_decay_scaling = rand() % 2;
  ////m_settings.m_eg1_dca_intensity =
  ////    (((float)rand() / (float)(RAND_MAX)) * 2.0) - 1;
  // m_settings.m_sustain_override = rand() % 2;

  Update();

  // minisynth_print_settings(ms);
}

void MiniSynth::Save(std::string new_preset_name) {
  if (new_preset_name.empty()) {
    printf(
        "Play tha game, pal, need a name to save yer synth settings "
        "with\n");
    return;
  }
  const char *preset_name = new_preset_name.c_str();

  printf("Saving '%s' settings for Minisynth to file %s\n", preset_name,
         MOOG_PRESET_FILENAME);
  FILE *presetzzz = fopen(MOOG_PRESET_FILENAME, "a+");
  if (presetzzz == NULL) {
    printf("Couldn't save settings!!\n");
    return;
  }

  int settings_count = 0;
  strncpy(m_settings.m_settings_name, preset_name, 256);

  fprintf(presetzzz, "::name=%s", m_settings.m_settings_name);
  settings_count++;

  fprintf(presetzzz, "::voice_mode=%d", m_settings.m_voice_mode);
  settings_count++;

  fprintf(presetzzz, "::monophonic=%d", m_settings.m_monophonic);
  settings_count++;

  // LFO1
  fprintf(presetzzz, "::lfo1_waveform=%d", m_settings.m_lfo1_waveform);
  settings_count++;
  fprintf(presetzzz, "::lfo1_dest=%d", m_settings.m_lfo1_dest);
  settings_count++;
  fprintf(presetzzz, "::lfo1_mode=%d", m_settings.m_lfo1_mode);
  settings_count++;
  fprintf(presetzzz, "::lfo1_rate=%f", m_settings.m_lfo1_rate);
  settings_count++;
  fprintf(presetzzz, "::lfo1_amp=%f", m_settings.m_lfo1_amplitude);
  settings_count++;
  fprintf(presetzzz, "::lfo1_osc_pitch_intensity=%f",
          m_settings.m_lfo1_osc_pitch_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo1_osc_pitch_enabled=%d",
          m_settings.m_lfo1_osc_pitch_enabled);
  settings_count++;
  fprintf(presetzzz, "::lfo1_filter_fc_intensity=%f",
          m_settings.m_lfo1_filter_fc_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo1_filter_fc_enabled=%d",
          m_settings.m_lfo1_filter_fc_enabled);
  settings_count++;
  fprintf(presetzzz, "::lfo1_amp_intensity=%f",
          m_settings.m_lfo1_amp_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo1_amp_enabled=%d", m_settings.m_lfo1_amp_enabled);
  settings_count++;
  fprintf(presetzzz, "::lfo1_pan_intensity=%f",
          m_settings.m_lfo1_pan_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo1_pan_enabled=%d", m_settings.m_lfo1_pan_enabled);
  settings_count++;
  fprintf(presetzzz, "::lfo1_pulsewidth_intensity=%f",
          m_settings.m_lfo1_pulsewidth_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo1_pulsewidth_enabled=%d",
          m_settings.m_lfo1_pulsewidth_enabled);
  settings_count++;

  // LFO2
  fprintf(presetzzz, "::lfo2_waveform=%d", m_settings.m_lfo2_waveform);
  settings_count++;
  fprintf(presetzzz, "::lfo2_dest=%d", m_settings.m_lfo2_dest);
  settings_count++;
  fprintf(presetzzz, "::lfo2_mode=%d", m_settings.m_lfo2_mode);
  settings_count++;
  fprintf(presetzzz, "::lfo2_rate=%f", m_settings.m_lfo2_rate);
  settings_count++;
  fprintf(presetzzz, "::lfo2_amp=%f", m_settings.m_lfo2_amplitude);
  settings_count++;
  fprintf(presetzzz, "::lfo2_osc_pitch_intensity=%f",
          m_settings.m_lfo2_osc_pitch_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo2_osc_pitch_enabled=%d",
          m_settings.m_lfo2_osc_pitch_enabled);
  settings_count++;
  fprintf(presetzzz, "::lfo2_filter_fc_intensity=%f",
          m_settings.m_lfo2_filter_fc_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo2_filter_fc_enabled=%d",
          m_settings.m_lfo2_filter_fc_enabled);
  settings_count++;
  fprintf(presetzzz, "::lfo2_amp_intensity=%f",
          m_settings.m_lfo2_amp_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo2_amp_enabled=%d", m_settings.m_lfo2_amp_enabled);
  settings_count++;
  fprintf(presetzzz, "::lfo2_pan_intensity=%f",
          m_settings.m_lfo2_pan_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo2_pan_enabled=%d", m_settings.m_lfo2_pan_enabled);
  settings_count++;
  fprintf(presetzzz, "::lfo2_pulsewidth_intensity=%f",
          m_settings.m_lfo2_pulsewidth_intensity);
  settings_count++;
  fprintf(presetzzz, "::lfo2_pulsewidth_enabled=%d",
          m_settings.m_lfo2_pulsewidth_enabled);
  settings_count++;
  // EG1
  fprintf(presetzzz, "::attack_time_msec=%f",
          m_settings.m_eg1_attack_time_msec);
  settings_count++;
  fprintf(presetzzz, "::decay_time_msec=%f", m_settings.m_eg1_decay_time_msec);
  settings_count++;
  fprintf(presetzzz, "::release_time_msec=%f",
          m_settings.m_eg1_release_time_msec);
  settings_count++;
  fprintf(presetzzz, "::sustain_level=%f", m_settings.m_eg1_sustain_level);
  settings_count++;

  fprintf(presetzzz, "::volume_db=%f", volume);
  settings_count++;
  fprintf(presetzzz, "::fc_control=%f", m_settings.m_fc_control);
  settings_count++;
  fprintf(presetzzz, "::q_control=%f", m_settings.m_q_control);
  settings_count++;

  fprintf(presetzzz, "::detune_cents=%f", m_settings.m_detune_cents);
  settings_count++;
  fprintf(presetzzz, "::pulse_width_pct=%f", m_settings.m_pulse_width_pct);
  settings_count++;
  fprintf(presetzzz, "::sub_osc_db=%f", m_settings.m_sub_osc_db);
  settings_count++;
  fprintf(presetzzz, "::noise_osc_db=%f", m_settings.m_noise_osc_db);
  settings_count++;

  fprintf(presetzzz, "::eg1_osc_intensity=%f", m_settings.m_eg1_osc_intensity);
  settings_count++;
  fprintf(presetzzz, "::eg1_osc_enabled=%d", m_settings.m_eg1_osc_enabled);
  settings_count++;

  fprintf(presetzzz, "::eg1_filter_intensity=%f",
          m_settings.m_eg1_filter_intensity);
  settings_count++;
  fprintf(presetzzz, "::eg1_filter_enabled=%d",
          m_settings.m_eg1_filter_enabled);
  settings_count++;

  fprintf(presetzzz, "::eg1_dca_intensity=%f", m_settings.m_eg1_dca_intensity);
  settings_count++;
  fprintf(presetzzz, "::eg1_dca_enabled=%d", m_settings.m_eg1_dca_enabled);
  settings_count++;

  fprintf(presetzzz, "::filter_keytrack_intensity=%f",
          m_settings.m_filter_keytrack_intensity);
  settings_count++;

  fprintf(presetzzz, "::octave=%d", m_settings.m_octave);
  settings_count++;
  fprintf(presetzzz, "::pitchbend_range=%d", m_settings.m_pitchbend_range);
  settings_count++;

  fprintf(presetzzz, "::legato_mode=%d", m_settings.m_legato_mode);
  settings_count++;
  fprintf(presetzzz, "::reset_to_zero=%d", m_settings.m_reset_to_zero);
  settings_count++;
  fprintf(presetzzz, "::filter_keytrack=%d", m_settings.m_filter_keytrack);
  settings_count++;
  fprintf(presetzzz, "::filter_type=%d", m_settings.m_filter_type);
  settings_count++;
  fprintf(presetzzz, "::filter_saturation=%f", m_settings.m_filter_saturation);
  settings_count++;

  fprintf(presetzzz, "::nlp=%d", m_settings.m_nlp);
  settings_count++;
  fprintf(presetzzz, "::velocity_to_attack_scaling=%d",
          m_settings.m_velocity_to_attack_scaling);
  settings_count++;
  fprintf(presetzzz, "::note_number_to_decay_scaling=%d",
          m_settings.m_note_number_to_decay_scaling);
  settings_count++;
  fprintf(presetzzz, "::portamento_time_msec=%f",
          m_settings.m_portamento_time_msec);
  settings_count++;

  fprintf(presetzzz, "::sustain_override=%d",
          m_settings.m_eg1_sustain_override);
  settings_count++;

  fprintf(presetzzz, ":::\n");
  fclose(presetzzz);
  printf("Wrote %d settings\n", settings_count++);
  return;
}

void MiniSynth::Load(std::string preset_name) {
  if (preset_name.empty()) {
    printf(
        "Play tha game, pal, need a name to LOAD yer synth settings "
        "with\n");
    return;
  }
  const char *preset_to_load = preset_name.c_str();

  char line[2048];
  char setting_key[512];
  char setting_val[512];
  double scratch_val = 0.;

  FILE *presetzzz = fopen(MOOG_PRESET_FILENAME, "r+");
  if (presetzzz == NULL) return;

  char *tok, *last_tok;
  char const *sep = "::";

  while (fgets(line, sizeof(line), presetzzz)) {
    int settings_count = 0;

    for (tok = strtok_r(line, sep, &last_tok); tok;
         tok = strtok_r(NULL, sep, &last_tok)) {
      sscanf(tok, "%[^=]=%s", setting_key, setting_val);
      sscanf(setting_val, "%lf", &scratch_val);
      if (strcmp(setting_key, "name") == 0) {
        if (strcmp(setting_val, preset_to_load) != 0) break;
        strcpy(m_settings.m_settings_name, setting_val);
        settings_count++;
      } else if (strcmp(setting_key, "voice_mode") == 0) {
        m_settings.m_voice_mode = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "monophonic") == 0) {
        m_settings.m_monophonic = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_waveform") == 0) {
        m_settings.m_lfo1_waveform = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_dest") == 0) {
        m_settings.m_lfo1_dest = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_mode") == 0) {
        m_settings.m_lfo1_mode = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_rate") == 0) {
        m_settings.m_lfo1_rate = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_amp") == 0) {
        m_settings.m_lfo1_amplitude = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_osc_pitch_intensity") == 0) {
        m_settings.m_lfo1_osc_pitch_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_osc_pitch_enabled") == 0) {
        m_settings.m_lfo1_osc_pitch_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_filter_fc_intensity") == 0) {
        m_settings.m_lfo1_filter_fc_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_filter_fc_enabled") == 0) {
        m_settings.m_lfo1_filter_fc_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_amp_intensity") == 0) {
        m_settings.m_lfo1_amp_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_amp_enabled") == 0) {
        m_settings.m_lfo1_amp_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_pan_intensity") == 0) {
        m_settings.m_lfo1_pan_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_pan_enabled") == 0) {
        m_settings.m_lfo1_pan_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_pulsewidth_intensity") == 0) {
        m_settings.m_lfo1_pulsewidth_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo1_pulsewidth_enabled") == 0) {
        m_settings.m_lfo1_pulsewidth_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_waveform") == 0) {
        m_settings.m_lfo2_waveform = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_dest") == 0) {
        m_settings.m_lfo2_dest = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_mode") == 0) {
        m_settings.m_lfo2_mode = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_rate") == 0) {
        m_settings.m_lfo2_rate = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_amp") == 0) {
        m_settings.m_lfo2_amplitude = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_osc_pitch_intensity") == 0) {
        m_settings.m_lfo2_osc_pitch_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_osc_pitch_enabled") == 0) {
        m_settings.m_lfo2_osc_pitch_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_filter_fc_intensity") == 0) {
        m_settings.m_lfo2_filter_fc_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_filter_fc_enabled") == 0) {
        m_settings.m_lfo2_filter_fc_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_amp_intensity") == 0) {
        m_settings.m_lfo2_amp_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_amp_enabled") == 0) {
        m_settings.m_lfo2_amp_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_pan_intensity") == 0) {
        m_settings.m_lfo2_pan_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_pan_enabled") == 0) {
        m_settings.m_lfo2_pan_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_pulsewidth_intensity") == 0) {
        m_settings.m_lfo2_pulsewidth_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "lfo2_pulsewidth_enabled") == 0) {
        m_settings.m_lfo2_pulsewidth_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "attack_time_msec") == 0) {
        m_settings.m_eg1_attack_time_msec = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "decay_time_msec") == 0) {
        m_settings.m_eg1_decay_time_msec = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "release_time_msec") == 0) {
        m_settings.m_eg1_release_time_msec = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "sustain_level") == 0) {
        m_settings.m_eg1_sustain_level = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "volume_db") == 0) {
        volume = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "fc_control") == 0) {
        m_settings.m_fc_control = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "q_control") == 0) {
        m_settings.m_q_control = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "detune_cents") == 0) {
        m_settings.m_detune_cents = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "pulse_width_pct") == 0) {
        m_settings.m_pulse_width_pct = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "sub_osc_db") == 0) {
        m_settings.m_sub_osc_db = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "noise_osc_db") == 0) {
        m_settings.m_noise_osc_db = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "eg1_osc_intensity") == 0) {
        m_settings.m_eg1_osc_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "eg1_osc_enabled") == 0) {
        m_settings.m_eg1_osc_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "eg1_filter_intensity") == 0) {
        m_settings.m_eg1_filter_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "eg1_filter_enabled") == 0) {
        m_settings.m_eg1_filter_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "eg1_dca_intensity") == 0) {
        m_settings.m_eg1_dca_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "eg1_dca_enabled") == 0) {
        m_settings.m_eg1_dca_enabled = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "filter_keytrack_intensity") == 0) {
        m_settings.m_filter_keytrack_intensity = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "octave") == 0) {
        m_settings.m_octave = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "pitchbend_range") == 0) {
        m_settings.m_pitchbend_range = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "legato_mode") == 0) {
        m_settings.m_legato_mode = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "reset_to_zero") == 0) {
        m_settings.m_reset_to_zero = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "filter_keytrack") == 0) {
        m_settings.m_filter_keytrack = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "filter_type") == 0) {
        m_settings.m_filter_type = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "filter_saturation") == 0) {
        m_settings.m_filter_saturation = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "nlp") == 0) {
        m_settings.m_nlp = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "velocity_to_attack_scaling") == 0) {
        m_settings.m_velocity_to_attack_scaling = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "note_number_to_decay_scaling") == 0) {
        m_settings.m_note_number_to_decay_scaling = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "portamento_time_msec") == 0) {
        m_settings.m_portamento_time_msec = scratch_val;
        settings_count++;
      } else if (strcmp(setting_key, "sustain_override") == 0) {
        m_settings.m_eg1_sustain_override = scratch_val;
        settings_count++;
      }
    }
    // if (settings_count > 0)
    //    printf("Loaded %d settings\n", settings_count);
    Update();
  }

  fclose(presetzzz);
}

void MiniSynth::SetFilterMod(double mod) {
  for (auto v : voices_) v->SetFilterMod(mod);
}

void MiniSynth::SetEgAttackTimeMs(unsigned int eg_num, double val) {
  if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS) {
    if (eg_num == 1)
      m_settings.m_eg1_attack_time_msec = val;
    else if (eg_num == 2)
      m_settings.m_eg2_attack_time_msec = val;
  } else
    printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void MiniSynth::SetEgDecayTimeMs(unsigned int eg_num, double val) {
  if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS) {
    if (eg_num == 1)
      m_settings.m_eg1_decay_time_msec = val;
    else if (eg_num == 2)
      m_settings.m_eg2_decay_time_msec = val;
  } else
    printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void MiniSynth::SetEgReleaseTimeMs(unsigned int eg_num, double val) {
  if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS) {
    if (eg_num == 1)
      m_settings.m_eg1_release_time_msec = val;
    else if (eg_num == 2)
      m_settings.m_eg2_release_time_msec = val;
  } else
    printf("val must be between %d and %d\n", EG_MINTIME_MS, EG_MAXTIME_MS);
}

void MiniSynth::SetOscAmp(unsigned int osc_num, double val) {
  if (osc_num == 0 || osc_num > 4) return;

  if (val >= -1 && val <= 1) {
    switch (osc_num) {
      case (1):
        m_settings.osc1_amp = val;
        break;
      case (2):
        m_settings.osc2_amp = val;
        break;
      case (3):
        m_settings.osc3_amp = val;
        break;
      case (4):
        m_settings.osc4_amp = val;
        break;
    }
  } else
    printf("val must be between -1 and 1\n");
}

void MiniSynth::SetDetune(double val) {
  if (val >= -100 && val <= 100)
    m_settings.m_detune_cents = val;
  else
    printf("val must be between -100 and 100\n");
}

void MiniSynth::SetEgDcaEnable(unsigned int osc_num, int val) {
  if (val == 0 || val == 1) {
    if (osc_num == 1)
      m_settings.m_eg1_dca_enabled = val;
    else if (osc_num == 2)
      m_settings.m_eg2_dca_enabled = val;
  } else
    printf("val must be boolean 0 or 1\n");
}

void MiniSynth::SetEgDcaInt(unsigned int eg_num, double val) {
  if (val >= -1 && val <= 1) {
    if (eg_num == 1)
      m_settings.m_eg1_dca_intensity = val;
    else if (eg_num == 2)
      m_settings.m_eg2_dca_intensity = val;
  } else
    printf("val must be between -1 and 1\n");
}

void MiniSynth::SetEgFilterEnable(unsigned int eg_num, int val) {
  if (val == 0 || val == 1) {
    if (eg_num == 1)
      m_settings.m_eg1_filter_enabled = val;
    else if (eg_num == 2)
      m_settings.m_eg2_filter_enabled = val;
  } else
    printf("val must be boolean 0 or 1\n");
}

void MiniSynth::SetEgFilterInt(unsigned int eg_num, double val) {
  if (val >= -1 && val <= 1) {
    if (eg_num == 1)
      m_settings.m_eg1_filter_intensity = val;
    else if (eg_num == 2)
      m_settings.m_eg2_filter_intensity = val;
  } else
    printf("val must be between -1 and 1\n");
}

void MiniSynth::SetEgOscEnable(unsigned int eg_num, int val) {
  if (val == 0 || val == 1) {
    if (eg_num == 1)
      m_settings.m_eg1_osc_enabled = val;
    else if (eg_num == 2)
      m_settings.m_eg2_osc_enabled = val;
  } else
    printf("val must be boolean 0 or 1\n");
}

void MiniSynth::SetEgOscInt(unsigned int eg_num, double val) {
  if (val >= -1 && val <= 1) {
    if (eg_num == 1)
      m_settings.m_eg1_osc_intensity = val;
    else if (eg_num == 2)
      m_settings.m_eg2_osc_intensity = val;
  } else
    printf("val must be between -1 and 1\n");
}

void MiniSynth::SetFilterFc(double val) {
  if (val >= 80 && val <= 18000)
    m_settings.m_fc_control = val;
  else
    printf("val must be between 80 and 18000\n");
}

void MiniSynth::SetFilterFq(double val) {
  if (val >= 0.5 && val <= 10)
    m_settings.m_q_control = val;
  else
    printf("val must be between 0.5 and 10\n");
}

void MiniSynth::SetFilterType(unsigned int val) {
  if (val == BSF2 || val == LPF1 || val == HPF1)
    printf("warning! useless change - %d not possible with moog\n", val);
  if (val < NUM_FILTER_TYPES)
    m_settings.m_filter_type = val;
  else
    printf("Val must be between 0 and %d\n", NUM_FILTER_TYPES - 1);
}

void MiniSynth::SetFilterSaturation(double val) {
  if (val >= 0 && val <= 100)
    m_settings.m_filter_saturation = val;
  else
    printf("Val must be between 0 and 100\n");
}

void MiniSynth::SetFilterNlp(unsigned int val) {
  if (val < 2)
    m_settings.m_nlp = val;
  else
    printf("Val must be 0 or 1\n");
}

void MiniSynth::SetKeytrackInt(double val) {
  if (val >= 0.5 && val <= 10)
    m_settings.m_filter_keytrack_intensity = val;
  else
    printf("val must be between 0.5 and 10\n");
}

void MiniSynth::SetKeytrack(unsigned int val) {
  if (val != 0 && val != 1) {
    printf("Val must be zero or one\n");
    return;
  }
  m_settings.m_filter_keytrack = val;
}

void MiniSynth::SetLegatoMode(unsigned int val) {
  if (val != 0 && val != 1) {
    printf("Val must be zero or one\n");
    return;
  }
  m_settings.m_legato_mode = val;
}

void MiniSynth::SetLFOOscEnable(int lfo_num, int val) {
  if (val == 0 || val == 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_osc_pitch_enabled = val;
        break;
      case (2):
        m_settings.m_lfo2_osc_pitch_enabled = val;
        break;
    }
  } else
    printf("Must be a boolean 0 or 1\n");
}

void MiniSynth::SetLFOAmpEnable(int lfo_num, int val) {
  if (val == 0 || val == 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_amp_enabled = val;
        break;
      case (2):
        m_settings.m_lfo2_amp_enabled = val;
        break;
    }
  } else
    printf("Must be a boolean 0 or 1\n");
}

void MiniSynth::SetLFOFilterEnable(int lfo_num, int val) {
  if (val == 0 || val == 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_filter_fc_enabled = val;
        break;
      case (2):
        m_settings.m_lfo2_filter_fc_enabled = val;
        break;
    }
  } else
    printf("Must be a boolean 0 or 1\n");
}

void MiniSynth::SetLFOPanEnable(int lfo_num, int val) {
  if (val == 0 || val == 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_pan_enabled = val;
        break;
      case (2):
        m_settings.m_lfo2_pan_enabled = val;
        break;
    }
  } else
    printf("Must be a boolean 0 or 1\n");
}

void MiniSynth::SetLFOPulsewidthEnable(int lfo_num, unsigned int val) {
  if (val == 0 || val == 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_pulsewidth_enabled = val;
        break;
      case (2):
        m_settings.m_lfo2_pulsewidth_enabled = val;
        break;
    }
  } else
    printf("Must be a boolean 0 or 1\n");
}

void MiniSynth::SetLFOAmpInt(int lfo_num, double val) {
  if (val >= 0 && val <= 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_amp_intensity = val;
        break;
      case (2):
        m_settings.m_lfo2_amp_intensity = val;
        break;
    }
  } else
    printf("val must be between 0 and 1\n");
}

void MiniSynth::SetLFOAmp(int lfo_num, double val) {
  if (val >= 0 && val <= 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_amplitude = val;
        break;
      case (2):
        m_settings.m_lfo2_amplitude = val;
        break;
    }
  } else
    printf("val must be between 0 and 1\n");
}

void MiniSynth::SetLFOFilterFcInt(int lfo_num, double val) {
  if (val >= -1 && val <= 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_filter_fc_intensity = val;
        break;
      case (2):
        m_settings.m_lfo2_filter_fc_intensity = val;
        break;
    }
  } else
    printf("val must be between -1 and 1\n");
}

void MiniSynth::SetLFOPulsewidthInt(int lfo_num, double val) {
  if (val >= -1 && val <= 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_pulsewidth_intensity = val;
        break;
      case (2):
        m_settings.m_lfo2_pulsewidth_intensity = val;
        break;
    }
  } else
    printf("val must be between -1 and 1\n");
}

void MiniSynth::SetLFORate(int lfo_num, double val) {
  if (val >= 0.02 && val <= 20) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_rate = val;
        break;
      case (2):
        m_settings.m_lfo2_rate = val;
        break;
    }
  } else
    printf("val must be between 0.02 and 20\n");
}

void MiniSynth::SetLFOPanInt(int lfo_num, double val) {
  if (val >= 0 && val <= 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_pan_intensity = val;
        break;
      case (2):
        m_settings.m_lfo2_pan_intensity = val;
        break;
    }
  } else
    printf("val must be between 0 and 1\n");
}

void MiniSynth::SetLFOOscInt(int lfo_num, double val) {
  if (val >= -1 && val <= 1) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_osc_pitch_intensity = val;
        break;
      case (2):
        m_settings.m_lfo2_osc_pitch_intensity = val;
        break;
    }
  } else
    printf("val must be between -1 and 1\n");
}

void MiniSynth::SetLFOWave(int lfo_num, unsigned int val) {
  if (val < MAX_LFO_OSC) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_waveform = val;
        break;
      case (2):
        m_settings.m_lfo2_waveform = val;
        break;
    }
  } else
    printf("val must be between 0 and %d\n", MAX_LFO_OSC);
}

void MiniSynth::SetLFOMode(int lfo_num, unsigned int val) {
  if (val < LFO_MAX_MODE) {
    switch (lfo_num) {
      case (1):
        m_settings.m_lfo1_mode = val;
        break;
      case (2):
        m_settings.m_lfo2_mode = val;
        break;
    }
  } else
    printf("val must be between 0 and %d\n", LFO_MAX_MODE - 1);
}

void MiniSynth::SetNoteToDecayScaling(unsigned int val) {
  if (val != 0 && val != 1) {
    printf("Val must be zero or one\n");
    return;
  }
  m_settings.m_note_number_to_decay_scaling = val;
}

void MiniSynth::SetNoiseOscDb(double val) {
  if (val >= -96 && val <= 0)
    m_settings.m_noise_osc_db = val;
  else
    printf("val must be between -96 and 0\n");
}

void MiniSynth::SetOctave(int val) {
  if (val >= -4 && val <= 4)
    m_settings.m_octave = val;
  else
    printf("val must be between -4 and 4\n");
}

void MiniSynth::SetPitchbendRange(int val) {
  if (val >= 0 && val <= 12)
    m_settings.m_pitchbend_range = val;
  else
    printf("val must be between 0 and 12\n");
}

void MiniSynth::SetPortamentoTimeMs(double val) {
  if (val >= 0 && val <= 5000)
    m_settings.m_portamento_time_msec = val;
  else
    printf("val must be between 0 and 5000\n");
}

void MiniSynth::SetPulsewidthPct(double val) {
  if (val >= 1 && val <= 99)
    m_settings.m_pulse_width_pct = val;
  else
    printf("val must be between 1 and 99\n");
}

void MiniSynth::SetSubOscDb(double val) {
  if (val >= -96 && val <= 0)
    m_settings.m_sub_osc_db = val;
  else
    printf("val must be between -96 and 0\n");
}

void MiniSynth::SetEgSustain(unsigned int eg_num, double val) {
  if (val >= 0 && val <= 1) {
    if (eg_num == 1)
      m_settings.m_eg1_sustain_level = val;
    else if (eg_num == 2)
      m_settings.m_eg2_sustain_level = val;
  } else
    printf("val must be between 0 and 1\n");
}

void MiniSynth::SetEgSustainOverride(unsigned int eg_num, bool b) {
  if (eg_num == 1)
    m_settings.m_eg1_sustain_override = b;
  else if (eg_num == 2)
    m_settings.m_eg2_sustain_override = b;
}

void MiniSynth::SetVelocityToAttackScaling(unsigned int val) {
  if (val != 0 && val != 1) {
    printf("Val must be zero or one\n");
    return;
  }
  m_settings.m_velocity_to_attack_scaling = val;
}

void MiniSynth::SetVoiceMode(unsigned int val) {
  if (val < MAX_VOICE_CHOICE)
    m_settings.m_voice_mode = val;
  else
    printf("val must be between 0 and %d\n", MAX_VOICE_CHOICE);
}

void MiniSynth::SetResetToZero(unsigned int val) {
  if (val != 0 && val != 1) {
    printf("Val must be zero or one\n");
    return;
  }
  m_settings.m_reset_to_zero = val;
}

void MiniSynth::SetMonophonic(bool b) { m_settings.m_monophonic = b; }
void MiniSynth::SetGenerate(bool b) { m_settings.m_generate_active = b; }

// void MiniSynth::SetOscType(int osc, unsigned int osc_type)
//{
//    if (osc > 0 && osc < 4 && osc_type < MAX_OSC)
//    {
//        printf("Setting OSC %d to %s(%d)\n", osc, s_waveform_names[osc_type],
//               osc_type);
//        switch (osc)
//        {
//        case (1):
//            m_settings.osc1_wave = osc_type;
//            break;
//        case (2):
//            m_settings.osc2_wave = osc_type;
//            break;
//        case (3):
//            m_settings.osc3_wave = osc_type;
//            break;
//        case (4):
//            m_settings.osc4_wave = osc_type;
//            break;
//        }
//    }
//}

void MiniSynth::SetOscSemitones(unsigned int osc, int semitones) {
  if (osc > 0 && osc < 4 && semitones > -100 && semitones < 100) {
    switch (osc) {
      case (1):
        m_settings.osc1_semis = semitones;
        break;
      case (2):
        m_settings.osc2_semis = semitones;
        break;
      case (3):
        m_settings.osc3_semis = semitones;
        break;
      case (4):
        m_settings.osc4_semis = semitones;
        break;
    }
  }
}

void MiniSynth::SetHardSync(bool val) {
  // TODO - add to export / load functions
  m_settings.hard_sync = val;
}

void MiniSynth::SetParam(std::string name, double val) {
  if (name == "vol")
    SetVolume(val);
  else if (name == "pan")
    SetPan(val);
  else if (name == "voice")
    SetVoiceMode(val);
  else if (name == "mono")
    SetMonophonic(val);
  else if (name == "hard_sync")
    SetHardSync(val);
  else if (name == "detune")
    SetDetune(val);
  else if (name == "legato")
    SetLegatoMode(val);
  else if (name == "kt")
    SetKeytrack(val);
  else if (name == "ndscale")
    SetNoteToDecayScaling(val);

  else if (name == "osc1")
    std::cout << "Change voice to change osc types.\n";
  else if (name == "o1amp")
    SetOscAmp(1, val);
  else if (name == "o1oct")
    SetOctave(val);
  else if (name == "o1semi")
    SetOscSemitones(1, val);
  else if (name == "o1cents")
    std::cout << "Use detune to adjust cents\n";

  else if (name == "osc2")
    std::cout << "Change voice to change osc types.\n";
  else if (name == "o2amp")
    SetOscAmp(2, val);
  else if (name == "o2oct")
    SetOctave(val);
  else if (name == "o2semi")
    SetOscSemitones(2, val);
  else if (name == "o2cents")
    std::cout << "Use detune to adjust cents\n";

  else if (name == "osc3")
    std::cout << "Change voice to change osc types.\n";
  else if (name == "o3amp")
    std::cout << "Use 'subosc' param to change osc3" << std::endl;
  else if (name == "o3oct")
    SetOctave(val);
  else if (name == "o3semi")
    SetOscSemitones(3, val);

  else if (name == "osc4")
    std::cout << "Can't change noise ooooooosc\n";
  else if (name == "o4amp")
    std::cout << "Use 'noisedb' param to change osc4" << std::endl;
  else if (name == "o4oct")
    SetOctave(val);
  else if (name == "o4semi")
    SetOscSemitones(4, val);

  else if (name == "noisedb")
    SetNoiseOscDb(val);
  else if (name == "octave")
    SetOctave(val);
  else if (name == "pitchrange")
    SetPitchbendRange(val);
  else if (name == "porta")
    SetPortamentoTimeMs(val);
  else if (name == "pw")
    SetPulsewidthPct(val);

  else if (name == "subosc")
    SetSubOscDb(val);
  else if (name == "vascale")
    SetVelocityToAttackScaling(val);
  else if (name == "zero")
    SetResetToZero(val);

  else if (name == "l1wave")
    SetLFOWave(1, val);
  else if (name == "l1mode")
    SetLFOMode(1, val);
  else if (name == "l1rate")
    SetLFORate(1, val);
  else if (name == "l1amp")
    SetLFOAmp(1, val);
  else if (name == "l1_filter_en")
    SetLFOFilterEnable(1, val);
  else if (name == "l1_osc_en")
    SetLFOOscEnable(1, val);
  else if (name == "l1_pan_en")
    SetLFOPanEnable(1, val);
  else if (name == "l1_amp_en")
    SetLFOAmpEnable(1, val);
  else if (name == "l1_pw_en")
    SetLFOPulsewidthEnable(1, val);
  else if (name == "l1_filter_int")
    SetLFOFilterFcInt(1, val);
  else if (name == "l1_osc_int")
    SetLFOOscInt(1, val);
  else if (name == "l1_pan_int")
    SetLFOPanInt(1, val);
  else if (name == "l1_amp_int")
    SetLFOAmpInt(1, val);
  else if (name == "l1_pw_int")
    SetLFOPulsewidthInt(1, val);

  else if (name == "l2wave")
    SetLFOWave(2, val);
  else if (name == "l2mode")
    SetLFOMode(2, val);
  else if (name == "l2rate")
    SetLFORate(2, val);
  else if (name == "l2amp")
    SetLFOAmp(2, val);
  else if (name == "l2_filter_en")
    SetLFOFilterEnable(2, val);
  else if (name == "l2_osc_en")
    SetLFOOscEnable(2, val);
  else if (name == "l2_pan_en")
    SetLFOPanEnable(2, val);
  else if (name == "l2_amp_en")
    SetLFOAmpEnable(2, val);
  else if (name == "l2_pw_en")
    SetLFOPulsewidthEnable(2, val);
  else if (name == "l2_filter_int")
    SetLFOFilterFcInt(2, val);
  else if (name == "l2_osc_int")
    SetLFOOscInt(2, val);
  else if (name == "l2_pan_int")
    SetLFOPanInt(2, val);
  else if (name == "l2_amp_int")
    SetLFOAmpInt(2, val);
  else if (name == "l2_pw_int")
    SetLFOPulsewidthInt(2, val);

  else if (name == "eg1_filter_en")
    SetEgFilterEnable(1, val);
  else if (name == "eg1_osc_en")
    SetEgOscEnable(1, val);
  else if (name == "eg1_dca_en")
    SetEgDcaEnable(1, val);
  else if (name == "eg1_sustain")
    SetEgSustainOverride(1, val);
  else if (name == "eg1_filter_int")
    SetEgFilterInt(1, val);
  else if (name == "eg1_osc_int")
    SetEgOscInt(1, val);
  else if (name == "eg1_dca_int")
    SetEgDcaInt(1, val);
  else if (name == "eg1_sus")
    SetEgSustain(1, val);
  else if (name == "eg1_attack")
    SetEgAttackTimeMs(1, val);
  else if (name == "eg1_decay")
    SetEgDecayTimeMs(1, val);
  else if (name == "eg1_release")
    SetEgReleaseTimeMs(1, val);
  else if (name == "eg1_sustain_override")
    SetEgSustainOverride(1, val);

  else if (name == "eg2_filter_en")
    SetEgFilterEnable(2, val);
  else if (name == "eg2_osc_en")
    SetEgOscEnable(2, val);
  else if (name == "eg2_dca_en")
    SetEgDcaEnable(2, val);
  else if (name == "eg2_sustain")
    SetEgSustainOverride(2, val);
  else if (name == "eg2_filter_int")
    SetEgFilterInt(2, val);
  else if (name == "eg2_osc_int")
    SetEgOscInt(2, val);
  else if (name == "eg2_dca_int")
    SetEgDcaInt(2, val);
  else if (name == "eg2_sus")
    SetEgSustain(2, val);
  else if (name == "eg2_attack")
    SetEgAttackTimeMs(2, val);
  else if (name == "eg2_decay")
    SetEgDecayTimeMs(2, val);
  else if (name == "eg2_release")
    SetEgReleaseTimeMs(2, val);
  else if (name == "eg2_sustain_override")
    SetEgSustainOverride(2, val);

  else if (name == "filter")
    SetFilterType(val);
  else if (name == "fc")
    SetFilterFc(val);
  else if (name == "fq")
    SetFilterFq(val);
  else if (name == "sat")
    SetFilterSaturation(val);
  else if (name == "kt_int")
    SetKeytrackInt(val);
  else if (name == "nlp")
    SetFilterNlp(val);

  Update();
}

}  // namespace SBAudio
