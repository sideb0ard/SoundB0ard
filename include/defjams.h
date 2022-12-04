#pragma once

#include <stdbool.h>

static const char DX_PRESET_FILENAME[] = "settings/dxpresets.dat";
static const char MOOG_PRESET_FILENAME[] = "settings/moogpresets.dat";

#include <array>
#include <string>
#include <vector>

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (64)

#define NUM_STATEGIES 100

#ifndef M_PI
#define M_PI (3.14159265358897932)
#endif

#define SIZE_OF_WURD 256
#define NUM_WURDS 25
#define DEFAULT_BPM 140

#define SAMPLE_DIR "/wavs/"

#define DEFAULT_ENV_LENGTH 8.0  // two bars Envelope

#define DEFAULT_ARRAY_SIZE 32

#define DEFAULT_VELOCITY 92

#define MAX_STATIC_STRING_SZ 4096  // arbitrary

#define ENVIRONMENT_ARRAY_SIZE 128
#define ENVIRONMENT_KEY_SIZE 128

#define PPQN 960  // Pulses Per Quarter Note // one beat
#define PPSIXTEENTH (PPQN / 4)
#define PPTWENTYFOURTH (PPQN / 6)
#define PPTHIRTYSECOND (PPQN / 8)
#define PPBAR (PPQN * 4)  // Pulses per loop/bar - i.e 4 * beats

#define TWO_PI (2.0 * M_PI)
#define FREQRAD (TWO_PI / SAMPLE_RATE)

#define TABLEN (1024.00)
#define TABRAD (TABLEN / SAMPLE_RATE)

#define NHARMS 7  // number of harmonics

#define NUM_COMPAT_NOTES 6

#define MAX_NUM_PROC 100
#define MAX_NUM_SOUND_GENERATORS 100
#define NUM_PROGRESSIONS 4

#define MAX_NUM_MIDI_LOOPS 64
#define MAX_VOICES 4
#define DEFAULT_LEGATO_MODE 0
#define DEFAULT_RESET_TO_ZERO 0
#define DEFAULT_FILTER_KEYTRACK 0
#define DEFAULT_FILTER_KEYTRACK_INTENSITY 0.5
#define DEFAULT_PORTAMENTO_TIME_MSEC 0.0

constexpr int kMaxNumSoundGenFx = 20;

constexpr int kMaxDelayLenSecs = 10;

const std::string kStartupConfigFile = "startup.sb";

#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_GREEN_TOO "\x1b[38;5;34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_PINK "\x1b[38;5;13m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_DEEP_RED "\x1b[38;5;124m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_WHITE "\x1b[37m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define COOL_COLOR_GREEN "\x1b[38;5;47m"
#define COOL_COLOR_YELLOW "\x1b[38;5;226m"
#define COOL_COLOR_PINK "\x1b[38;5;200m"
#define COOL_COLOR_MAUVE "\x1b[38;5;135m"
#define COOL_COLOR_ORANGE "\x1b[38;5;202m"
#define COOL_COLOR_YELLOW_MELLOW "\x1b[38;5;190m"
#define COOL_COLOR_BLUE "\x1b[38;5;45m"
#define COOL_COLOR_PINK2 "\x1b[38;5;219m"

#define NADA -999

#define MAX_LENGTH_VAR_VALUE 128

enum Quantize { Q32, Q16, Q8, Q4, Q2 };

enum { LEFT, RIGHT };

enum { UP, DOWN };

enum {
  MIDI_ON = 144,
  MIDI_OFF = 128,
  MIDI_CONTROL = 176,
  MIDI_PITCHBEND = 224,
};

enum {
  MIDI_KNOB_MODE_ONE,
  MIDI_KNOB_MODE_TWO,
  MIDI_KNOB_MODE_THREE,
  MAX_NUM_KNOB_MODES,
};

enum {
  KEY_MODE_ONE,
  KEY_MODE_TWO,
  MAX_NUM_KEY_MODES,
};

typedef enum {
  MINISYNTH_TYPE,
  DXSYNTH_TYPE,
  LOOPER_TYPE,
  DRUMSAMPLER_TYPE,
  DRUMSYNTH_TYPE,
  SBSYNTH_TYPE,
  NUM_SOUNDGEN_TYPE
} sound_generator_type;

typedef enum {
  C,        // 0
  C_SHARP,  // 1
  D,        // 2
  D_SHARP,  // 3
  E,        // 4
  F,        // 5
  F_SHARP,  // 6
  G,        // 7
  G_SHARP,  // 8
  A,        // 9
  A_SHARP,  // 10
  B,        // 11
  NUM_KEYS
} key;

typedef enum { MONO, LEGATO } legato_mode;

enum ProcessType { NO_PROCESS_TYPE, PATTERN_PROCESS, COMMAND_PROCESS };

enum ProcessTimerType {
  NO_PROCESS_TIMER_TYPE,
  EVERY,
  OSCILLATE,
  OVER,
  RAMP,
  WHILE,
};

enum ProcessPatternTarget {
  NO_PROCESS_PATTERN_TARGET,
  ENV,     // pattern contains values from environment
  VALUES,  // values in pattern to be applied to list of targets provided
};

typedef enum {
  NONE,
  MIDI_CONTROL_SYNTH_TYPE,
} midi_control_type;

typedef struct mixer_timing_info {
  double frames_per_midi_tick{0};
  double ms_per_midi_tick{0};
  double midi_ticks_per_ms{0};
  float bpm{0};

  double time_of_next_midi_tick{0};

  // perpetually incrementing counters
  int sixteenth_note_tick{0};
  int midi_tick{0};

  int loop_beat{0};
  bool loop_started{false};

  int cur_sample{0};  // inverse of SAMPLE RATE

  unsigned int key{0};
  unsigned int chord_progression_index{0};
  unsigned int chord{0};
  unsigned int chord_type{0};
  unsigned int octave{0};
  unsigned int notes[8] = {};
  unsigned int quantize{0};

  // informational for other sound generators
  double loop_len_in_frames{0};
  double loop_len_in_ticks{0};
  // these are in frames
  double size_of_thirtysecond_note{0};
  double size_of_sixteenth_note{0};
  double size_of_eighth_note{0};
  double size_of_quarter_note{0};

  bool has_started{false};
  bool is_start_of_loop{false};  // true for one sample during loop time
  bool is_thirtysecond{false};
  bool is_twentyfourth{false};
  bool is_sixteenth{false};
  bool is_twelth{false};
  bool is_eighth{false};
  bool is_sixth{false};
  bool is_quarter{false};
  bool is_third{false};
  bool is_midi_tick{false};

} mixer_timing_info;

typedef struct chord_midi_notes {
  int root;
  int third;
  int fifth;
  int seventh;
} chord_midi_notes;

enum {
  MAJOR_CHORD,
  MINOR_CHORD,
  DIMINISHED_CHORD,
  NUM_CHORD_TYPES,
};  // chord type

typedef enum event_type {
  TIME_MIDI_TICK,
  TIME_THIRTYSECOND_TICK,
  TIME_TWENTYFOURTH_TICK,
  TIME_SIXTEENTH_TICK,
  TIME_TWELTH_TICK,
  TIME_EIGHTH_TICK,
  TIME_SIXTH_TICK,
  TIME_QUARTER_TICK,
  TIME_THIRD_TICK,
  TIME_START_OF_LOOP_TICK,
  TIME_BPM_CHANGE,
  TIME_CHORD_CHANGE,
  SEQUENCER_NOTE,
} event_type;

typedef struct broadcast_event {
  unsigned int type;
  int sequencer_src;  // only for SEQUENCER_NOTE_EVENT
} broadcast_event;

typedef struct pattern_change_info {
  bool clear_previous;
  bool temporary;
} pattern_change_info;

struct StereoVal {
  double left;
  double right;

  StereoVal operator+(StereoVal const &obj) {
    StereoVal res;
    res.left = left + obj.left;
    res.right = right + obj.right;
    return res;
  }
};

enum {
  INTERNAL_SYNTH,
  EXTERNAL_DEVICE,
  EXTERNAL_OSC,
};  // source of midi event

class midi_event {
 public:
  friend std::ostream &operator<<(std::ostream &, const midi_event &);
  bool operator<(const midi_event &e) const {
    return original_tick < e.original_tick;
  }

  int source{};
  int event_type{};
  int data1{};
  int data2{};
  int playback_tick{};
  int original_tick{};
  int dur{0};
};

struct MusicalEvent {
  MusicalEvent() = default;
  MusicalEvent(std::string value)
      : MusicalEvent(value, /* midi velocity */ 127, /* duration in ms */ 300,
                     ENV) {}
  MusicalEvent(std::string value, ProcessPatternTarget target_type)
      : MusicalEvent(value, /* midi velocity */ 127, /* duration in ms */ 300,
                     target_type) {}
  MusicalEvent(std::string value, float velocity,
               ProcessPatternTarget target_type)
      : MusicalEvent(value, velocity, /* duration in ms */ 300, target_type) {}
  MusicalEvent(std::string value, float velocity, float duration,
               ProcessPatternTarget target_type)
      : value_{value},
        velocity_{velocity},
        duration_{duration},
        target_type_{target_type} {}
  std::string value_;
  float velocity_;
  float duration_;
  ProcessPatternTarget target_type_;
};

typedef midi_event midi_pattern[PPBAR];

typedef std::array<std::vector<midi_event>, PPBAR> MultiEventMidiPattern;
