#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "wave.h"

OSCIL* new_oscil(uint32_t freq)
{
  OSCIL *p_osc;

  p_osc = calloc(1, sizeof(OSCIL));
  if (p_osc == NULL) 
    return NULL;
  p_osc->freq = freq;
  p_osc->curphase = 0.0;
  p_osc->incr = FREQRAD * freq;

  return p_osc;
}

double sinetick(OSCIL *p_osc)
{
  double val;
  val = sin(p_osc->curphase);
  p_osc->curphase += p_osc->incr;
  if (p_osc->curphase >= TWO_PI)
    p_osc->curphase -= TWO_PI;
  if (p_osc->curphase < 0.0)
    p_osc->curphase += TWO_PI;
  return val;
}

void status(OSCIL *p_osc, char *status_string)
{
  printf("Calling me up! I gots an OSC at %p\n", p_osc);
  printf("FREQy! %f\n", p_osc->freq);
  //sprintf(sstatus, "freq: %f curphase: %f", p_osc->freq, p_osc->curphase);
  sprintf(status_string, "freq: %f", p_osc->freq);

  //return "jobbie";
}
