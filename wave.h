/* wave.h */

#ifndef WAVE_H
#define WAVE_H

#include <stdint.h>

typedef struct t_oscil
{
  double freq;
  double curphase;
  double incr;
} OSCIL;

OSCIL* new_oscil(uint32_t freq);

double sinetick(OSCIL *p_osc);
void status(OSCIL *p_osc, char *status_string);

#endif // WAVE_H
