#include <stdlib.h>

#include "defjams.h"
#include "fm.h"
#include "table.h"

extern GTABLE *sine_table;

float fm_gen_next(FM *fm)
{
  //float val = fm->cmod_osc->tick(fm->cmod_osc) * fm->fmod_osc->tick(fm->fmod_osc);
  float val = fm->cmod_osc->tick(fm->cmod_osc);
  float mod_val = 100 * fm->fmod_osc->tick(fm->fmod_osc);
  fm->cmod_osc->incradj(fm->cmod_osc, TABRAD * (fm->cmod_osc->freq + mod_val));

  return val;
}

FM* new_fm(double fmod, double cmod)
{
  FM* fm;
  fm = (FM *) calloc(1, sizeof(FM));
  if (fm == NULL)
    return NULL;
  fm->fmod_osc = new_oscil(fmod, sine_table);
  fm->cmod_osc = new_oscil(cmod, sine_table);

  fm->fmod_osc->voladj(fm->fmod_osc, 0.7);
  fm->cmod_osc->voladj(fm->cmod_osc, 0.7);

  fm->gen_next = &fm_gen_next;
  
  return fm;
}

void fm_status(FM *fm)
{
  printf("FM! fmod: %f %f // cmod: %f %f\n", fm->fmod_osc->freq, fm->fmod_osc->curphase, fm->cmod_osc->freq, fm->cmod_osc->curphase);
}
