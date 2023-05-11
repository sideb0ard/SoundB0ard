#pragma once

#include <audioutils.h>
#include <fx/fx.h>
#include <soundgenerator.h>

#include <interpreter/object.hpp>
#include <map>
#include <string>
#include <vector>

#include "drum_synth.h"

enum AudioAction {
  ADD,
  ADD_FX,
  BPM,
  INFO,
  HELP,
  LIST_PRESETS,
  LOAD_PRESET,
  RAND_PRESET,
  RECORDED_MIDI_EVENT,
  MIDI_EVENT_ADD,
  MIDI_EVENT_ADD_DELAYED,
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
  PREVIEW,
  RAND,
  SAVE_PRESET,
  STATUS,
  SOLO,
  STOP,
  UNSOLO,
  UPDATE,
  VOLUME,
};

struct audio_action_queue_item {
  AudioAction type;
  int mixer_soundgen_idx{-1};
  bool is_xfader{false};
  std::vector<int> group_of_soundgens{};
  std::shared_ptr<SBAudio::SoundGenerator> sg;
  std::vector<std::shared_ptr<Fx>> fx;
  unsigned int xfade_channel{99};
  unsigned int xfade_direction{99};

  std::string preset_name;
  std::map<std::string, double> preset;

  std::vector<double> buffer;
  AudioBufferDetails audio_buffer_details;

  int delayed_by{0};  // in midi ticks
  int start_at{0};    // in midi ticks

  bool has_midi_event{false};
  midi_event event;

  // MIDI MAP TYPE
  int mapped_id;
  std::string mapped_param;

  // ADD varz
  unsigned int soundgenerator_type;
  std::string filepath;  // used for sample and digisynth
  // bool loop_mode{false};  // ? NOT USED?

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
  int velocity;
  int duration;
  int note_start_time;

  // PREVIEW varz
  std::string preview_filename;

  // ADD_FX varz
  // PRESET varz

  // BPM varz
  double new_bpm;
  double new_volume;
};
