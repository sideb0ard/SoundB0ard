#ifndef ENVELOPE_H
#define ENVELOPE_H

typedef struct envelope {
    double time;
    double value;
} ENVELOPE;

typedef struct envelope_stream {
    ENVELOPE *points;
    ENVELOPE leftpoint, rightpoint;
    unsigned long npoints;
    double curpos;
    double incr;
    double width;
    double height;
    int started;
    unsigned long ileft, iright;
} ENVSTREAM;

ENVELOPE maxpoint(const ENVELOPE *points, long npoints);
ENVSTREAM *new_envelope_stream(int env_len, int type); // env_len is bars
void free_stream(ENVSTREAM *stream);
double envelope_stream_tick(ENVSTREAM *stream);

ENVELOPE *newpoints(void);
ENVELOPE *new_fadein_points(void);
ENVELOPE *new_fadeout_points(void);
ENVELOPE *new_wavey_points(void);
void printbp(ENVELOPE *bp);

void ps_envelope_stream(ENVSTREAM *stream);
void update_envelope_stream_bpm(ENVSTREAM *stream);

#endif
