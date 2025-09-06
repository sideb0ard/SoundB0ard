#pragma once

#include <audioutils.h>
#include <fx/fx.h>

#include <interpreter/object.hpp>
#include <map>
#include <string>
#include <vector>

#include "drum_synth.h"
#include "filebuffer.h"
#include "soundgenerator.h"

enum AudioAction {
  ADD,
  ADD_FX,
  ADD_BUFFER,
  BPM,
  INFO,
  HELP,
  LIST_PRESETS,
  LOAD_PRESET,
  RAND_PRESET,
  RECORDED_MIDI_EVENT,
  MIDI_NOTE_ON,
  MIDI_NOTE_ON_DELAYED,
  MIDI_NOTE_OFF,
  MIDI_NOTE_OFF_DELAYED,
  MIDI_EVENT_CLEAR,
  MIDI_INIT,
  MIDI_MAP,
  MIDI_MAP_SHOW,
  MIXER_UPDATE,
  MIXER_FX_UPDATE,
  MIXER_XFADE_ASSIGN,
  MIXER_XFADE_ACTION,
  MIXER_XFADE_CLEAR,
  MONITOR,
  NO_ACTION,
  NOW,
  PREVIEW,
  RAND,
  SAVE_PRESET,
  STATUS,
  SOLO,
  STOP,
  UNSOLO,
  UPDATE,
  VOLUME,
  ENABLE_WEBSOCKET,
};

struct AudioActionItem {
  explicit AudioActionItem(AudioAction type) : type{type} {}
  ~AudioActionItem() = default;
  AudioAction type;
  std::unique_ptr<SBAudio::SoundGenerator> sg{nullptr};
  std::unique_ptr<SBAudio::FileBuffer> fb{nullptr};
  int mixer_soundgen_idx{-1};
  bool is_xfader{false};
  std::vector<int> group_of_soundgens{};
  std::vector<std::shared_ptr<Fx>> fx;
  unsigned int xfade_channel{99};
  unsigned int xfade_direction{99};

  // can be used by simple action types rather than add specific field
  bool general_val{false};

  std::string preset_name;
  std::map<std::string, double> preset;

  std::vector<double> buffer;
  AudioBufferDetails audio_buffer_details;

  int delayed_by{0};  // in midi ticks
  int start_at{0};    // in midi ticks

  bool has_midi_event{false};
  midi_event event;

  // MIDI MAP TYPE
  int mapped_id{-1};
  std::string mapped_param;

  // ADD varz
  unsigned int soundgenerator_type{static_cast<unsigned int>(-1)};
  std::string filepath;  // used for sample and digisynth

  // STATUS varz
  bool status_all{false};

  // UPDATE varz
  int fx_id{0};
  std::string param_name{};
  std::string param_val{0};

  // MIXER FX VARZ
  int mixer_fx_id{-1};
  double fx_intensity{0};

  // NOTE_ON varz
  std::vector<std::shared_ptr<object::Object>> args;
  int soundgen_num{-1};
  std::vector<int> notes;
  int velocity{0};
  int duration{0};
  int note_start_time{0};

  // PREVIEW varz
  std::string preview_filename;

  // ADD_FX varz
  // PRESET varz

  // BPM varz
  double new_bpm{0};
  double new_volume{0};
};
