#include <stdio.h>
#include <stdlib.h>

#include "breakpoint.h"
#include "bpmrrr.h"
#include "defjams.h"
#include "mixer.h"

extern mixer *mixr;
extern bpmrrr *b;

static void _bps_updatepoints(BRKSTREAM *stream)
{
  stream->leftpoint = stream->points[stream->ileft];
  stream->rightpoint = stream->points[stream->iright];
  stream->width = stream->rightpoint.time - stream->leftpoint.time;
  stream->height = stream->rightpoint.value - stream->leftpoint.value;
}

static void _bps_reset(BRKSTREAM *stream)
{
  stream->curpos = 0.0;
  stream->ileft = 0;
  stream->iright = 1;
  _bps_updatepoints(stream);

}

void update_stream_bpm(BRKSTREAM *stream)
{
  stream->incr = 100.0 / (60.0 / (b->bpm) * SAMPLE_RATE * 4.0);
  printf("called me - BPM IS NOW %d\n", b->bpm);
}


BREAKPOINT maxpoint(const BREAKPOINT* points, long npoints)
{
  BREAKPOINT point;

  point.value = points[0].value;
  point.time = points[0].time;

  for (int i = 1; i < npoints; i++) {
    if ( point.value < points[i].value ) {
      point.value = points[i].value;
      point.time = points[i].time;
    }
  }
  return point;
}

BREAKPOINT* newpoints()
{

  BREAKPOINT *points = NULL;
  points = (BREAKPOINT*) calloc(4, sizeof(BREAKPOINT));
  if (points == NULL)
    return NULL;
  points[0].time = 0.0; 
  points[0].value = 0.0;
  points[1].time = 15.00;
  points[1].value = 1.0;
  points[2].time = 85.00; 
  points[2].value = 1.0;
  points[3].time = 99.0;
  points[3].value = 0.0;
  return points;
}

BRKSTREAM* bps_newstream()
{
  BRKSTREAM* stream;
  BREAKPOINT *points = newpoints();

  unsigned long npoints = 4;
  stream = (BRKSTREAM*) calloc(1, sizeof(BRKSTREAM));
  if (stream == NULL)
    return NULL;
  stream->points = points;
  stream->npoints = npoints;
  stream->incr = 100.0 / (60.0 / DEFAULT_BPM * SAMPLE_RATE * 4); // 4 bars long

  _bps_reset(stream);

  return stream;
}

void free_stream(BRKSTREAM* stream)
{
  if (stream && stream->points) {
    free(stream->points);
    stream->points = NULL;
  }
}

double bps_tick(BRKSTREAM* stream)
{
  double thisval, frac;
  frac = (stream->curpos - stream->leftpoint.time) / stream->width;
  thisval = stream->leftpoint.value+(stream->height*frac);

  stream->curpos += stream->incr;
  if(stream->curpos > stream->rightpoint.time) {
    stream->ileft++; stream->iright++;
    if (stream->iright < stream->npoints) {
      _bps_updatepoints(stream);
    } else {
      _bps_reset(stream);
    }
  }
  return thisval;
}

void printbp(BREAKPOINT *bp)
{
  printf("Time: %f Val: %f\n", bp->time, bp->value);
}

void ps_stream(BRKSTREAM *stream)
{
    //BREAKPOINT tmp = maxpoint(stream->points, 4);
    //for (int i = 0; i < 4; i++)
    //  printbp(&stream->points[0]);
    //printf("Max: %f\n", tmp.value);
    
    printf("Cur: %f Incr: %f Val: %f\n", stream->curpos, stream->incr, bps_tick(stream));
}
