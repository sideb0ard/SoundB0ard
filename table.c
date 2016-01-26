#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "defjams.h"
#include "table.h"

static GTABLE* _new_gtable(void);
// static void norm_gtable(GTABLE* gtable);

static GTABLE* _new_gtable(void)
{
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
  return gtable;
}

// TABLE GENERATION SIGNALS
//
GTABLE* new_sine_table()
{
  unsigned long i;
  double step;
  
  GTABLE* gtable = _new_gtable();
  if (gtable == NULL)
    return NULL;

  step = TWO_PI / TABLEN;
  for (i = 0; i < TABLEN; i++)
    gtable->table[i] = sin(step * i);
  gtable->table[i] = gtable->table[0]; // guard point

  return gtable;
}

GTABLE* new_tri_table()
{
  unsigned long i;
  double step;
  
  GTABLE* gtable = _new_gtable();
  if (gtable == NULL)
    return NULL;

  step = TWO_PI / TABLEN;
  for (i = 0; i < TABLEN; i++)
    gtable->table[i] = (2.0 * ((step * i) * (1.0 / TWO_PI) )) - 1.0;
  gtable->table[i] = gtable->table[0]; // guard point

  return gtable;
}

GTABLE* new_square_table()
{
  unsigned long i;
  
  GTABLE* gtable = _new_gtable();
  if (gtable == NULL)
    return NULL;

  for (i = 0; i < TABLEN; i++)
    if (i < M_PI)
      gtable->table[i] = 1.0;
    else
      gtable->table[i] = -1;

  gtable->table[i] = gtable->table[0]; // guard point

  return gtable;
}

GTABLE* new_saw_table(int up)
{
  unsigned long i;
  float step;
  
  GTABLE* gtable = _new_gtable();
  if (gtable == NULL)
    return NULL;

  step = TWO_PI / TABLEN;
  if (up) {
    for (i = 0; i < TABLEN; i++)
      gtable->table[i] = (2.0 * ((step * i) * (1.0 / TWO_PI) )) - 1.0;
  } else {
    for (i = 0; i < TABLEN; i++)
      gtable->table[i] = 1.0 - 2.0 * ((step * i) * (1.0 / TWO_PI) );
  }
  gtable->table[i] = gtable->table[0]; // guard point

  return gtable;
}
// END TABLE GENERATION SIGNALS

void table_info(GTABLE* gtable)
{
  printf("TABLE LEN: %lu\n", gtable->length);
  for (double i=0; i<gtable->length; i++) 
    printf("%f", gtable->table[(int)i]);
}

void gtable_free(GTABLE** gtable)
{
  if (gtable && *gtable && (*gtable)->table) {
    free((*gtable)->table);
    free(*gtable);
    *gtable = NULL;
  }
}

// static void norm_gtable(GTABLE* gtable) 
// {
//   unsigned long i;
//   double val, maxamp = 0.0;
//   for (i = 0; i < gtable->length; i++) {
//     val = fabs(gtable->table[i]);
//     if ( maxamp < val)
//       maxamp = val;
//   }
//   maxamp = 1.0 / maxamp;
//   for (i=0; i < gtable->length; i++)
//     gtable->table[i] *= maxamp;
//   gtable->table[i] = gtable->table[0];
// }
