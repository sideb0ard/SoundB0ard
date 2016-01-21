#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "oscil.h"

OSCIL* new_oscil(uint32_t freq, tickfunc tic)
{
  OSCIL *p_osc;

  p_osc = calloc(1, sizeof(OSCIL));
  if (p_osc == NULL) 
    return NULL;
  p_osc->freq = freq;
  p_osc->curphase = 0.0;
  p_osc->incr = FREQRAD * freq;

  p_osc->tick = tic;

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

double sqtick(OSCIL *p_osc)
{
  double val;
  if (p_osc->curphase <= M_PI)
    val = 1.0;
  else
    val = -1;
  p_osc->curphase += p_osc->incr;
  if (p_osc->curphase >= TWO_PI)
    p_osc->curphase -= TWO_PI;
  if (p_osc->curphase < 0.0)
    p_osc->curphase += TWO_PI;
  return val;
}

double sawdtick(OSCIL *p_osc)
{
  double val;
  val = 1.0 - 2.0 * (p_osc->curphase * (1.0 / TWO_PI));
  p_osc->curphase += p_osc->incr;
  if (p_osc->curphase >= TWO_PI)
    p_osc->curphase -= TWO_PI;
  if (p_osc->curphase < 0.0)
    p_osc->curphase += TWO_PI;
  return val;
}

double sawutick(OSCIL *p_osc)
{
  double val;
  val = (2.0 * (p_osc->curphase * (1.0 / TWO_PI))) - 1.0;
  p_osc->curphase += p_osc->incr;
  if (p_osc->curphase >= TWO_PI)
    p_osc->curphase -= TWO_PI;
  if (p_osc->curphase < 0.0)
    p_osc->curphase += TWO_PI;
  return val;
}

double tritick(OSCIL *p_osc)
{
  double val;
  val = (2.0 * (p_osc->curphase * (1.0 / TWO_PI))) - 1.0;
  if(val < 0.0)
    val = -val;
  val = 2.0 * (val - 0.5);
  p_osc->curphase += p_osc->incr;
  if (p_osc->curphase >= TWO_PI)
    p_osc->curphase -= TWO_PI;
  if (p_osc->curphase < 0.0)
    p_osc->curphase += TWO_PI;
  return val;
}

void status(OSCIL *p_osc, char *status_string)
{
  //printf("Calling me up! I gots an OSC at %p\n", p_osc);
  //printf("FREQy! %f\n", p_osc->freq);
  //sprintf(sstatus, "freq: %f curphase: %f", p_osc->freq, p_osc->curphase);
  sprintf(status_string, "freq: %f", p_osc->freq);

  //return "jobbie";
}
