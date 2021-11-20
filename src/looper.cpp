#include <defjams.h>
#include <libgen.h>
#include <looper.h>
#include <mixer.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils.h>

#include <iostream>

const std::array<std::string, 6> kEnvNames = {
    "PARABOLIC", "TRAPEZOIDAL", "TUKEY", "GENERATOR", "EXP_CURVE", "LOG_CURVE"};
const std::array<std::string, 3> kLoopModeNames = {"LOOP", "STATIC", "SMUDGE"};

Looper::Looper(std::string filename, unsigned int loop_mode) {
  std::cout << "NEW LOOOOOPPPPPER " << loop_mode << std::endl;

  audio_buffer_read_idx = 0;
  grain_attack_time_pct = 15;
  grain_release_time_pct = 15;
  envelope_mode = EnvMode::logarithmic_curve;
  envelope_taper_ratio = 0.5;
  reverse_mode = 0;  // bool

  density_duration_sync = true;
  fill_factor = 3.;
  SetGrainDensity(30);

  type = LOOPER_TYPE;

  ImportFile(filename);

  m_eg1.m_attack_time_msec = 10;
  m_eg1.m_release_time_msec = 50;

  degrade_by = 0;
  gate_mode = false;

  SetLoopMode(loop_mode);

  start();
}

void Looper::eventNotify(broadcast_event event, mixer_timing_info tinfo) {
  SoundGenerator::eventNotify(event, tinfo);

  if (tinfo.is_midi_tick) {
    grain_spacing = CalculateGrainSpacing(tinfo);
    if (started) {
      // increment for next step
      cur_midi_idx_ = fmodf(cur_midi_idx_ + incr_speed_, PPBAR);
    }
  }

  if (tinfo.is_start_of_loop) {
    started = true;
    loop_counter++;

    if (scramble_pending) {
      scramble_mode = true;
      scramble_pending = false;
    } else
      scramble_mode = false;

    if (stutter_pending) {
      stutter_mode = true;
      stutter_pending = false;
    } else
      stutter_mode = false;
  }

  // used to track which 16th we're on if loop != 1 bar
  float loop_num = fmod(loop_counter, loop_len);
  if (loop_num < 0) loop_num = 0;

  if (loop_mode_ == LoopMode::loop_mode) {
    // used to track which 16th we're on if loop != 1 bar
    float loop_num = fmod(loop_counter, loop_len);
    if (loop_num < 0) loop_num = 0;

    int relative_midi_idx = (loop_num * PPBAR) + cur_midi_idx_;
    double decimal_percent_of_loop = relative_midi_idx / (PPBAR * loop_len);
    double new_read_idx = decimal_percent_of_loop * audio_buffer_len;
    if (reverse_mode) new_read_idx = (audio_buffer_len - 1) - new_read_idx;

    // this ensures new_read_idx is even
    if (num_channels == 2) new_read_idx -= ((int)new_read_idx & 1);

    audio_buffer_read_idx = new_read_idx;

    int rel_pos_within_a_sixteenth =
        new_read_idx - ((tinfo.midi_tick % PPBAR) * size_of_sixteenth);
    if (scramble_mode) {
      audio_buffer_read_idx =
          (scramble_idx * size_of_sixteenth) + rel_pos_within_a_sixteenth;
    } else if (stutter_mode) {
      audio_buffer_read_idx =
          (stutter_idx * size_of_sixteenth) + rel_pos_within_a_sixteenth;
    }
  }

  if (tinfo.is_sixteenth) {
    if (scramble_mode) {
      scramble_idx = tinfo.sixteenth_note_tick % 16;
      if (scramble_idx % 2 != 0) {
        int randy = rand() % 100;
        if (randy < 25)  // repeat the third 16th
          scramble_idx = 3;
        else if (randy > 25 && randy < 50)  // repeat the 4th sixteenth
          scramble_idx = 4;
        else if (randy > 50 && randy < 75)  // repeat the 7th sixteenth
          scramble_idx = 7;
      }
    }
    if (stutter_mode) {
      if (rand() % 100 > 75) stutter_idx++;
      if (stutter_idx == 16) stutter_idx = 0;
    }
  }
}

void Looper::LaunchGrain(mixer_timing_info tinfo) {
  next_grain_launch_sample_time = tinfo.cur_sample + grain_spacing;
  cur_grain_num = GetAvailableGrainNum();

  int duration = grain_duration_ms * 44.1;
  if (quasi_grain_fudge != 0)
    duration += rand() % (int)(quasi_grain_fudge * 44.1);

  int grain_idx = audio_buffer_read_idx;

  if (granular_spray_frames > 0) grain_idx += rand() % granular_spray_frames;

  int attack_time_pct = grain_attack_time_pct;
  int release_time_pct = grain_release_time_pct;

  SoundGrainParams params = {.dur = duration,
                             .starting_idx = grain_idx,
                             .attack_pct = attack_time_pct,
                             .release_pct = release_time_pct,
                             .reverse_mode = reverse_mode,
                             .pitch = grain_pitch,
                             .num_channels = num_channels,
                             .degrade_by = degrade_by,
                             .envelope_mode = envelope_mode};

  m_grains[cur_grain_num].Initialize(params);
  num_active_grains = CountActiveGrains();
}

stereo_val Looper::genNext(mixer_timing_info tinfo) {
  stereo_val val = {0., 0.};

  if (!started || !active) return val;

  if (stop_pending && m_eg1.m_state == OFFF) active = false;

  if (tinfo.cur_sample > next_grain_launch_sample_time)  // new grain time
    LaunchGrain(tinfo);

  for (int i = 0; i < highest_grain_num; i++) {
    stereo_val tmp = m_grains[i].Generate(audio_buffer, audio_buffer_len);

    val.left += tmp.left;
    val.right += tmp.right;
  }

  m_eg1.Update();
  double eg_amp = m_eg1.DoEnvelope(NULL);

  pan = fmin(pan, 1.0);
  pan = fmax(pan, -1.0);
  double pan_left = 0.707;
  double pan_right = 0.707;
  calculate_pan_values(pan, &pan_left, &pan_right);

  val.left = val.left * volume * eg_amp * pan_left;
  val.right = val.right * volume * eg_amp * pan_right;

  // val.left = val.left * volume * pan_left;
  // val.right = val.right * volume * pan_right;

  val = Effector(val);

  return val;
}

std::string Looper::Status() {
  std::stringstream ss;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << ANSI_COLOR_RED;
  ss << filename << " vol:" << volume << " pan:" << pan
     << " pitch:" << grain_pitch
     << " idx:" << (int)(100. / audio_buffer_len * audio_buffer_read_idx)
     << " mode:" << kLoopModeNames[loop_mode_] << "(" << loop_mode_ << ")"
     << " len:" << loop_len << ANSI_COLOR_RESET;

  return ss.str();
}
std::string Looper::Info() {
  char *INSTRUMENT_COLOR = (char *)ANSI_COLOR_RESET;
  if (active) INSTRUMENT_COLOR = (char *)ANSI_COLOR_RED;

  std::stringstream ss;
  ss << ANSI_COLOR_WHITE << filename << INSTRUMENT_COLOR << " vol:" << volume
     << " pan:" << pan << " pitch:" << grain_pitch << " speed:" << incr_speed_
     << " mode:" << kLoopModeNames[loop_mode_]
     << " env_mode:" << kEnvNames[envelope_mode] << "(" << envelope_mode << ") "
     << "\ngrain_dur_ms:" << grain_duration_ms
     << " grains_per_sec:" << grains_per_sec
     << " density_dur_sync:" << density_duration_sync
     << " quasi_grain_fudge:" << quasi_grain_fudge
     << " fill_factor:" << fill_factor
     << "\ngrain_spray_ms:" << granular_spray_frames / 44.1 << "\n";

  return ss.str();
}

void Looper::start() {
  m_eg1.StartEg();
  active = true;
  stop_pending = false;
}

void Looper::stop() {
  m_eg1.Release();
  stop_pending = true;
}

Looper::~Looper() {
  // TODO delete file
}

//////////////////////////// grain stuff //////////////////////////
// looper functions continue below

void SoundGrain::Initialize(SoundGrainParams params) {
  amp = 0;
  audiobuffer_start_idx = params.starting_idx;
  grain_len_frames = params.dur;
  grain_counter_frames = 0;

  attack_time_pct = params.attack_pct;
  release_time_pct = params.release_pct;

  attack_time_samples = params.dur / 100. * params.attack_pct;
  attack_to_sustain_boundary_sample_idx = attack_time_samples;
  release_time_samples = params.dur / 100. * params.release_pct;
  sustain_to_decay_boundary_sample_idx = params.dur - release_time_samples;

  audiobuffer_num_channels = params.num_channels;
  degrade_by = params.degrade_by;
  debug = params.debug;

  reverse_mode = params.reverse_mode;
  if (params.reverse_mode) {
    audiobuffer_cur_pos =
        params.starting_idx + (params.dur * params.num_channels) - 1;
    incr = -1.0 * params.pitch;
  } else {
    audiobuffer_cur_pos = params.starting_idx;
    incr = params.pitch;
  }

  // case (LOOPER_ENV_LOGARITHMIC_CURVE):
  exp_mul = pow(exp_min / (exp_min + 1), 1.0 / attack_time_samples);
  exp_now = exp_min + 1;

  amp = 0;
  double rdur = 1.0 / params.dur;
  double rdur2 = rdur * rdur;
  slope = 4.0 * 1.0 * (rdur - rdur2);
  curve = -8.0 * 1.0 * rdur2;

  double loop_len_ms = 1000. * params.dur / SAMPLE_RATE;
  double attack_time_ms = loop_len_ms / 100. * params.attack_pct;
  double release_time_ms = loop_len_ms / 100. * params.release_pct;
  // printf("ATTACKMS: %f RELEASEMS: %f\n", attack_time_ms,
  // release_time_ms);

  eg.Reset();
  eg.SetAttackTimeMsec(attack_time_ms);
  eg.SetDecayTimeMsec(0);
  eg.SetReleaseTimeMsec(release_time_ms);
  eg.StartEg();

  active = true;
}

static inline void sound_grain_check_idx(int *index, int buffer_len) {
  while (*index < 0.0) *index += buffer_len;
  while (*index >= buffer_len) *index -= buffer_len;
}

stereo_val SoundGrain::Generate(std::vector<double> &audio_buffer,
                                int audio_buffer_len) {
  stereo_val out = {0., 0.};
  if (!active) return out;

  if (degrade_by > 0) {
    if (rand() % 100 < degrade_by) return out;
  }

  int num_channels = audiobuffer_num_channels;

  int read_idx = (int)audiobuffer_cur_pos;
  double frac = audiobuffer_cur_pos - read_idx;
  sound_grain_check_idx(&read_idx, audio_buffer_len);

  if (num_channels == 1) {
    int read_next_idx = read_idx + 1;
    sound_grain_check_idx(&read_next_idx, audio_buffer_len);
    out.left = utils::LinTerp(0, 1, audio_buffer[read_idx],
                              audio_buffer[read_next_idx], frac);
    out.left *= amp;
    out.right = out.left;
  } else if (num_channels == 2) {
    int read_next_idx = read_idx + 2;
    sound_grain_check_idx(&read_next_idx, audio_buffer_len);
    out.left = utils::LinTerp(0, 1, audio_buffer[read_idx],
                              audio_buffer[read_next_idx], frac);
    out.left *= amp;

    int read_idx_right = read_idx + 1;
    sound_grain_check_idx(&read_idx_right, audio_buffer_len);
    int read_next_idx_right = read_idx_right + 2;
    sound_grain_check_idx(&read_next_idx_right, audio_buffer_len);
    out.right = utils::LinTerp(0, 1, audio_buffer[read_idx_right],
                               audio_buffer[read_next_idx_right], frac);
    out.right *= amp;
  }
  audiobuffer_cur_pos += (incr * num_channels);

  if (grain_counter_frames < attack_to_sustain_boundary_sample_idx ||
      grain_counter_frames > sustain_to_decay_boundary_sample_idx) {
    exp_now *= exp_mul;
    amp = (1 - (exp_now - exp_min));
  } else if (grain_counter_frames == attack_to_sustain_boundary_sample_idx) {
    amp = 1.;
  } else if (grain_counter_frames == sustain_to_decay_boundary_sample_idx) {
    exp_now = exp_min;
    exp_mul = pow((exp_min + 1) / exp_min, 1 / release_time_samples);
  }

  grain_counter_frames++;
  if (grain_counter_frames > grain_len_frames) {
    active = false;
  }

  return out;
}

//////////////////////////// end of grain stuff //////////////////////////

void Looper::ImportFile(std::string filename) {
  AudioBufferDetails deetz = ImportFileContents(audio_buffer, filename);
  audio_buffer_len = deetz.buffer_length;
  num_channels = deetz.num_channels;
  SetLoopLen(1);
}

int Looper::CalculateGrainSpacing(mixer_timing_info tinfo) {
  int looplen_in_seconds = tinfo.loop_len_in_frames / (double)SAMPLE_RATE;
  num_grains_per_looplen = looplen_in_seconds * grains_per_sec;
  if (num_grains_per_looplen == 0) {
    num_grains_per_looplen = 2;  // whoops! dn't wanna div by 0 below
  }
  int spacing = tinfo.loop_len_in_frames / num_grains_per_looplen;
  return spacing;
}

void Looper::SetGrainDuration(int dur) {
  grain_duration_ms = dur;
  if (density_duration_sync)
    grains_per_sec = 1000. / (grain_duration_ms / fill_factor);
}

void Looper::SetGrainDensity(int gps) {
  grains_per_sec = gps;
  if (density_duration_sync)
    grain_duration_ms = 1000. / grains_per_sec * fill_factor;
}

void Looper::SetGrainAttackSizePct(int attack_pct) {
  if (attack_pct < 50) grain_attack_time_pct = attack_pct;
}

void Looper::SetGrainReleaseSizePct(int release_pct) {
  if (release_pct < 50) grain_release_time_pct = release_pct;
}

void Looper::SetAudioBufferReadIdx(int pos) {
  if (pos < 0 || pos >= audio_buffer_len) {
    return;
  }
  audio_buffer_read_idx = pos;
}

void Looper::SetGranularSpray(int spray_ms) {
  int spray_frames = spray_ms * 44.1;
  granular_spray_frames = spray_frames;
}

void Looper::SetQuasiGrainFudge(int fudgefactor) {
  quasi_grain_fudge = fudgefactor;
}

void Looper::SetGrainPitch(double pitch) { grain_pitch = pitch; }
void Looper::SetIncrSpeed(double speed) { incr_speed_ = speed; }

void Looper::SetEnvelopeMode(unsigned int mode) { envelope_mode = mode; }

void Looper::SetReverseMode(bool b) { reverse_mode = b; }

void Looper::SetLoopMode(unsigned int m) {
  std::cout << "LOOP MODE IS:" << m << std::endl;
  volume = 0.2;
  switch (m) {
    case (0):
      loop_mode_ = LoopMode::loop_mode;
      volume = 0.7;
      break;
    case (1):
      loop_mode_ = LoopMode::smudge_mode;
      quasi_grain_fudge = 220;
      granular_spray_frames = 441;  // 10ms * (44100/1000)
      break;
    case (3):
    default:
      quasi_grain_fudge = 0;
      granular_spray_frames = 0;
  }
}
void Looper::SetScramblePending() { scramble_pending = true; }

void Looper::SetStutterPending() { stutter_pending = true; }

void Looper::SetLoopLen(double bars) {
  if (bars != 0) {
    loop_len = bars;
    size_of_sixteenth = audio_buffer_len / 16 * bars;
  }
}

int Looper::GetAvailableGrainNum() {
  int idx = 0;
  while (idx < kMaxConcurrentGrains) {
    if (!m_grains[idx].active) {
      if (idx > highest_grain_num) highest_grain_num = idx;
      return idx;
    }
    idx++;
  }
  printf("WOW - NO GRAINS TO BE FOUND IN %d attempts\n", idx);
  return 0;
}

int Looper::CountActiveGrains() {
  int active = 0;
  for (int i = 0; i < highest_grain_num; i++)
    if (m_grains[i].active) active++;

  return active;
}

void Looper::SetFillFactor(double fillfactor) {
  if (fillfactor >= 0. && fillfactor <= 10.) fill_factor = fillfactor;
}

void Looper::SetDensityDurationSync(bool b) { density_duration_sync = b; }

void Looper::SetGateMode(bool b) { gate_mode = b; }

void Looper::SetGrainEnvAttackPct(int percent) {
  if (percent > 0 && percent < 100) grain_attack_time_pct = percent;
}
void Looper::SetGrainEnvReleasePct(int percent) {
  if (percent > 0 && percent < 100) grain_release_time_pct = percent;
}

void Looper::SetDegradeBy(int degradation) {
  if (degradation >= 0 && degradation <= 100) degrade_by = degradation;
}

void Looper::noteOn(midi_event ev) {
  (void)ev;
  m_eg1.StartEg();
}

void Looper::noteOff(midi_event ev) {
  (void)ev;
  m_eg1.NoteOff();
}

void Looper::SetParam(std::string name, double val) {
  if (name == "active") {
    active = val;
  } else if (name == "pitch")
    SetGrainPitch(val);
  else if (name == "speed")
    SetIncrSpeed(val);
  else if (name == "mode") {
    SetLoopMode(val);
  } else if (name == "gate_mode")
    SetGateMode(val);
  else if (name == "idx") {
    if (val <= 100) {
      double pos = audio_buffer_len / 100 * val;
      SetAudioBufferReadIdx(pos);
    }
  } else if (name == "len")
    SetLoopLen(val);
  else if (name == "scramble")
    SetScramblePending();
  else if (name == "stutter")
    SetStutterPending();
  else if (name == "reverse")
    SetReverseMode(val);
  else if (name == "grain_dur_ms")
    SetGrainDuration(val);
  else if (name == "grains_per_sec")
    SetGrainDensity(val);
  else if (name == "density_dur_sync")
    SetDensityDurationSync(val);
  else if (name == "quasi_grain_fudge")
    SetQuasiGrainFudge(val);
  else if (name == "fill_factor")
    SetFillFactor(val);
  else if (name == "grain_spray_ms")
    SetGranularSpray(val);
  else if (name == "env_mode")
    SetEnvelopeMode(val);
}

double Looper::GetParam(std::string name) { return 0; }
