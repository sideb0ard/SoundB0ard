#include <pthread.h>
#include <stdio.h>

#include "audioutils.h"
#include "breakpoint.h"
#include "bpmrrr.h"
#include "cmdloop.h"
#include "defjams.h"
#include "mixer.h"

mixer *mixr;
bpmrrr *b;
GTABLE *sine_table;
GTABLE *tri_table;
GTABLE *square_table;
GTABLE *saw_up_table;
GTABLE *saw_down_table;

BRKSTREAM* ampstream = NULL;

int main()
{
  // PortAudio start me up!
  pa_setup();

  // lookup table for wavs
  sine_table = new_sine_table();
  tri_table = new_tri_table();
  square_table = new_square_table();
  saw_up_table = new_saw_table(1);
  saw_down_table = new_saw_table(0);

  ampstream = bps_newstream();

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
