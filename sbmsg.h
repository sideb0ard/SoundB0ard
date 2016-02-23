#ifndef SBMSG_H
#define SBMSG_H

#include "sound_generator.h"

typedef struct sbmsg {
  char cmd[20];
  char params[20];
  int freq;
  SOUNDGEN* sound_generator;
} SBMSG;

SBMSG* new_sbmsg(void);

#endif
