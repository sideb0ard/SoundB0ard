#ifndef SOUNDGEN_H
#define SOUNDGEN_H

// abstract class
//

// typedef struct t_soundgen SOUNDGEN;
//typedef double (*sg_gennext) (SOUNDGEN* sg);
//typedef void (*sg_voladj) (SOUNDGEN* sg, double vol);

typedef struct t_soundgen {
  double (*gennext)(void *self);
} SOUNDGEN;

#endif // SOUNDGEN_H
