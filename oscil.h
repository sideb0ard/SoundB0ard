/* oscil.h */

#ifndef WAVE_H
#define WAVE_H

#include <stdint.h>

typedef struct t_oscil OSCIL;
typedef double (*tickfunc)( OSCIL* osc);

typedef struct t_oscil
{
  double freq;
  double curphase;
  double incr;

  tickfunc tick;
} OSCIL;

OSCIL* new_oscil(uint32_t freq, tickfunc tic);

double sinetick(OSCIL *p_osc);
double sawdtick(OSCIL *p_osc);
double sawutick(OSCIL *p_osc);
double tritick(OSCIL *p_osc);
double sqtick(OSCIL *p_osc);

void status(OSCIL *p_osc, char *status_string);

#endif // WAVE_H
