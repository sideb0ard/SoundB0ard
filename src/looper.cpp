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

namespace SBAudio {

namespace {
void check_idx(int *index, int buffer_len) {
  while (*index < 0.0) *index += buffer_len;
  while (*index >= buffer_len) *index -= buffer_len;
}

constexpr double len_frame_ms = 1000. / 44100;
const std::array<std::string, 3> kLoopModeNames = {"LOOP", "STATIC", "SMUDGE"};

}  // namespace

Looper::Looper(std::string filename, unsigned int loop_mode) {
  std::cout << "NEW LOOOOOPPPPPER " << loop_mode << std::endl;

  audio_buffer_read_idx = 0;
  grain_attack_time_pct = 15;
  grain_release_time_pct = 15;
  reverse_mode = false;

  density_duration_sync = true;
  // fill_factor = 2.;
  SetGrainDensity(30);

  type = LOOPER_TYPE;

  ImportFile(filename);
  number_of_frames = audio_buffer.size() / num_channels;

  m_eg1.m_attack_time_msec = 10;
  m_eg1.m_release_time_msec = 50;

  degrade_by = 0;
  gate_mode = false;

  SetLoopMode(loop_mode);

  start();
}

void Looper::eventNotify(broadcast_event event, mixer_timing_info tinfo) {
  SoundGenerator::eventNotify(event, tinfo);

  double decimal_percent_of_loop = 0;
  if (tinfo.is_start_of_loop) {
    started = true;
  }

  if (tinfo.is_midi_tick) {
    // grain_spacing = CalculateGrainSpacing(tinfo);
    if (started)
      cur_midi_idx_ = fmodf(cur_midi_idx_ + incr_speed_, PPBAR * loop_len);
    // cur_frame_tick = (cur_frame_tick + 1) % number_of_frames;
    // idx_incr = audio_buffer.size() / tinfo.loop_len_in_frames;
    if (loop_mode_ == LoopMode::loop_mode) {
      decimal_percent_of_loop = cur_midi_idx_ / (PPBAR * loop_len);
      double new_read_idx = decimal_percent_of_loop * audio_buffer.size();
      if (reverse_mode) new_read_idx = (audio_buffer.size() - 1) - new_read_idx;

      // this ensures new_read_idx is even
      if (num_channels == 2) new_read_idx -= ((int)new_read_idx & 1);

      audio_buffer_read_idx = new_read_idx;
      int rel_pos_within_a_sixteenth =
          fmod(audio_buffer_read_idx, size_of_sixteenth);
      if (scramble_mode) {
        audio_buffer_read_idx =
            (scramble_idx * size_of_sixteenth) + rel_pos_within_a_sixteenth;
      } else if (stutter_mode) {
        audio_buffer_read_idx =
            (stutter_idx * size_of_sixteenth) + rel_pos_within_a_sixteenth;
      }
    }
    // std::cout << "MIDITICK:"
    //           << "Decimal Position:" << decimal_percent_of_loop
    //           << " READIDX:" << audio_buffer_read_idx
    //           << " MIDITICK:" << (tinfo.midi_tick % PPBAR)
    //           << " My MiIDITICK:" << cur_midi_idx_ << std::endl;
  }

  if (tinfo.is_start_of_loop) {
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

  float duration_frames = grain_duration_ms * 44.1;
  if (quasi_grain_fudge != 0)
    duration_frames += rand() % (int)(quasi_grain_fudge * 44.1);

  int grain_idx = audio_buffer_read_idx;

  if (granular_spray_frames > 0) grain_idx += rand() % granular_spray_frames;

  // float duration_frames = grain_duration_ms * 44.1;
  attack_frames_ = duration_frames / 100. * grain_attack_time_pct;
  SoundGrainParams params = {.dur_frames = duration_frames,
                             .starting_idx = grain_idx,
                             .attack_pct = grain_attack_time_pct,
                             .release_pct = grain_release_time_pct,
                             .reverse_mode = reverse_mode,
                             .pitch = grain_pitch,
                             .num_channels = num_channels,
                             .degrade_by = degrade_by,
                             .envelope_mode = envelope_mode,
                             .dur_ms = grain_duration_ms};

  m_grains[cur_grain_num].Initialize(params);
  num_active_grains = CountActiveGrains();
}

stereo_val Looper::GenNext(mixer_timing_info tinfo) {
  stereo_val val = {0., 0.};
  if (!started || !active) return val;

  if (stop_pending && m_eg1.m_state == OFFF) active = false;

  if (loop_mode_ == LoopMode::loop_mode) {
    float loop_num = fmod(loop_counter, loop_len);
    if (loop_num < 0) loop_num = 0;
  }

  if (tinfo.cur_sample > next_grain_launch_sample_time)  // new grain time
    LaunchGrain(tinfo);

  for (int i = 0; i < highest_grain_num; i++) {
    stereo_val tmp = m_grains[i].Generate(audio_buffer);
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

  val = Effector(val);

  // if (reverse_mode)
  //   audio_buffer_read_idx--;
  // else
  //   audio_buffer_read_idx++;
  // check_idx(&audio_buffer_read_idx, audio_buffer.size());

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
     << " idx:" << (int)(100. / audio_buffer.size() * audio_buffer_read_idx)
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
     << "\ngrain_dur_ms:" << grain_duration_ms
     << " grains_per_sec:" << grains_per_sec
     << " density_dur_sync:" << density_duration_sync
     << " quasi_grain_fudge:" << quasi_grain_fudge
     << " fill_factor:" << fill_factor
     << "\ngrain_spray_ms:" << granular_spray_frames / 44.1
     << " attack:" << m_eg1.m_attack_time_msec
     << " decay:" << m_eg1.m_decay_time_msec
     << " release:" << m_eg1.m_release_time_msec
     << " grain_att_pct:" << grain_attack_time_pct
     << " grain_rel_pct:" << grain_release_time_pct << "\n";

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
  audiobuffer_start_idx = params.starting_idx;
  grain_len_frames = params.dur_frames;
  grain_counter_frames = 0;

  start_sustain_frame = grain_len_frames / 100. * params.attack_pct;
  release_frame = params.dur_frames - start_sustain_frame;

  // amp_increment = 1 / start_sustain_frame;
  eg.Reset();
  eg.SetSustainOverride(true);
  eg.SetAttackTimeMsec(start_sustain_frame * len_frame_ms);
  eg.SetDecayTimeMsec(0);
  eg.SetSustainLevel(1.);
  eg.SetReleaseTimeMsec(start_sustain_frame * len_frame_ms);
  eg.StartEg();

  audiobuffer_num_channels = params.num_channels;
  degrade_by = params.degrade_by;
  debug = params.debug;

  reverse_mode = params.reverse_mode;
  if (params.reverse_mode) {
    audiobuffer_cur_pos =
        params.starting_idx + (params.dur_frames * params.num_channels) - 1;
    incr = -1.0 * params.pitch;
  } else {
    audiobuffer_cur_pos = params.starting_idx;
    incr = params.pitch;
  }

  active = true;
}

stereo_val SoundGrain::Generate(std::vector<double> &audio_buffer) {
  stereo_val out = {0., 0.};
  if (!active) return out;

  if (degrade_by > 0) {
    if (rand() % 100 < degrade_by) return out;
  }

  int num_channels = audiobuffer_num_channels;

  int read_idx = (int)audiobuffer_cur_pos;
  double frac = audiobuffer_cur_pos - read_idx;
  check_idx(&read_idx, audio_buffer.size());

  double eg_amp = eg.DoEnvelope(nullptr);

  out.left = audio_buffer[read_idx] * eg_amp;
  if (num_channels == 1) {
    out.right = out.left;
  } else if (num_channels == 2) {
    int read_idx_right = read_idx + 1;
    out.right = audio_buffer[read_idx_right] * eg_amp;
  }
  audiobuffer_cur_pos += (incr * num_channels);

  grain_counter_frames++;

  if (grain_counter_frames == release_frame) {
    eg.SetSustainOverride(false);
    eg.Release();
  }

  if (grain_counter_frames > grain_len_frames) {
    active = false;
  }

  return out;
}

//////////////////////////// end of grain stuff //////////////////////////

void Looper::ImportFile(std::string filename) {
  AudioBufferDetails deetz = ImportFileContents(audio_buffer, filename);
  num_channels = deetz.num_channels;
  SetLoopLen(1);
}

void Looper::SetGrainDuration(int dur) {
  grain_duration_ms = dur;
  if (density_duration_sync) grains_per_sec = 1000. / grain_duration_ms;
}

void Looper::SetGrainDensity(int gps) {
  grains_per_sec = gps;
  if (density_duration_sync) grain_duration_ms = 1000. / grains_per_sec;

  float duration_frames = grain_duration_ms * 44.1;
  attack_frames_ = duration_frames / 100. * grain_attack_time_pct;

  grain_spacing = (44100. / grains_per_sec) - attack_frames_;
}

void Looper::SetGrainAttackSizePct(int attack_pct) {
  if (attack_pct < 50) grain_attack_time_pct = attack_pct;
}

void Looper::SetGrainReleaseSizePct(int release_pct) {
  if (release_pct < 50) grain_release_time_pct = release_pct;
}

void Looper::SetAudioBufferReadIdx(int pos) {
  if (pos < 0 || pos >= audio_buffer.size()) {
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
    size_of_sixteenth = audio_buffer.size() / 16 * bars;
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
  // printf("WOW - NO GRAINS TO BE FOUND IN %d attempts\n", idx);
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
  if (name == "on") {
    m_eg1.StartEg();
  } else if (name == "off") {
    m_eg1.NoteOff();
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
      double pos = audio_buffer.size() / 100 * val;
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
  else if (name == "attack")
    m_eg1.SetAttackTimeMsec(val);
  else if (name == "decay")
    m_eg1.SetDecayTimeMsec(val);
  else if (name == "release")
    m_eg1.SetReleaseTimeMsec(val);
  else if (name == "grain_att_pct")
    SetGrainAttackSizePct(val);
  else if (name == "grain_rel_pct")
    SetGrainReleaseSizePct(val);
}
}  // namespace SBAudio
