#ifndef EFFECT_H
#define EFFECT_H

typedef struct {
  double* buffer;
  int buf_p;
  int buf_length;
} EFFECT;

EFFECT* new_effect(double duration); 

#endif
