#pragma once

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (64)

#define NUM_STATEGIES 100

#ifndef M_PI
#define M_PI (3.14159265358897932)
#endif

#define DEFAULT_BPM 110

#define DEFAULT_ENV_LENGTH 8.0 // two bars Envelope

#define DEFAULT_ARRAY_SIZE 4

#define MAX_PS_STRING_SZ 204800 // arbitrary

#define ENVIRONMENT_ARRAY_SIZE 128
#define ENVIRONMENT_KEY_SIZE 128

#define SYNTH_NUM_BARS 2
#define PPQN 960 // Pulses Per Quarter Note // one beat
#define PPSIXTEENTH (PPQN / 4)
#define PPTWENTYFOURTH (PPQN / 6)
// #define PPTHIRTYSECOND (PPQN / 8)
#define PPBAR (PPQN * 4) // Pulses per loop/bar - i.e 4 * beats
#define PPNS                                                                   \
    (PPBAR *                                                                   \
     SYNTH_NUM_BARS) // Pulses per NanoSynth recording loop, i.e 2 loops/bars

#define SEQUENCER_PATTERN_LEN 16 // 16 1/4 notes i.e. one bar

#define TWO_PI (2.0 * M_PI)
#define FREQRAD (TWO_PI / SAMPLE_RATE)

#define TABLEN (1024.00)
#define TABRAD (TABLEN / SAMPLE_RATE)

#define NHARMS 7 // number of harmonics

// #define sparkchars L"\u2581\u2582\u2583\u2585\u2586\u2587"

#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_WHITE "\x1b[37m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define COOL_COLOR_GREEN "\x1b[38;5;47m"
#define COOL_COLOR_YELLOW "\x1b[38;5;226m"
#define COOL_COLOR_PINK "\x1b[38;5;200m"
#define COOL_COLOR_MAUVE "\x1b[38;5;135m"

#define WANSI_COLOR_BLUE L"\x1b[34m"
#define WANSI_COLOR_CYAN L"\x1b[36m"
#define WANSI_COLOR_GREEN L"\x1b[32m"
#define WANSI_COLOR_MAGENTA L"\x1b[35m"
#define WANSI_COLOR_RED L"\x1b[31m"
#define WANSI_COLOR_RESET L"\x1b[0m"
#define WANSI_COLOR_WHITE L"\x1b[37m"
#define WANSI_COLOR_YELLOW L"\x1b[33m"
#define WCOOL_COLOR_GREEN L"\x1b[38;5;47m"
#define WCOOL_COLOR_YELLOW L"\x1b[38;5;226m"
#define WCOOL_COLOR_PINK L"\x1b[38;5;200m"
#define WCOOL_COLOR_MAUVE L"\x1b[38;5;135m"

enum { KICK, SNARE };

typedef enum {
    MIDI_KNOB_MODE_ONE,
    MIDI_KNOB_MODE_TWO,
    MIDI_KNOB_MODE_THREE,
    MAX_NUM_KNOB_MODES,
} midi_mode; // to switch control knob routing

typedef enum {
    KEY_MODE_ONE,
    KEY_MODE_TWO,
    MAX_NUM_KEY_MODES,
} key_mode; // to switch key control routing

typedef enum {
    SYNTH_TYPE,
    LOOPER_TYPE,
    BITWIZE_TYPE,
    SEQUENCER_TYPE,
    SYNTHDRUM_TYPE,
    ALGORITHM_TYPE,
    CHAOSMONKEY_TYPE,
    SPORK_TYPE,
    NUM_SOUNDGEN_TYPE
} sound_generator_type;

typedef enum { MONO, LEGATO } legato_mode;
typedef enum {
    NONE,
    SYNTH,
    DELAYFX,
    MIDISPORK,
    MIDISEQUENCER,
    MIDISYNTHDRUM,
    MIDILOOPER
} midi_control_type;
