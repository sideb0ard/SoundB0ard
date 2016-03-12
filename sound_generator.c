#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "sound_generator.h"

void link_effect(SOUNDGEN* sg, int effect_no) // effect no is where in mixer effect array
{
  if (sg->effects_size <= sg->effects_num) {
    if (sg->effects_size == 0) {
      sg->effects_size = DEFAULT_ARRAY_SIZE;
    } else {
    sg->effects_size *= 2;
    }

    int* new_effects = realloc(sg->effects, sg->effects_size * sizeof(int));
    if (new_effects == NULL) {
      printf("Ooh, burney - cannae allocate memory for new effects");
      return;
    } else {
      sg->effects = new_effects;
    }
  }
  sg->effects[sg->effects_num++] = effect_no;
  sg->effects_on = 1;

  printf("Effect added!\n");
}
