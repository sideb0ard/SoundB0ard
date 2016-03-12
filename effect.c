#include <stdio.h>
#include <stdlib.h>

#include "effect.h"
#include "defjams.h"

EFFECT* new_effect(double duration)
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

  return e;
  }

