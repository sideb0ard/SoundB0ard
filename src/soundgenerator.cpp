#include <defjams.h>
#include <fx/basicfilterpass.h>
#include <fx/bitcrush.h>
#include <fx/decimate.h>
#include <fx/distortion.h>
#include <fx/dynamics_processor.h>
#include <fx/fx.h>
#include <fx/genz.h>
#include <fx/modfilter.h>
#include <fx/modular_delay.h>
#include <fx/reverb.h>
#include <fx/stereodelay.h>
#include <fx/waveshaper.h>
#include <math.h>
#include <soundgenerator.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

namespace SBAudio {

SoundGenerator::SoundGenerator() {};

double SoundGenerator::GetVolume() {
  return volume;
}

void SoundGenerator::SetVolume(double val) {
  if (val >= 0.0 && val <= 1.0) volume = val;
}

void SoundGenerator::SetFxSend(int fx_num, double intensity) {
  if (fx_num < kMixerNumSendFx && intensity >= 0 && intensity <= 1) {
    mixer_fx_send_intensity_[fx_num] = intensity;
  }
}

void SoundGenerator::Start() {
  active = true;
}
void SoundGenerator::Stop() {
  active = false;
}

void SoundGenerator::Save(std::string preset_name) {
  std::cout << "BASE CLASS SAVE " << preset_name << " - NO OP!" << std::endl;
}

std::string SoundGenerator::Status() {
  return std::string{"BASE CLASS, YO"};
}

void SoundGenerator::ParseMidiEvent(midi_event ev, mixer_timing_info tinfo) {
  (void)tinfo;

  switch (ev.event_type) {
    case (MIDI_ON): {  // Hex 0x80
      NoteOn(ev);
      break;
    }
    case (MIDI_OFF): {  // Hex 0x90
      if (ev.data1 == 0) {
        AllNotesOff();
      } else {
        NoteOff(ev);
      }
      break;
    }
    case (MIDI_CONTROL): {  // Hex 0xB0
      Control(ev);
      break;
    }
    case (MIDI_PITCHBEND): {  // Hex 0xE0
      PitchBend(ev);
      break;
    }
    default:
      std::cout << "HERE PAL, I've NAE IDEA WHIT KIND OF MIDI EVENT THAT WiS! "
                << ev << std::endl;
  }
}

void SoundGenerator::EventNotify(broadcast_event event,
                                 mixer_timing_info tinfo) {
  (void)event;
  (void)tinfo;
}

double SoundGenerator::GetPan() {
  return pan;
}

void SoundGenerator::SetPan(double val) {
  if (val >= -1.0 && val <= 1.0) pan = val;
}

void SoundGenerator::AddFx(std::shared_ptr<Fx> f) {
  std::cout << "YO, ADDING FX TO SG\n";
  if (effects_num < kMaxNumSoundGenFx) {
    effects_[effects_num++] = f;
    printf("done adding effect\n");
  }
}

StereoVal SoundGenerator::Effector(StereoVal val) {
  int num_fx = effects_num.load();
  // Ensure we don't exceed the actual array size to prevent race conditions
  int safe_fx_count = (num_fx < static_cast<int>(effects_.size()))
                          ? num_fx
                          : static_cast<int>(effects_.size());

  for (int i = 0; i < safe_fx_count; i++) {
    // Additional bounds check in case array was resized
    if (i >= effects_.size()) break;

    auto f = effects_[i];
    if (f && f->enabled_) {
      val = f->Process(val);
    }
  }
  return val;
}

bool SoundGenerator::IsSynth() {
  if (type == MINISYNTH_TYPE || type == DXSYNTH_TYPE || type == DRUMSYNTH_TYPE)
    return true;

  return false;
}

bool SoundGenerator::IsStepper() {
  if (type == DRUMSAMPLER_TYPE) return true;
  return false;
}

}  // namespace SBAudio
