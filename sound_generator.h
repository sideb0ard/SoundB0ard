#ifndef SOUNDGEN_H
#define SOUNDGEN_H

// abstract class
//

typedef struct t_soundgen {
  // TODO : ENUM for type - i.e. OSC, FM or SAMPLE
  double (*gennext)(void *self);
  void (*status)(void *self, char *string);
  void (*vol)(void *self, double val); // TODO - implement this
} SOUNDGEN;

#endif // SOUNDGEN_H
