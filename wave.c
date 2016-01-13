#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "defjams.h"
#include "wave.h"

OSCIL* new_oscil(uint32_t srate)
{
  OSCIL *p_osc;

  p_osc = calloc(1, sizeof(OSCIL));
  if (p_osc == NULL) 
    return NULL;
  p_osc->twopiovrsr = TWO_PI / (double) srate;
  p_osc->curfreq = 0.0;
  p_osc->curphase = 0.0;
  p_osc->incr = 0.0;

  return p_osc;
}

double sinetick(OSCIL *p_osc, double freq)
{
  double val;
  val = sin(p_osc->curphase);
  if (p_osc->curfreq != freq) {
    p_osc->curfreq = freq;
    p_osc->incr = p_osc->twopiovrsr * freq;
  }
  p_osc->curphase += p_osc->incr;
  if (p_osc->curphase >= TWO_PI)
    p_osc->curphase -= TWO_PI;
  if (p_osc->curphase < 0.0)
    p_osc->curphase += TWO_PI;
  return val;
}
