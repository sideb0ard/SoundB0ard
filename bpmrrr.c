#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "defjams.h"
#include "bpmrrr.h"

bpmrrr *new_bpmrrr()
{
  bpmrrr *b = NULL;
  b = calloc(1, sizeof(bpmrrr));
  if (b == NULL) {
    fprintf(stderr, "Nae memory for a bpmrrr, mate.\n");
    return NULL;
  }

  b->bpm = DEFAULT_BPM;
  //b->sleeptime = (60 / b->bpm / 4 ) * 1000000000; // 4 "microticks" per BPS as nanosec
  b->sleeptime = (60.0 / b->bpm / 4 ) * 1000000000; // 4 "microticks" per BPS as nanosec
  printf("SLEEPTIME IS %lf\n", b->sleeptime);
  //b->sleeptime = 999999999L; // 4 "microticks" per BPS as nanosec

  return b;
}

void bpm_change(bpmrrr *b, int bpm)
{
  if (bpm > 60) { // the sleeptime calculation would bring if this was under 60
    b->bpm = bpm;
    b->sleeptime = (60.0 / b->bpm / 4 ) * 1000000000;
  }
}

void bpm_info(bpmrrr *b)
{
  printf(ANSI_COLOR_WHITE "BPM: %d // Current Tick: %d\n" ANSI_COLOR_RESET, b->bpm, b->cur_tick);
}

void *bpm_run(void *bp)
{
  bpmrrr *b = (bpmrrr*) bp; 
  while (1)
  {
    // printf("TICK: %d\n", (b->cur_tick%4 + 1)); 
    b->cur_tick++;

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = b->sleeptime;
    nanosleep(&ts, NULL);
  }
}
