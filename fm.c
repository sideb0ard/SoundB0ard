#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "fm.h"
#include "table.h"

extern GTABLE *sine_table;

FM* new_fm(double modf, double carf)
{
  FM* fm;
  fm = (FM *) calloc(1, sizeof(FM));
  if (fm == NULL)
    return NULL;
  fm->mod_osc = new_oscil(modf, sine_table);
  fm->car_osc = new_oscil(carf, sine_table);

  fm->vol = 0.0;
  fm->mod_osc->sound_generator.setvol(fm->mod_osc, 0.7);
  fm->car_osc->sound_generator.setvol(fm->car_osc, 0.7);
  //fm->car_osc->voladj(fm->car_osc, 0.7);

  fm->sound_generator.gennext = &fm_gennext;
  fm->sound_generator.status = &fm_status;
  fm->sound_generator.setvol = &fm_setvol;
  fm->sound_generator.getvol = &fm_getvol;
  
  return fm;
}

void fm_setvol(void *self, double v)
{
  //printf("FM SETVOL CALL\n");
  FM *fm = (FM *) self;
  if (v < 0.0 || v > 1.0) {
    return;
  }
  fm->vol = v;
}

double fm_getvol(void *self)
{
  printf("FM GET VOL!\n");
  FM *fm = (FM *) self;
  return fm->vol;
}

void fm_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
  FM *fm = (FM *) self;

  //double* local_osc_car_vals = calloc(framesPerBuffer, sizeof(double));
  //fm->car_osc->sound_generator.gennext(fm->car_osc, local_osc_car_vals, framesPerBuffer);

  //double* local_osc_mod_vals = calloc(framesPerBuffer, sizeof(double));
  //fm->mod_osc->sound_generator.gennext(fm->mod_osc, local_osc_car_vals, framesPerBuffer);

  for (int i = 0; i < framesPerBuffer; i++) {
    //double val = local_osc_car_vals[i];
    double val = fm->car_osc->gennext(fm->car_osc);
    double mod_val = 100 * fm->mod_osc->gennext(fm->mod_osc);
    fm->car_osc->incradj(fm->car_osc, TABRAD * (fm->car_osc->freq + mod_val));

    //return fm->vol * val;
    frame_vals[i] += fm->vol * val;
  }
  //free(local_osc_car_vals);
  //free(local_osc_mod_vals);
}

void fm_status(void *self, char *status_string)
{
  FM *fm = (FM *) self;
  snprintf(status_string, 119, ANSI_COLOR_RED "FM! modulator: %.2f(freq) %.2f(phase) // carrier: %.2f %.2f // vol: %.2f" ANSI_COLOR_RESET, fm->mod_osc->freq, fm->mod_osc->curphase, fm->car_osc->freq, fm->car_osc->curphase, fm->vol);
}

void mfm(void *self, char *osc, double val)
{
  FM *fm = (FM *) self;
  if (!strcmp(osc, "mod")) {
    freqfunc(fm->mod_osc, val);
  } else if (!strcmp(osc, "car")) {
    freqfunc(fm->car_osc, val);
  } else {
    return;
  }
}
