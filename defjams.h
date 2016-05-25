#ifndef DEFJAMS_H
#define DEFJAMS_H

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

#ifndef M_PI
#define M_PI  (3.14159265358897932)
#endif

#define DEFAULT_BPM 80

#define DEFAULT_ENV_LENGTH 8.0 // two bars Envelope

#define DEFAULT_ARRAY_SIZE 4

#define TWO_PI (2.0 * M_PI)
#define FREQRAD (TWO_PI / SAMPLE_RATE)

#define TABLEN (1024.00)
#define TABRAD (TABLEN / SAMPLE_RATE)

#define NHARMS 7 // number of harmonics

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define COOL_COLOR_GREEN   "\x1b[38;5;47m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef enum {
  UP,
  DOWN
} direction;

typedef enum {
  OSCIL_TYPE,
  FM_TYPE,
  SAMPLER_TYPE,
  BITWIZE_TYPE,
  DRUM_TYPE
} sound_generator_type;

#endif
