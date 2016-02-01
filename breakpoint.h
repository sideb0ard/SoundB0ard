#ifndef BREAKPOINT_H
#define BREAKPOINT_H

typedef struct breakpoint {
  double time;
  double value;
} BREAKPOINT;

typedef struct breakpoint_stream {
  BREAKPOINT* points;
  BREAKPOINT leftpoint, rightpoint;
  unsigned long npoints;
  double curpos;
  double incr;
  double width;
  double height;
  unsigned long ileft, iright;
  int more_points;
} BRKSTREAM;

BREAKPOINT maxpoint(const BREAKPOINT* points, long npoints);
BRKSTREAM *bps_newstream(void);
void free_stream(BRKSTREAM* stream);
double bps_tick(BRKSTREAM* stream);

void ps_stream(BRKSTREAM *stream);
void update_stream_bpm(BRKSTREAM *stream);

#endif
