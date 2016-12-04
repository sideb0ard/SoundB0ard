#ifndef SAMPLER_H
#define SAMPLER_H

#include <pthread.h>
#include <stdbool.h>
#include "sound_generator.h"

typedef struct t_sampler {
    SOUNDGEN sound_generator;
    char     *filename;

    int      *orig_file_buffer;
    int       orig_file_bufsize;

    double   *resampled_file_buffer;
    int       resampled_file_bufsize;

    pthread_mutex_t resample_mutex;

    int       position;

    int       samplerate;
    int       channels;

    double loop_len;
    double vol;
    bool started;
    bool just_been_resampled;
} SAMPLER;

SAMPLER *new_sampler(char *filename, double loop_len); // loop_len in bars

// void sampler_gennext(void* self, double* frame_vals, int framesPerBuffer);
double sampler_gennext(void *self);
void sampler_status(void *self, char *ss);
void sampler_setvol(void *self, double v);
double sampler_getvol(void *self);

void sampler_resample_to_loop_size(SAMPLER *self);
void sampler_import_file_contents(SAMPLER *s, char *filename);
void sampler_set_file_name(SAMPLER *s, char *filename);

#endif // SAMPLER_H
