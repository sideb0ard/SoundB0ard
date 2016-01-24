#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "defjams.h"
#include "table.h"

GTABLE* new_sine_table()
{
  unsigned long i;
  double step;
  GTABLE* gtable = NULL;

  gtable = (GTABLE*) calloc(1, sizeof(GTABLE));
  if (gtable == NULL)
    return NULL;
  gtable->table = (double*) calloc(1, (TABLEN+1)*sizeof(double));
  if (gtable->table == NULL) {
    free(gtable);
    return NULL;
  }
  gtable->length = TABLEN;
  step = TWO_PI / TABLEN;
  for (i = 0; i < TABLEN; i++)
    gtable->table[i] = sin(step * i);
  gtable->table[i] = gtable->table[0]; // guard point
  return gtable;
}

void table_info(GTABLE* gtable)
{
  printf("TABLE LEN: %lu\n", gtable->length);
  for (double i=0; i<gtable->length; i++) 
    printf("%f", gtable->table[(int)i]);
}

//void gtable_free(GTABLE** gtable)
//{
//  if (gtable && *gtable && (*gtable)->table) {
//    free((*gtable)->table);
//    free(*gtable);
//    *gtable = NULL;
//  }
//}
