#include <stdlib.h>

#include "defjams.h"
#include "mixer.h"
#include "bitwize.h"
#include "sound_generator.h"

extern mixer *mixr;

BITWIZE* new_bitwize()
{
  BITWIZE* p_bitwize;
  p_bitwize = (BITWIZE *) calloc(1, sizeof(BITWIZE));
  if (p_bitwize == NULL)
    return NULL;
  p_bitwize->vol = 0.2;
  p_bitwize->incr = 0;
  printf("NEW BITWIZET!\n");

  p_bitwize->sound_generator.gennext = &bitwize_gennext;
  p_bitwize->sound_generator.status = &bitwize_status;
  // TODO
  p_bitwize->sound_generator.getvol = &bitwize_getvol;
  p_bitwize->sound_generator.setvol = &bitwize_setvol;
  p_bitwize->sound_generator.type = BITWIZE_TYPE;

  return p_bitwize;
}

double bitwize_getvol(void *self)
{
  BITWIZE *p_bitwize = (BITWIZE *) self;
  return p_bitwize->vol;
}

void bitwize_setvol(void *self, double v)
{
  BITWIZE *p_bitwize = (BITWIZE *) self;
  if (v < 0.0 || v > 1.0) {
    return;
  }
  p_bitwize->vol = v;
}

double bitwize_gennext(void* self)
{
  BITWIZE *p_bitwize = (BITWIZE *) self;
  int t = p_bitwize->incr++;
  double vol = p_bitwize->vol;
  //char val = t * (( t >> 9| t >> 13 ) & 25 & t >> 6);
  //char val = ( t >> 7 | t | t >> 6) * 10 + 4* ((t & (t>>13))| t >> 6);
  //char val = ( t * ( t >> 5 | t >> 8 )) >> ( t>> 16 );
  char val = ( t * ( t >> 5 | t >> 8 )) >> ( t>> 16 );

  double scale_val = (2.0 / 256 * val);
  //printf("val : %f\n", scale_val);
  return scale_val * vol;
}

void bitwize_status(void *self, char *status_string)
{
  BITWIZE *p_bitwize = self;
  snprintf(status_string, 119, ANSI_COLOR_YELLOW "vol: %f incr: %f" ANSI_COLOR_RESET, p_bitwize->vol, p_bitwize->incr);
}
