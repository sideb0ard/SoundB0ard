#ifndef EFFECT_H
#define EFFECT_H

typedef enum {
  delay,
  lowpass,
  highpass
} EFFECT_TYPE;

typedef struct {
  double* buffer;
  int buf_p;
  int buf_length;
  EFFECT_TYPE type;
} EFFECT;

EFFECT* new_delay(double duration); 

#endif
