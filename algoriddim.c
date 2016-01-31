#include <stdio.h>
#include <stdlib.h>

#include "algoriddim.h"
#include "bpmrrr.h"
#include "mixer.h"

extern mixer *mixr;
extern bpmrrr *b;
extern GTABLE *sine_table;
extern GTABLE *tri_table;
extern GTABLE *square_table;
extern GTABLE *saw_up_table;
extern GTABLE *saw_down_table;

algo *new_algo()
{
  algo *a = NULL;
  a = calloc(1, sizeof(algo));
  if (a == NULL) {
    fprintf(stderr, "Nae memory for that song, mate..\n");
    return NULL;
  }
  return a;
}

void *algo_run(void *a)
{
  (void) a;
  int sleep;

  //int num_sigs = 0;

  int melody[] = {180, 180, 170, 150};

  add_osc(mixr, 227, sine_table);
  add_osc(mixr, 230, sine_table);
  add_osc(mixr, 180, sine_table);
  for (int i = 1; ; i = (i+1) %4)
  {
    if (b->cur_tick % 8 == 0) {
      if (!sleep) {
        sleep = 1;
        freq_change(mixr, 2, (melody[i]));
      }
    } else if (sleep)
      sleep = 0;
  }
}
