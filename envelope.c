#include <stdio.h>
#include <stdlib.h>

#include "envelope.h"
#include "bpmrrr.h"
#include "defjams.h"
#include "mixer.h"

extern mixer *mixr;
extern bpmrrr *b;

static void _env_updatepoints(ENVSTREAM *stream)
{
  stream->leftpoint = stream->points[stream->ileft];
  stream->rightpoint = stream->points[stream->iright];
  stream->width = stream->rightpoint.time - stream->leftpoint.time;
  stream->height = stream->rightpoint.value - stream->leftpoint.value;
}

static void _env_reset(ENVSTREAM *stream)
{
  stream->curpos = 0.0;
  stream->ileft = 0;
  stream->iright = 1;
  _env_updatepoints(stream);

}

void update_envelope_stream_bpm(ENVSTREAM *stream)
{
  // 100 is for percent - time of envelope is measured as percent of loop len
  stream->incr = 100.0 / (60.0 / (b->bpm) * SAMPLE_RATE * DEFAULT_ENV_LENGTH);
  printf("called me - BPM IS NOW %d\n", b->bpm);
}


ENVELOPE maxpoint(const ENVELOPE* points, long npoints)
{
  ENVELOPE point;

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

ENVELOPE* newpoints()
{

  ENVELOPE *points = NULL;
  points = (ENVELOPE*) calloc(4, sizeof(ENVELOPE));
  if (points == NULL)
    return NULL;
  points[0].time = 0.0; 
  points[0].value = 0.1;
  points[1].time = 15.00;
  points[1].value = 1.0;
  points[2].time = 85.00; 
  points[2].value = 1.0;
  points[3].time = 99.0;
  points[3].value = 0.1;
  return points;
}

ENVELOPE* new_fadein_points()
{

  ENVELOPE *points = NULL;
  points = (ENVELOPE*) calloc(4, sizeof(ENVELOPE));
  if (points == NULL)
    return NULL;
  points[0].time = 0.0; 
  points[0].value = 0.1;
  points[1].time = 45.00;
  points[1].value = 0.5;
  points[2].time = 85.00; 
  points[2].value = 1.0;
  points[3].time = 99.0;
  points[3].value = 0.1;
  return points;
}

ENVELOPE* new_fadeout_points()
{

  ENVELOPE *points = NULL;
  points = (ENVELOPE*) calloc(4, sizeof(ENVELOPE));
  if (points == NULL)
    return NULL;
  points[0].time = 0.0; 
  points[0].value = 0.1;
  points[1].time = 10.00;
  points[1].value = 1.0;
  points[2].time = 35.00; 
  points[2].value = 1.0;
  points[3].time = 99.0;
  points[3].value = 0.1;
  return points;
}

ENVSTREAM* new_envelope_stream(int env_len, int type) // env_len is bars TODO: enum
{
  ENVSTREAM* stream;

  ENVELOPE *points;
  switch(type) {
    case 1 :
      points = newpoints();
      break;
    case 2 :
      points = new_fadein_points();
      break;
    case 3 :
      points = new_fadeout_points();
      break;
  }

  unsigned long npoints = 4;
  stream = (ENVSTREAM*) calloc(1, sizeof(ENVSTREAM));
  if (stream == NULL)
    return NULL;
  stream->points = points;
  stream->npoints = npoints;
  stream->incr = 100.0 / (60.0 / DEFAULT_BPM * SAMPLE_RATE * env_len); 

  _env_reset(stream);

  return stream;
}

void free_stream(ENVSTREAM* stream)
{
  if (stream && stream->points) {
    free(stream->points);
    stream->points = NULL;
  }
}

double envelope_stream_tick(ENVSTREAM* stream)
{
  double thisval, frac;
  frac = (stream->curpos - stream->leftpoint.time) / stream->width;
  thisval = stream->leftpoint.value+(stream->height*frac);

  stream->curpos += stream->incr;
  if(stream->curpos > stream->rightpoint.time) {
    stream->ileft++; stream->iright++;
    if (stream->iright < stream->npoints) {
      _env_updatepoints(stream);
    } else {
      _env_reset(stream);
    }
  }
  return thisval;
}

void printbp(ENVELOPE *bp)
{
  printf("Time: %f Val: %f\n", bp->time, bp->value);
}

void ps_envelope_stream(ENVSTREAM *stream)
{
    //ENVELOPE tmp = maxpoint(stream->points, 4);
    //for (int i = 0; i < 4; i++)
    //  printbp(&stream->points[0]);
    //printf("Max: %f\n", tmp.value);
    
    printf("Cur: %f Incr: %f Val: %f\n", stream->curpos, stream->incr, envelope_stream_tick(stream));
}
