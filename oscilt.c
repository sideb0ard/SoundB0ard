#include <stdlib.h>

#include "defjams.h"
#include "oscilt.h"
#include "table.h"

extern GTABLE *gtable;

OSCILT* new_oscilt(double freq, ttickfunc tic)
{
  OSCILT* p_osc;
  p_osc = (OSCILT *) calloc(1, sizeof(OSCILT));
  if (p_osc == NULL)
    return NULL;
  p_osc->osc.freq = freq;
  p_osc->osc.incr = TABRAD * freq;
  printf("NEW OSCILT! - TABRAD IS %f // freq is %f\n", TABRAD, freq);
  p_osc->gtable = gtable;
  p_osc->dtablen = (double) TABLEN;

  p_osc->tick = tic;

  return p_osc;
}

double tabtick(OSCILT* p_osc)
{
  printf("TAB tick called!\n");
  int index = (int) (p_osc->osc.curphase);
  double dtablen = p_osc->dtablen, curphase = p_osc->osc.curphase;
  double* table = p_osc->gtable->table;
  curphase += p_osc->osc.incr;
  while ( curphase >= dtablen)
    curphase += dtablen;
  while ( curphase < 0.0)
    curphase -= dtablen;
  p_osc->osc.curphase = curphase;
  return table[index];
}

double tabitick(OSCILT* p_osc) // interpolating
{
  //printf("TAB tick called!\n");
  int base_index = (int) (p_osc->osc.curphase);
  //printf("BASE INDEX is %d!\n", base_index);
  unsigned long next_index = base_index + 1;
  //printf("NEXT INDEX is %lu!\n", next_index);
  double frac, slope, val;
  double dtablen = p_osc->dtablen, curphase = p_osc->osc.curphase;
  double* table = p_osc->gtable->table;
 
  frac = curphase - base_index;
  //printf("FRAC is %f\n", frac);
  val = table[base_index];
  //printf("VAL is %f\n", val);
  //printf("NEXT VAL is %f\n", table[next_index]);
  slope = table[next_index] - val;
  //printf("SLOPE VAL is %f\n", slope);

  val += (frac * slope);
  curphase += p_osc->osc.incr;
  //printf("INCR IS %f\n", p_osc->osc.incr);

  while ( curphase >= dtablen)
    curphase -= dtablen;
  while ( curphase < 0.0)
    curphase += dtablen;

  p_osc->osc.curphase = curphase;
  //printf("VAL Is %f\n", val);
  return val;
}

void tstatus(OSCILT *p_osc, char *status_string)
{
  //printf("Calling me up! I gots an OSC at %p\n", p_osc);
  //printf("FREQy! %f\n", p_osc->freq);
  //sprintf(sstatus, "freq: %f curphase: %f", p_osc->freq, p_osc->curphase);
  sprintf(status_string, "freq: %f incr: %f cur: %f", p_osc->osc.freq, p_osc->osc.incr, p_osc->osc.curphase);

  //return "jobbie";
}
