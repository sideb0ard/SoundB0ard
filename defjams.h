#pragma once

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (64)

#ifndef M_PI
#define M_PI (3.14159265358897932)
#endif

#define DEFAULT_BPM 80

#define DEFAULT_ENV_LENGTH 8.0 // two bars Envelope

#define DEFAULT_ARRAY_SIZE 4

#define PPQN 960       // Pulses Per Quarter Note // one beat
#define PPS (PPQN / 4) // pulses per SIXTEENTH note, i.e. drum machine hit
#define PPL (PPQN * 4) // Pulses per loop/bar - i.e 4 * beats
#define PPNS (PPL * 2) // Pulses per NanoSynth recording loop, i.e 2 loops/bars

#define DRUM_PATTERN_LEN 16 // 16 1/4 notes i.e. one bar

#define TWO_PI (2.0 * M_PI)
#define FREQRAD (TWO_PI / SAMPLE_RATE)

#define TABLEN (1024.00)
#define TABRAD (TABLEN / SAMPLE_RATE)

#define NHARMS 7 // number of harmonics

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

typedef enum { OFF, ON } onoff;

typedef enum { UP, DOWN } direction;

typedef enum {
    NANOSYNTH_TYPE,
    SAMPLER_TYPE,
    BITWIZE_TYPE,
    DRUM_TYPE,
    ALGORITHM_TYPE,
    CHAOSMONKEY_TYPE,
} sound_generator_type;

typedef enum { SINE, TRI, SQUARE, SAW_U, SAW_D } wave_type;
