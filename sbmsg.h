#ifndef SBMSG_H
#define SBMSG_H

#include "sound_generator.h"

typedef struct sbmsg
{
    // this is a generic data structure to pass arguments
    // at some point, i'll sure this will be too messy and
    // need separated into message types.
    char cmd[20];
    char params[20];
    char filename[100];
    double looplen;
    int freq;
    char mod_osc[10];
    int modfreq;
    char car_osc[10];
    int carfreq;
    int sound_gen_num;
} SBMSG;

SBMSG *new_sbmsg(void);

#endif
