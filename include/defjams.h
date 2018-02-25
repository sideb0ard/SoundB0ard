#pragma once

#include <stdbool.h>

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (64)

#define NUM_STATEGIES 100

#ifndef M_PI
#define M_PI (3.14159265358897932)
#endif

#define SIZE_OF_WURD 256
#define NUM_WURDS 25
#define DEFAULT_BPM 110

#define DEFAULT_ENV_LENGTH 8.0 // two bars Envelope

#define DEFAULT_ARRAY_SIZE 4

#define MAX_PS_STRING_SZ 4096 // arbitrary

#define ENVIRONMENT_ARRAY_SIZE 128
#define ENVIRONMENT_KEY_SIZE 128

#define PPQN 960 // Pulses Per Quarter Note // one beat
#define PPSIXTEENTH (PPQN / 4)
#define PPTWENTYFOURTH (PPQN / 6)
#define PPTHIRTYSECOND (PPQN / 8)
#define PPBAR (PPQN * 4) // Pulses per loop/bar - i.e 4 * beats

#define SEQUENCER_PATTERN_LEN 16 // 16 1/4 notes i.e. one bar

#define TWO_PI (2.0 * M_PI)
#define FREQRAD (TWO_PI / SAMPLE_RATE)

#define TABLEN (1024.00)
#define TABRAD (TABLEN / SAMPLE_RATE)

#define NHARMS 7 // number of harmonics

#define NUM_COMPAT_NOTES 6

// #define sparkchars L"\u2581\u2582\u2583\u2585\u2586\u2587"

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

#define WANSI_COLOR_BLUE L"\x1b[34m"
#define WANSI_COLOR_CYAN L"\x1b[36m"
#define WANSI_COLOR_GREEN L"\x1b[32m"
#define WANSI_COLOR_GREEN_TOO L"\x1b[38;5;34m"
#define WANSI_COLOR_MAGENTA L"\x1b[35m"
#define WANSI_COLOR_PINK L"\x1b[38;5;13m"
#define WANSI_COLOR_RED L"\x1b[31m"
#define WANSI_COLOR_DEEP_RED L"\x1b[38;5;124m"
#define WANSI_COLOR_RESET L"\x1b[0m"
#define WANSI_COLOR_WHITE L"\x1b[37m"
#define WANSI_COLOR_YELLOW L"\x1b[33m"
#define WCOOL_COLOR_GREEN L"\x1b[38;5;47m"
#define WCOOL_COLOR_YELLOW L"\x1b[38;5;226m"
#define WCOOL_COLOR_PINK L"\x1b[38;5;200m"
#define WCOOL_COLOR_MAUVE L"\x1b[38;5;135m"
#define WCOOL_COLOR_ORANGE L"\x1b[38;5;202m"

#define NADA -999

#define MAX_LENGTH_VAR_VALUE 128

enum
{
    LEFT,
    RIGHT
};

enum
{
    MIDI_ON = 144,
    MIDI_OFF = 128,
    MIDI_CONTROL = 176,
    MIDI_PITCHBEND = 224,
};

enum
{
    MIDI_KNOB_MODE_ONE,
    MIDI_KNOB_MODE_TWO,
    MIDI_KNOB_MODE_THREE,
    MAX_NUM_KNOB_MODES,
};

enum
{
    KEY_MODE_ONE,
    KEY_MODE_TWO,
    MAX_NUM_KEY_MODES,
};

typedef enum {
    MINISYNTH_TYPE,
    DIGISYNTH_TYPE,
    DXSYNTH_TYPE,
    LOOPER_TYPE,
    BITWIZE_TYPE,
    SEQUENCER_TYPE,
    SYNTHDRUM_TYPE,
    NUM_SOUNDGEN_TYPE
} sound_generator_type;

typedef enum {
    C_MAJOR,       // midi 24
    G_MAJOR,       //  31
    D_MAJOR,       // 26
    A_MAJOR,       // 33
    E_MAJOR,       // 28
    B_MAJOR,       // 35
    F_SHARP_MAJOR, // 30
    D_FLAT_MAJOR,  // 25
    A_FLAT_MAJOR,  // 32
    E_FLAT_MAJOR,  // 27
    B_FLAT_MAJOR,  // 34
    F_MAJOR,       // 29
    NUM_KEYS
} key_type;

typedef enum { MONO, LEGATO } legato_mode;

typedef enum {
    NONE,
    SYNTH,
} midi_control_type;

typedef struct mixer_timing_info
{
    int frames_per_midi_tick;
    double midi_ticks_per_ms;

    double time_of_next_midi_tick;

    int sixteenth_note_tick;
    int midi_tick;

    int loop_beat;
    bool loop_started;

    int cur_sample; // inverse of SAMPLE RATE

    // informational for other sound generators
    unsigned int loop_len_in_frames;
    unsigned int loop_len_in_ticks;
    unsigned int size_of_thirtysecond_note;
    unsigned int size_of_sixteenth_note;
    unsigned int size_of_eighth_note;
    unsigned int size_of_quarter_note;

    bool has_started;
    bool start_of_loop; // true for one sample during loop time
    bool is_thirtysecond;
    bool is_sixteenth;
    bool is_eighth;
    bool is_quarter;
    bool is_midi_tick;
} mixer_timing_info;

typedef enum time_event {
    TIME_MIDI_TICK,
    TIME_THIRTYSECOND_TICK,
    TIME_SIXTEENTH_TICK,
    TIME_EIGHTH_TICK,
    TIME_QUARTER_TICK,
    TIME_START_OF_LOOP_TICK,
} time_event;

typedef struct stereo_val
{
    double left;
    double right;
} stereo_val;

typedef struct midi_event
{
    unsigned event_type;
    unsigned data1;
    unsigned data2;
    bool delete_after_use;
} midi_event;

typedef midi_event midi_pattern[PPBAR];

typedef enum {
    MIDI_PATTERN, // numbers
    NOTE_PATTERN, // alphanums
    BEAT_PATTERN, // env variables
    STEP_PATTERN, // individual step sequencers
} pattern_type;
