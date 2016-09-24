#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "envelope.h"
#include "mixer.h"

extern mixer *mixr;

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
    stream->incr = 100.0 / (60.0 / (mixr->bpm) * SAMPLE_RATE * DEFAULT_ENV_LENGTH);
    printf("called me - BPM IS NOW %d\n", mixr->bpm);
}

ENVELOPE maxpoint(const ENVELOPE *points, long npoints)
{
    ENVELOPE point;

    point.value = points[0].value;
    point.time = points[0].time;

    for (int i = 1; i < npoints; i++) {
        if (point.value < points[i].value) {
            point.value = points[i].value;
            point.time = points[i].time;
        }
    }
    return point;
}

ENVELOPE *newpoints()
{

    ENVELOPE *points = NULL;
    points = (ENVELOPE *)calloc(4, sizeof(ENVELOPE));
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

ENVELOPE *new_fadein_points()
{

    ENVELOPE *points = NULL;
    points = (ENVELOPE *)calloc(7, sizeof(ENVELOPE));
    if (points == NULL)
        return NULL;
    points[0].time = 0.0;
    points[0].value = 0.1;
    points[1].time = 45.00;
    points[1].value = 0.1;
    points[2].time = 55.00;
    points[2].value = 0.4;
    points[3].time = 60.0;
    points[3].value = 1.0;
    points[4].time = 80.0;
    points[4].value = 1.0;
    points[5].time = 85.0;
    points[5].value = 0.5;
    points[6].time = 99.0;
    points[6].value = 0.1;
    return points;
}

ENVELOPE *new_fadeout_points()
{

    ENVELOPE *points = NULL;
    points = (ENVELOPE *)calloc(5, sizeof(ENVELOPE));
    if (points == NULL)
        return NULL;
    points[0].time = 0.0;
    points[0].value = 0.1;
    points[1].time = 5.00;
    points[1].value = 1.0;
    points[2].time = 45.00;
    points[2].value = 1.0;
    points[3].time = 50.00;
    points[3].value = 0.1;
    points[4].time = 99.0;
    points[4].value = 0.1;
    return points;
}

ENVELOPE *new_wavey_points()
{

    ENVELOPE *points = NULL;
    points = (ENVELOPE *)calloc(8, sizeof(ENVELOPE));
    if (points == NULL)
        return NULL;
    points[0].time = 0.0;
    points[0].value = 0.3;
    points[1].time = 12.00;
    points[1].value = 0.6;
    points[2].time = 24.00;
    points[2].value = 0.8;
    points[3].time = 36.00;
    points[3].value = 0.5;
    points[4].time = 48.0;
    points[4].value = 0.4;
    points[5].time = 60.0;
    points[5].value = 0.6;
    points[6].time = 72.0;
    points[6].value = 0.4;
    points[7].time = 85.0;
    points[7].value = 0.3;
    return points;
}

ENVSTREAM *new_envelope_stream(int env_len,
                               int type) // env_len is bars TODO: enum
{

    // ENVSTREAM* stream;
    ENVSTREAM *stream = (ENVSTREAM *)calloc(1, sizeof(ENVSTREAM));
    if (stream == NULL)
        return NULL;

    ENVELOPE *points;
    switch (type) {
    case 0:
        points = newpoints();
        stream->npoints = 4;
        break;
    case 1:
        points = new_fadein_points();
        stream->npoints = 7;
        break;
    case 2:
        points = new_fadeout_points();
        stream->npoints = 5;
        break;
    case 3:
        points = new_wavey_points();
        stream->npoints = 8;
        break;
    default:
        points = newpoints();
    }

    // unsigned long npoints = 4;
    stream->points = points;
    stream->incr = 100.0 / (60.0 / DEFAULT_BPM * SAMPLE_RATE * env_len * 4);

    _env_reset(stream);

    return stream;
}

void free_stream(ENVSTREAM *stream)
{
    if (stream && stream->points) {
        free(stream->points);
        stream->points = NULL;
    }
}

double envelope_stream_tick(ENVSTREAM *stream)
{
    double thisval, frac;
    frac = (stream->curpos - stream->leftpoint.time) / stream->width;
    thisval = stream->leftpoint.value + (stream->height * frac);

    stream->curpos += stream->incr;
    if (stream->curpos > stream->rightpoint.time) {
        stream->ileft++;
        stream->iright++;
        if (stream->iright < stream->npoints) {
            _env_updatepoints(stream);
        }
        else {
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
    // ENVELOPE tmp = maxpoint(stream->points, 4);
    // for (int i = 0; i < 4; i++)
    //  printbp(&stream->points[0]);
    // printf("Max: %f\n", tmp.value);

    printf("Cur: %f Incr: %f Val: %f\n", stream->curpos, stream->incr,
           envelope_stream_tick(stream));
}
