#pragma once

#include <defjams.h>
#include <fx/envelope.h>
#include <fx/fx.h>
#include <stdbool.h>

#include <array>
#include <atomic>
#include <iostream>
#include <map>

namespace SBAudio {
class SoundGenerator {
 public:
  SoundGenerator();
  virtual ~SoundGenerator() = default;

  virtual StereoVal GenNext(mixer_timing_info tinfo) = 0;

  virtual std::string Info() = 0;
  virtual std::string Status() = 0;

  virtual void SetParam(std::string name, double val) = 0;

  virtual void start();
  virtual void stop();

  virtual void noteOn(midi_event ev) { (void)ev; };
  virtual void noteOff(midi_event ev) { (void)ev; };
  virtual void Save(std::string preset_name);
  virtual void LoadPreset(std::string preset_name,
                          std::map<std::string, double> preset) {
    (void)preset_name;
    (void)preset;
  }
  virtual void SavePreset(std::string preset_name,
                          std::map<std::string, double> preset) {
    (void)preset_name;
    (void)preset;
  }
  virtual void ListPresets();

  virtual void control(midi_event ev) { (void)ev; };
  virtual void pitchBend(midi_event ev) { (void)ev; };
  virtual void randomize(){};
  virtual void allNotesOff(){};

  virtual void EventNotify(broadcast_event event, mixer_timing_info tinfo);

  void parseMidiEvent(midi_event ev, mixer_timing_info tinfo);

  void SetVolume(double val);
  double GetVolume();

  void SetPan(double val);
  double GetPan();

  void SetFxSend(int fx_num, double intensity);

 public:
  sound_generator_type type;

  int soundgen_id_{-1};

  bool active;

  double volume{1.0};  // between 0 and 1.0
  double pan{0.};      // between -1(hard left) and 1(hard right)

  // hard coded - 0: delay 1: reverb 2: distortion
  std::array<double, kMixerNumSendFx> mixer_fx_send_intensity_{0};

  int note_duration_ms_{100};

  std::atomic<int16_t> effects_num{0};  // num of effects
  std::array<std::shared_ptr<Fx>, kMaxNumSoundGenFx> effects_;
  bool effects_on{true};  // bool

 public:
  void AddFx(std::shared_ptr<Fx>);

  StereoVal Effector(StereoVal val);

  bool IsSynth();
  bool IsStepper();
};

}  // namespace SBAudio
