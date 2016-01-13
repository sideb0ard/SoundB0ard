/* wave.h */

#include <stdint.h>

typedef struct t_oscil
{
  double twopiovrsr;
  double curfreq;
  double curphase;
  double incr;
} OSCIL;

OSCIL* new_oscil(uint32_t srate);
double sinetick(OSCIL *p_osc, double freq);
