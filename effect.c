#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "effect.h"
#include "defjams.h"

EFFECT* new_delay(double duration)
{
  EFFECT* e;
  e = (EFFECT*) calloc(1, sizeof(EFFECT));
  if ( e == NULL )
    return NULL;

  double* buffer;
  int buf_length = (int) (duration * SAMPLE_RATE);
  buffer = (double*) calloc( buf_length, sizeof(double));
  if ( buffer == NULL ) {
    perror("Couldn't allocate effect buffer");
    free(e);
    return NULL;
  }
  e->buffer = buffer;
  e->buf_length = buf_length;
  e->type = DELAY;

  return e;
}

EFFECT* new_freq_pass(double freq, effect_type pass_type)
{
  EFFECT* e;
  e = (EFFECT*) calloc(1, sizeof(EFFECT));
  if ( e == NULL )
    return NULL;

  double* buffer;
  int buf_length = 1;
  buffer = (double*) calloc( buf_length, sizeof(double));
  if ( buffer == NULL ) {
    perror("Couldn't allocate effect buffer");
    free(e);
    return NULL;
  }

  double costh = 2. - cos(TWO_PI * freq / SAMPLE_RATE);
  e->costh = costh;

  switch (pass_type) {
    case LOWPASS :
      e->coef = sqrt(costh*costh - 1.) - costh;
      e->type = LOWPASS;
      break;
    case HIGHPASS :
      e->coef = costh - sqrt(costh*costh - 1.);
      e->type = HIGHPASS;
      break;
    default:
      break;
  }

  e->buffer = buffer;
  e->buf_length = buf_length;

  return e;
}
