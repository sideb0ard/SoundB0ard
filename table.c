#include <stdlib.h>
#include <math.h>

#include "defjams.h"
#include "table.h"

GTABLE* new_sine_table(unsigned long length)
{
  unsigned long i;
  double step;
  GTABLE* gtable = NULL;

  if (length == 0)
    return NULL;
  gtable = (GTABLE*) calloc(1, sizeof(GTABLE));
  if (gtable == NULL)
    return NULL;
  gtable->table = (double*) calloc(1, (length+1)*sizeof(double));
  if (gtable->table == NULL) {
    free(gtable);
    return NULL;
  }
  gtable->length = length;
  step = TWO_PI / length;
  for (i = 0; i < length; i++)
    gtable->table[i] = sin(step * i);
  gtable->table[i] = gtable->table[0]; // guard point
  return gtable;
}

void gtable_free(GTABLE** gtable)
{
  if (gtable && *gtable && (*gtable)->table) {
    free((*gtable)->table);
    free(*gtable);
    *gtable = NULL;
  }
}
