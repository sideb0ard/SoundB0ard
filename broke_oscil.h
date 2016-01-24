/* oscil.h */

#ifndef OSCIL_H
#define OSCIL_H

#include <stdint.h>

#include "defjams.h"

typedef struct t_oscil OSCIL;
typedef double (*tickfunc)( OSCIL* osc);

typedef struct t_oscil
{
  double freq;
  double curphase;
  double incr;
  double table[TABLEN + 1]; // +1 is a guard point

  tickfunc tick;
} OSCIL;

OSCIL* new_oscil(uint32_t freq, tickfunc tic);

double sinetick(OSCIL *p_osc);
double sawdtick(OSCIL *p_osc);
double sawutick(OSCIL *p_osc);
double tritick(OSCIL *p_osc);
double sqtick(OSCIL *p_osc);

void status(OSCIL *p_osc, char *status_string);

#endif // OSCIL_H
