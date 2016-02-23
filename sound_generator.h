#ifndef SOUNDGEN_H
#define SOUNDGEN_H

// abstract class
//

typedef struct t_soundgen {
  double (*gennext)(void *self);
  void (*status)(void *self, char *string);
} SOUNDGEN;

#endif // SOUNDGEN_H
