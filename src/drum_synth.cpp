#include <drum_synth.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "midi_freq_table.h"
#include "utils.h"

namespace {
std::array<std::string, 8> osc_types{"SINE", "SAW1",   "SAW2",  "SAW3",
                                     "TRI",  "SQUARE", "NOISE", "PSNOISE"};
std::string GetOscType(int type) {
  std::string the_type = "dunno";
  if (type < osc_types.size()) return osc_types[type];
  return the_type;
}
bool BoolGen() { return std::rand() % 2; }
}  // namespace

namespace SBAudio {

DrumSynth::DrumSynth() {
  // TRANSIENT
  noise_ = std::make_unique<QBLimitedOscillator>();
  noise_->m_waveform = NOISE;

  noise_eg_.SetRampMode(true);
  noise_eg_.m_reset_to_zero = true;

  noise_filter_ = std::make_unique<CKThreeFive>();

  // PITCH
  osc1_ = std::make_unique<QBLimitedOscillator>();

  osc_eg_.SetRampMode(true);
  osc_eg_.m_reset_to_zero = true;

  // OUTPUT
  amp_eg_.SetRampMode(true);
  amp_eg_.m_reset_to_zero = true;

  amp_filter_ = std::make_unique<CKThreeFive>();

  // default
  LoadSettings(DrumSettings());
  active = true;
}

StereoVal DrumSynth::GenNext(mixer_timing_info tinfo) {
  StereoVal out = {.left = 0, .right = 0};
  if (!active) return out;
  ms_per_midi_tick_ = tinfo.ms_per_midi_tick;

  if (osc1_->m_note_on) {
    // Transient
    noise_->Update();
    double noise_eg_out = noise_eg_.DoEnvelope(nullptr);
    double noise_out = noise_->DoOscillate(nullptr) * noise_eg_out;
    noise_out = noise_filter_->DoFilter(noise_out);

    // OSCILLATORS

    double biased_eg_out = 0;
    osc_eg_.DoEnvelope(&biased_eg_out);

    double eg_osc_mod = OSC_FO_MOD_RANGE * biased_eg_out;
    double osc1_mod_val = eg_osc_mod;

    osc1_->SetFoModExp(osc1_mod_val);
    osc1_->Update();

    double osc1_out = osc1_->DoOscillate(nullptr);
    // double osc_mix = 0.;
    // if (settings_.hard_sync) {
    //   osc1_->DoOscillate(nullptr);
    //   if (osc1_->just_wrapped) osc2_->StartOscillator();
    //   double osc2_out = osc2_->DoOscillate(nullptr) * settings_.osc2_amp;
    //   if (settings_.filter2_enable) {
    //     osc2_out = filter2_->DoFilter(osc2_out);
    //   }
    //   osc_mix = 0.666 * osc2_out + 0.333 * osc3_out;
    // } else {
    //   double osc1_out = osc1_->DoOscillate(nullptr) * settings_.osc1_amp;
    //   if (settings_.filter1_enable) {
    //     osc1_out = filter1_->DoFilter(osc1_out);
    //   }
    //   double osc2_out = osc2_->DoOscillate(nullptr) * settings_.osc2_amp;
    //   if (settings_.filter2_enable) {
    //     osc2_out = filter2_->DoFilter(osc2_out);
    //   }
    //   osc_mix = 0.333 * osc1_out + 0.333 * osc2_out + 0.333 * osc3_out;
    // }

    double osc_mix = osc1_out + noise_out;
    // double osc_mix = osc1_out;

    //// OUTPUT //////////////////////////

    // FILTER ////////////////////
    double osc_out = amp_filter_->DoFilter(osc_mix);

    double out_left = 0.0;
    double out_right = 0.0;

    // lfo_.Update();
    // double lfo_out = lfo_.DoOscillate(0);

    // OUT MIXDOWN through AMP ENV
    double amp_eg_out = amp_eg_.DoEnvelope(nullptr);
    double dca_mod_val = amp_eg_out;
    dca_.SetEgMod(dca_mod_val);
    dca_.Update();
    dca_.DoDCA(osc_out, osc_out, &out_left, &out_right);

    // final OUT and FX
    out = {.left = out_left * volume * velocity_,
           .right = out_right * volume * velocity_};
    out = distortion_.Process(out);
    out = Effector(out);
  }

  if (amp_eg_.GetState() == OFFF) {
    osc1_->StopOscillator();
    noise_->StopOscillator();

    amp_eg_.StopEg();
    osc_eg_.StopEg();
    noise_eg_.StopEg();
  }

  return out;
}

void DrumSynth::SetParam(std::string name, double val) {
  if (name == "volume")
    settings_.volume = val;
  else if (name == "pan")
    settings_.pan = val;
  else if (name == "distort")
    settings_.distortion_threshold = val;
  else if (name == "hard_sync")
    settings_.hard_sync = val;
  else if (name == "detune")
    settings_.detune_cents = val;
  else if (name == "pulse_width")
    settings_.pulse_width_pct = val;

  else if (name == "noise_amp")
    settings_.noise_amp = val;
  else if (name == "noise_eg_attack")
    settings_.noise_eg_attack_ms = val;
  else if (name == "noise_eg_decay")
    settings_.noise_eg_decay_ms = val;
  else if (name == "noise_eg_mode")
    settings_.noise_eg_mode = val;
  else if (name == "noise_filter_type")
    settings_.noise_filter_type = val;
  else if (name == "noise_filter_fc")
    settings_.noise_filter_fc = val;
  else if (name == "noise_filter_q")
    settings_.noise_filter_q = val;

  //// PITCH
  else if (name == "osc1")
    settings_.osc1_wav = val;
  else if (name == "osc1_amp")
    settings_.osc1_amp = val;
  else if (name == "osc1_ratio")
    settings_.osc1_ratio = val;
  else if (name == "osc_eg_attack")
    settings_.osc_eg_attack_ms = val;
  else if (name == "osc_eg_decay")
    settings_.osc_eg_decay_ms = val;

  else if (name == "amp_eg_attack")
    settings_.amp_eg_attack_ms = val;
  else if (name == "amp_eg_decay")
    settings_.amp_eg_decay_ms = val;
  else if (name == "amp_eg_mode")
    settings_.amp_eg_mode = val;
  else if (name == "amp_filter_type")
    settings_.amp_filter_type = val;
  else if (name == "amp_filter_fc")
    settings_.amp_filter_fc = val;
  else if (name == "amp_filter_q")
    settings_.amp_filter_q = val;

  Update();
}

std::string DrumSynth::Info() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "DrumZynth - " << COOL_COLOR_PINK2 << settings_.name
     << COOL_COLOR_YELLOW_MELLOW << " - vol:" << volume << " pan:" << pan
     << std::endl;
  ss << "     distort:" << settings_.distortion_threshold
     << " hard_sync:" << settings_.hard_sync
     << " detune:" << settings_.detune_cents
     << " pulse_width:" << settings_.pulse_width_pct << std::endl;
  ss << COOL_COLOR_PINK2;
  ss << "     noise_amp:" << settings_.noise_amp
     << " noise_eg_attack:" << settings_.noise_eg_attack_ms
     << " noise_eg_decay:" << settings_.noise_eg_decay_ms
     << " noise_eg_mode:" << settings_.noise_eg_mode << std::endl;
  ss << "     noise_filter_type:"
     << k_filter_type_names[settings_.noise_filter_type] << "("
     << settings_.noise_filter_type << ")"
     << " noise_filter_fc:" << settings_.noise_filter_fc
     << " noise_filter_q:" << settings_.noise_filter_q << std::endl;
  ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "     osc1:" << GetOscType(settings_.osc1_wav) << "("
     << settings_.osc1_wav << ")"
     << " osc1_amp:" << settings_.osc1_amp
     << " osc1_ratio:" << settings_.osc1_ratio
     << " osc_eg_attack:" << settings_.osc_eg_attack_ms
     << " osc_eg_decay:" << settings_.osc_eg_decay_ms << std::endl;
  ss << COOL_COLOR_PINK2;
  ss << "     amp_eg_attack:" << settings_.amp_eg_attack_ms
     << " amp_eg_decay:" << settings_.amp_eg_decay_ms
     << " amp_eg_mode:" << settings_.amp_eg_mode << std::endl;
  ss << "     amp_filter_type:"
     << k_filter_type_names[settings_.amp_filter_type] << "("
     << settings_.amp_filter_type << ")"
     << " amp_filter_fc:" << settings_.amp_filter_fc
     << " amp_filter_q:" << settings_.amp_filter_q << std::endl;

  return ss.str();
}

std::string DrumSynth::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << COOL_COLOR_YELLOW_MELLOW;
  ss << "DrumZynth - "
     << COOL_COLOR_PINK2
     // ss << "DrumZynth - " << COOL_COLOR_PINK2 << settings_.name
     << COOL_COLOR_YELLOW_MELLOW << " - vol:" << volume << " pan:" << pan;

  return ss.str();
}

void DrumSynth::Start() {
  if (active) return;  // no-op
  active = true;
}

void DrumSynth::Stop() {
  if (active) return;  // no-op
  active = false;
}

void DrumSynth::NoteOff(midi_event ev) {
  osc_eg_.NoteOff();
  amp_eg_.NoteOff();
  noise_eg_.NoteOff();
}

void DrumSynth::NoteOn(midi_event ev) {
  unsigned int midinote = ev.data1;
  unsigned int velocity = ev.data2;

  velocity_ = scaleybum(0, 127, 0, 1, velocity);
  osc1_->m_osc_fo = Midi2Freq(midinote);

  osc1_->StartOscillator();
  noise_->StartOscillator();

  osc_eg_.StartEg();
  amp_eg_.StartEg();
  noise_eg_.StartEg();
}

void DrumSynth::Save(std::string new_preset_name) {
  std::cout << "DRUMSYNTH -- saving new preset '" << new_preset_name << "'"
            << std::endl;
  if (new_preset_name.empty()) {
    std::cerr << "Play tha game, pal, need a name to save yer synth settings "
              << std::endl;
    return;
  }
  settings_.name = new_preset_name;

  std::ofstream presetzzz;
  const std::string kSEP = "::";
  presetzzz.open(DRUM_PRESET_FILENAME, std::ios::app);
  presetzzz << "name=" << settings_.name << kSEP;
  presetzzz << "volume=" << settings_.volume << kSEP;
  presetzzz << "pan=" << settings_.pan << kSEP;
  presetzzz << "distortion_threshold=" << settings_.distortion_threshold
            << kSEP;
  presetzzz << "hard_sync=" << settings_.hard_sync << kSEP;
  presetzzz << "detune_cents=" << settings_.detune_cents << kSEP;
  presetzzz << "pulse_width_pct=" << settings_.pulse_width_pct << kSEP;

  presetzzz << "noise_amp=" << settings_.noise_amp << kSEP;
  presetzzz << "noise_eg_attack_ms=" << settings_.noise_eg_attack_ms << kSEP;
  presetzzz << "noise_eg_decay_ms=" << settings_.noise_eg_decay_ms << kSEP;
  presetzzz << "noise_eg_mode=" << settings_.noise_eg_mode << kSEP;
  presetzzz << "noise_filter_type=" << settings_.noise_filter_type << kSEP;
  presetzzz << "noise_filter_fc=" << settings_.noise_filter_fc << kSEP;
  presetzzz << "noise_filter_q=" << settings_.noise_filter_q << kSEP;

  // OSCILLATORS
  presetzzz << "osc1_wav=" << settings_.osc1_wav << kSEP;
  presetzzz << "osc1_amp=" << settings_.osc1_amp << kSEP;
  presetzzz << "osc1_ratio=" << settings_.osc1_ratio << kSEP;
  presetzzz << "osc_eg_attack_ms=" << settings_.osc_eg_attack_ms << kSEP;
  presetzzz << "osc_eg_decay_ms=" << settings_.osc_eg_decay_ms << kSEP;
  presetzzz << "osc_eg_mode=" << settings_.osc_eg_mode << kSEP;

  // OUTPUT
  presetzzz << "amp_eg_attack_ms=" << settings_.amp_eg_attack_ms << kSEP;
  presetzzz << "amp_eg_decay_ms=" << settings_.amp_eg_decay_ms << kSEP;
  presetzzz << "amp_eg_mode=" << settings_.amp_eg_mode << kSEP;
  presetzzz << "amp_filter_type=" << settings_.amp_filter_type << kSEP;
  presetzzz << "amp_filter_fc=" << settings_.amp_filter_fc << kSEP;
  presetzzz << "amp_filter_q=" << settings_.amp_filter_q << kSEP;

  presetzzz << std::endl;
  presetzzz.close();

  std::cout << "DRUMSYNTH -- saving -- DONE" << std::endl;
}

void DrumSynth::Update() {
  // GLOBALS
  volume = settings_.volume;
  pan = settings_.pan;
  distortion_.SetParam("threshold", settings_.distortion_threshold);

  //// TRANSIENT
  noise_->m_amplitude = settings_.noise_amp;
  noise_->Update();
  noise_filter_->SetType(settings_.noise_filter_type);
  noise_filter_->SetFcControl(settings_.noise_filter_fc);
  noise_filter_->SetQControlGUI(settings_.noise_filter_q);
  noise_filter_->Update();
  noise_eg_.SetEgMode(settings_.noise_eg_mode);
  noise_eg_.SetAttackTimeMsec(settings_.noise_eg_attack_ms);
  noise_eg_.SetDecayTimeMsec(settings_.noise_eg_decay_ms);
  noise_eg_.Update();

  // PITCH
  osc1_->m_waveform = settings_.osc1_wav;
  osc1_->m_amplitude = settings_.osc1_amp;
  osc1_->m_fo_ratio = settings_.osc1_ratio;
  osc1_->m_pulse_width_control = settings_.pulse_width_pct;
  osc1_->Update();
  osc_eg_.SetEgMode(settings_.osc_eg_mode);
  osc_eg_.SetAttackTimeMsec(settings_.osc_eg_attack_ms);
  osc_eg_.SetDecayTimeMsec(settings_.osc_eg_decay_ms);
  osc_eg_.Update();

  // OUTPUT
  amp_filter_->SetType(settings_.amp_filter_type);
  amp_filter_->SetFcControl(settings_.amp_filter_fc);
  amp_filter_->SetQControlGUI(settings_.amp_filter_q);
  amp_filter_->Update();
  amp_eg_.SetEgMode(settings_.amp_eg_mode);
  amp_eg_.SetAttackTimeMsec(settings_.amp_eg_attack_ms);
  amp_eg_.SetDecayTimeMsec(settings_.amp_eg_decay_ms);
  amp_eg_.Update();

  // lfo_.m_waveform = settings_.lfo_wave;
  // lfo_.m_lfo_mode = settings_.lfo_mode;
  // lfo_.m_fo = settings_.lfo_rate;
  // lfo_.Update();
}

void DrumSynth::LoadSettings(DrumSettings settings) {
  settings_ = settings;
  Update();
}

void DrumSynth::PrintSettings(DrumSettings settingz) {
  std::cout << "AAIIGht, settings for" << settingz.name << std::endl;

  // std::cout << "lfo_filter2_q_enable:" << settings_.lfo_filter2_q_enable
  //           << std::endl;
}

void DrumSynth::Randomize() {
  DrumSettings rand_settings;
  rand_settings.name = "RANDWOOF";

  // rand_settings.distortion_threshold = 0.7;
  // rand_settings.amplitude = 1;

  // rand_settings.pitch_bend = BoolGen();
  // rand_settings.pitch_range = rand() % 30 + 10;
  // rand_settings.q_range = rand() % 7 + 1;

  // rand_settings.osc1_wav = rand() % MAX_OSC;
  // rand_settings.osc1_amp = (float)rand() / RAND_MAX;
  // rand_settings.filter1_enable = BoolGen();
  // rand_settings.filter1_type = rand() % NUM_FILTER_TYPES;
  // rand_settings.filter1_fc = rand() % 10000 + 2000;
  // rand_settings.filter1_q = rand() % 8 + 1;
  // rand_settings.osc1_ratio = rand() % 4 + 1;

  // rand_settings.osc2_wav = rand() % MAX_OSC;
  // rand_settings.osc2_amp = (float)rand() / RAND_MAX;
  // rand_settings.filter2_enable = BoolGen();
  // rand_settings.filter2_type = rand() % NUM_FILTER_TYPES;
  // rand_settings.filter2_fc = rand() % 10000 + 2000;
  // rand_settings.filter2_q = rand() % 8 + 1;
  // rand_settings.osc2_ratio = rand() % 4 + 1;

  // rand_settings.osc3_wav = rand() % MAX_OSC;
  // rand_settings.osc3_amp = (float)rand() / RAND_MAX;
  // rand_settings.filter3_enable = BoolGen();
  // rand_settings.filter3_type = rand() % NUM_FILTER_TYPES;
  // rand_settings.filter3_fc = rand() % 10000 + 3000;
  // rand_settings.filter3_q = rand() % 8 + 1;
  // rand_settings.osc3_ratio = rand() % 4 + 1;

  //// rand_settings.master_filter_enable = BoolGen();
  //// rand_settings.master_filter_type = rand() % NUM_FILTER_TYPES;
  //// rand_settings.master_filter_fc = rand() % 10000 + 2000;
  //// rand_settings.master_filter_q = rand() % 8 + 1;

  // rand_settings.eg_attack_ms = rand() % 100;
  // rand_settings.eg_decay_ms = rand() % 100;

  // rand_settings.lfo_wave = rand() % MAX_OSC;
  // rand_settings.lfo_mode = rand() % LFO_MAX_MODE;
  // rand_settings.lfo_rate =
  //     ((float)rand()) / RAND_MAX * (MAX_LFO_RATE - MIN_LFO_RATE) +
  //     MIN_LFO_RATE;

  // rand_settings.eg_osc1_pitch_enable = BoolGen();
  // rand_settings.eg_osc2_pitch_enable = BoolGen();
  // rand_settings.eg_filter1_freq_enable = BoolGen();
  // rand_settings.eg_filter1_q_enable = BoolGen();
  // rand_settings.eg_filter2_freq_enable = BoolGen();
  // rand_settings.eg_filter2_q_enable = BoolGen();
  // rand_settings.eg_master_filter_freq_enable = BoolGen();
  // rand_settings.eg_master_filter_q_enable = BoolGen();

  // rand_settings.lfo_master_amp_enable = BoolGen();
  // rand_settings.lfo_osc1_pitch_enable = BoolGen();
  // rand_settings.lfo_osc2_pitch_enable = BoolGen();
  // rand_settings.lfo_filter1_freq_enable = BoolGen();
  // rand_settings.lfo_filter1_q_enable = BoolGen();
  // rand_settings.lfo_filter2_freq_enable = BoolGen();
  // rand_settings.lfo_filter2_q_enable = BoolGen();
  // rand_settings.lfo_master_filter_freq_enable = BoolGen();
  // rand_settings.lfo_master_filter_q_enable = BoolGen();

  // PrintSettings(rand_settings);
  // LoadSettings(rand_settings);
}

void DrumSynth::LoadPreset(std::string name,
                           std::map<std::string, double> preset_vals) {
  DrumSettings settings = Map2DrumSettings(name, preset_vals);
  LoadSettings(settings);
}

DrumSettings Map2DrumSettings(std::string name,
                              std::map<std::string, double> &preset_vals) {
  DrumSettings preset;
  preset.name = name;

  for (auto &[key, dval] : preset_vals) {
    // std::cout << "KEY:" << key << " VAL:" << dval << std::endl;
    if (key == "volume")
      preset.volume = dval;
    else if (key == "pan")
      preset.pan = dval;
    else if (key == "hard_sync")
      preset.hard_sync = dval;
    else if (key == "detune_cents")
      preset.detune_cents = dval;
    else if (key == "distortion_threshold")
      preset.distortion_threshold = dval;

    else if (key == "noise_amp")
      preset.noise_amp = dval;
    else if (key == "noise_eg_attack_ms")
      preset.noise_eg_attack_ms = dval;
    else if (key == "noise_eg_decay_ms")
      preset.noise_eg_decay_ms = dval;
    else if (key == "noise_eg_mode")
      preset.noise_eg_mode = dval;
    else if (key == "noise_filter_type")
      preset.noise_filter_type = dval;
    else if (key == "noise_filter_fc")
      preset.noise_filter_fc = dval;
    else if (key == "noise_filter_q")
      preset.noise_filter_q = dval;

    else if (key == "osc1_wav")
      preset.osc1_wav = dval;
    else if (key == "osc1_amp")
      preset.osc1_amp = dval;
    else if (key == "osc1_ratio")
      preset.osc1_ratio = dval;
    else if (key == "osc_eg_attack_ms")
      preset.osc_eg_attack_ms = dval;
    else if (key == "osc_eg_decay_ms")
      preset.osc_eg_decay_ms = dval;
    else if (key == "osc_eg_mode")
      preset.osc_eg_mode = dval;

    else if (key == "amp_eg_attack_ms")
      preset.amp_eg_attack_ms = dval;
    else if (key == "amp_eg_decay_ms")
      preset.amp_eg_decay_ms = dval;
    else if (key == "amp_eg_mode")
      preset.amp_eg_mode = dval;
    else if (key == "amp_filter_type")
      preset.amp_filter_type = dval;
    else if (key == "amp_filter_fc")
      preset.amp_filter_fc = dval;
    else if (key == "amp_filter_q")
      preset.amp_filter_q = dval;
  }

  return preset;
}

}  // namespace SBAudio
