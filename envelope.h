#ifndef ENVELOPE_H
#define ENVELOPE_H

typedef struct envelope {
  double time;
  double value;
} ENVELOPE;

typedef struct envelope_stream {
  ENVELOPE* points;
  ENVELOPE leftpoint, rightpoint;
  unsigned long npoints;
  double curpos;
  double incr;
  double width;
  double height;
  unsigned long ileft, iright;
} ENVSTREAM;

ENVELOPE maxpoint(const ENVELOPE* points, long npoints);
ENVSTREAM *new_envelope_stream(void);
void free_stream(ENVSTREAM* stream);
double envelope_stream_tick(ENVSTREAM* stream);

ENVELOPE* newpoints(void);
void printbp(ENVELOPE *bp);

void ps_envelope_stream(ENVSTREAM *stream);
void update_envelope_stream_bpm(ENVSTREAM *stream);

#endif
