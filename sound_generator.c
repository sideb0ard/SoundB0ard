#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "effect.h"
#include "sound_generator.h"

int add_effect_soundgen(SOUNDGEN* self, float duration)
{
  printf("Booya, adding a new effect to SOUNDGEN: %f!\n", duration);
  EFFECT **new_effects = NULL;
  if (self->effects_size <= self->effects_num) {
    if (self->effects_size == 0) {
      self->effects_size = DEFAULT_ARRAY_SIZE;
    } else {
    self->effects_size *= 2;
    }

    new_effects = realloc(self->effects, self->effects_size *
                          sizeof(EFFECT*));
    if (new_effects == NULL) {
      printf("Ooh, burney - cannae allocate memory for new effects");
      return -1;
    } else {
      self->effects = new_effects;
    }
  }

  EFFECT* e = new_effect(duration);
  if ( e == NULL ) {
    perror("Couldn't create effect");
    return -1;
  }
  self->effects[self->effects_num] = e;
  self->effects_on = 1;
  printf("done adding effect\n");
  return self->effects_num++;
}

float effector(SOUNDGEN* self, float val)
{
  double delay_val_copy = val;

  if (self->effects_num > 0) {

    int delay_p = self->effects[0]->buf_p;
    double *delay = self->effects[0]->buffer;

    if (self->effects_on) {
      val += delay[delay_p];
    }

    delay[delay_p++] = delay_val_copy*0.5;

    if (delay_p >= self->effects[0]->buf_length) delay_p = 0;

    self->effects[0]->buf_p = delay_p;
  }
  return val;
}

