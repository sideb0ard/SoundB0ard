#include <stdlib.h>

#include "defjams.h"
#include "oscil.h"
#include "table.h"

OSCIL* new_oscil(double freq, GTABLE *gt)
{
  OSCIL* p_osc;
  p_osc = (OSCIL *) calloc(1, sizeof(OSCIL));
  if (p_osc == NULL)
    return NULL;
  p_osc->freq = freq;
  p_osc->incr = TABRAD * freq;
  printf("NEW OSCILT! - TABRAD IS %f // freq is %f\n", TABRAD, freq);
  p_osc->gtable = gt;
  p_osc->dtablen = (double) TABLEN;

  p_osc->tick = &tabitick;

  return p_osc;
}

// TODO: do i need this?
double tabtick(OSCIL* p_osc)
{
  printf("TAB tick called!\n");
  int index = (int) (p_osc->curphase);
  double dtablen = p_osc->dtablen, curphase = p_osc->curphase;
  double* table = p_osc->gtable->table;
  curphase += p_osc->incr;
  while ( curphase >= dtablen)
    curphase += dtablen;
  while ( curphase < 0.0)
    curphase -= dtablen;
  p_osc->curphase = curphase;
  return table[index];
}

double tabitick(OSCIL* p_osc) // interpolating
{
  int base_index = (int) (p_osc->curphase);
  unsigned long next_index = base_index + 1;
  double frac, slope, val;
  double dtablen = p_osc->dtablen, curphase = p_osc->curphase;
  double* table = p_osc->gtable->table;
 
  frac = curphase - base_index;
  val = table[base_index];
  slope = table[next_index] - val;

  val += (frac * slope);
  curphase += p_osc->incr;

  while ( curphase >= dtablen)
    curphase -= dtablen;
  while ( curphase < 0.0)
    curphase += dtablen;

  p_osc->curphase = curphase;
  return val;
}

void status(OSCIL *p_osc, char *status_string)
{
  sprintf(status_string, "freq: %f incr: %f cur: %f", p_osc->freq, p_osc->incr, p_osc->curphase);
}
