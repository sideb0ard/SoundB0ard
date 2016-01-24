#ifndef OSCILT_H
#define OSCILT_H

#include <stdio.h>
#include "oscil.h"
#include "table.h"

typedef struct t_tab_oscil OSCILT;
typedef double (*ttickfunc) (OSCILT* tosc);

typedef struct t_tab_oscil
{
  OSCIL osc;
  const GTABLE* gtable;
  double dtablen;

  ttickfunc tick;
} OSCILT;

typedef double (*ttickfunc) (OSCILT* tosc);

OSCILT* new_oscilt(double freq, ttickfunc tic);
double tabtick(OSCILT* p_osc);
double tabitick(OSCILT* p_osc);

void tstatus(OSCILT *p_osc, char *status_string);

#endif // OSCILT_H
