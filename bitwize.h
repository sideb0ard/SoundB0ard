#ifndef BITWIZE_H
#define BITWIZE_H

#include <stdio.h>

#include "sound_generator.h"

typedef struct t_bitwize
{

  SOUNDGEN sound_generator;

  double incr;
  double vol;

  int pattern;

} BITWIZE;

BITWIZE* new_bitwize(int pattern);

char bitwize_process(int pattern, int t);
double bitwize_gennext(void* self);
void bitwize_setvol(void *self, double v);
double bitwize_getvol(void *self);

void bitwize_status(void *self, char *status_string);

#endif // BITWIZE_H