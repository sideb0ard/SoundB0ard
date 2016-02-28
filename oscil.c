#include <stdlib.h>

#include "defjams.h"
#include "oscil.h"
#include "sound_generator.h"
#include "table.h"

OSCIL* new_oscil(double freq, GTABLE *gt)
{
  OSCIL* p_osc;
  p_osc = (OSCIL *) calloc(1, sizeof(OSCIL));
  if (p_osc == NULL)
    return NULL;
  p_osc->freq = freq;
  p_osc->incr = TABRAD * freq;
  p_osc->vol = 0.0;
  printf("NEW OSCILT! - TABRAD IS %f // freq is %f\n", TABRAD, freq);
  p_osc->gtable = gt;
  p_osc->dtablen = (double) TABLEN;

  p_osc->sound_generator.gennext = &oscil_gennext;
  p_osc->sound_generator.status = &oscil_status;
  p_osc->sound_generator.getvol = &oscil_getvol;
  p_osc->sound_generator.setvol = &oscil_setvol;

  //p_osc->voladj = &volfunc;
  p_osc->freqadj = &freqfunc;
  p_osc->incradj = &incrfunc;
  p_osc->gennext = &oscil_gennext_single;

  return p_osc;
}

void incrfunc(OSCIL* p_osc, double v)
{
  p_osc->incr = v;
}

double oscil_getvol(void *self)
{
  OSCIL *p_osc = (OSCIL *) self;
  return p_osc->vol;
}

void oscil_setvol(void *self, double v)
{
  OSCIL *p_osc = (OSCIL *) self;
  if (v < 0.0 || v > 1.0) {
    return;
  }
  p_osc->vol = v;
}

void freqfunc(OSCIL* p_osc, double f)
{
  p_osc->freq = f;
  p_osc->incr = TABRAD * f;
}

double oscil_gennext_single(void *self)
{
 OSCIL *p_osc = (OSCIL *) self;
 int base_index = (int) (p_osc->curphase);
 unsigned long next_index = base_index + 1;
 double frac, slope, val;
 double dtablen = p_osc->dtablen, curphase = p_osc->curphase;
 double* table = p_osc->gtable->table;
 double vol = p_osc->vol;
 
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
 return val * vol;
}

void oscil_gennext(void* self, double* frame_vals, int framesPerBuffer) // interpolating
{
 OSCIL *p_osc = (OSCIL *) self;

  for (int i = 0; i < framesPerBuffer; i++) {
    int base_index = (int) (p_osc->curphase);
    unsigned long next_index = base_index + 1;
    double frac, slope, val;
    double dtablen = p_osc->dtablen, curphase = p_osc->curphase;
    double* table = p_osc->gtable->table;
    double vol = p_osc->vol;
 
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
    //return val * vol;
    frame_vals[i] += val * vol;
  }
}

void oscil_status(void *self, char *status_string)
{
  OSCIL *p_osc = self;
  snprintf(status_string, 119, ANSI_COLOR_YELLOW "freq: %f vol: %f incr: %f cur: %f" ANSI_COLOR_RESET, p_osc->freq, p_osc->vol, p_osc->incr, p_osc->curphase);
}
