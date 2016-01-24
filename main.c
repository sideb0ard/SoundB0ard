#include <pthread.h>
#include <stdio.h>

#include "audioutils.h"
#include "bpmrrr.h"
#include "cmdloop.h"
#include "defjams.h"
#include "mixer.h"

mixer *mixr;
bpmrrr *b;
GTABLE *sine_table;

int main()
{
  // PortAudio start me up!
  pa_setup();

  // lookup table for wavs
  sine_table = new_sine_table();
  // TODO: more tables

  // run da mixer
  mixr = new_mixer();
  pthread_t mixrrun_th;
  if ( pthread_create (&mixrrun_th, NULL, mixer_run, (void*) mixr)) {
    fprintf(stderr, "Error running mixer_run thread\n");
    return 1;
  }
  pthread_detach(mixrrun_th);

  // run da BPM counterrr...
  b = new_bpmrrr();
  pthread_t bpmrun_th;
  if ( pthread_create (&bpmrun_th, NULL, bpm_run, (void*) b)) {
    fprintf(stderr, "Error running BPM_run thread\n");
    return 1;
  }
  pthread_detach(bpmrun_th);

  // interactive loop
  loopy();

  // all done, time to go home
  pa_teardown();

  return 0;
}
