#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mixer.h"

mixer *new_mixer()
{
  printf("New Mixer called!\n");
  mixer *mixr = NULL;
  mixr = calloc(1, sizeof(mixer));
  if (mixr == NULL) {
    printf("Nae mixer, fucked up!\n");
    return NULL;
  }
  printf("Finished Calloc - returning from new mixer..\n");
  return mixr;
}

void *mixer_run(void *args)
{
  while(1)
  {
    printf("Mixingg away...\n");
    sleep(2);
  }
}
