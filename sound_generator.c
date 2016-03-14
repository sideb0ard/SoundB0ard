#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "effect.h"
#include "sound_generator.h"

int add_delay_soundgen(SOUNDGEN* self, float duration)
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

  EFFECT* e = new_delay(duration);
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

int add_envelope_soundgen(SOUNDGEN* self, int env_len, int type)
{
  printf("Booya, adding a new envelope to SOUNDGEN!\n");
  ENVSTREAM **new_envelopes = NULL;
  if (self->envelopes_size <= self->envelopes_num) {
    if (self->envelopes_size == 0) {
      self->envelopes_size = DEFAULT_ARRAY_SIZE;
    } else {
    self->envelopes_size *= 2;
    }

    new_envelopes = realloc(self->envelopes, self->envelopes_size *
                          sizeof(ENVSTREAM*));
    if (new_envelopes == NULL) {
      printf("Ooh, burney - cannae allocate memory for new envelopes");
      return -1;
    } else {
      self->envelopes = new_envelopes;
    }
  }

  ENVSTREAM* e = new_envelope_stream(env_len, type);
  if ( e == NULL ) {
    perror("Couldn't create envelope");
    return -1;
  }
  self->envelopes[self->envelopes_num] = e;
  self->envelopes_on = 1;
  printf("done adding envelope\n");
  return self->envelopes_num++;
}

float envelopor(SOUNDGEN* self, float val)
{

  if (self->envelopes_num > 0) {
    for ( int i = 0; i < self->envelopes_num; i++ ) {
      double mix_env = envelope_stream_tick(self->envelopes[i]);
      if ( self->envelopes[i]->started ) {
        val *= mix_env;
      } else if ( mix_env == 1 ) {
        self->envelopes[i]->started = 1;
        val *= mix_env;
      }
    }
  }
  return val;
}
