#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "mixer.h"
#include "sampler.h"

extern mixer *mixr;

SAMPLER *new_sampler(char *filename, double loop_len)
{
    SAMPLER *sampler = calloc(1, sizeof(SAMPLER));
    sampler->position = 0;
    sampler->loop_len = loop_len;
    sampler->vol = 0.0;

    sampler_set_file_name(sampler, filename);
    sampler_import_file_contents(sampler, filename);

    pthread_mutex_init(&sampler->resample_mutex, NULL);
    sampler_resample_to_loop_size(sampler);

    sampler->sound_generator.gennext = &sampler_gennext;
    sampler->sound_generator.status = &sampler_status;
    sampler->sound_generator.getvol = &sampler_getvol;
    sampler->sound_generator.setvol = &sampler_setvol;
    sampler->sound_generator.type = SAMPLER_TYPE;

    printf("Filename:: %s\n", sampler->filename);
    printf("SR: %d\n", sampler->samplerate);
    printf("Channels: %d\n", sampler->channels);
    printf("Ticks in a loop: %d\n", sampler->resampled_file_bufsize);

    return sampler;
}

void sampler_set_file_name(SAMPLER *s, char *filename)
{
    int fslen = strlen(filename);
    s->filename = calloc(1, fslen + 1);
    strncpy(s->filename, filename, fslen);
}

void sampler_import_file_contents(SAMPLER *s, char *filename)
{
    SNDFILE *snd_file;
    SF_INFO sf_info;

    sf_info.format = 0;
    snd_file = sf_open(filename, SFM_READ, &sf_info);
    if (!snd_file) {
        printf("Err opening %s : %d\n", filename, sf_error(snd_file));
        return;
    }

    int bufsize = sf_info.channels * sf_info.frames;
    printf("Making buffer size of %d\n", bufsize);

    int *buffer = calloc(bufsize, sizeof(int));
    if (buffer == NULL) {
        perror("Ooft, memory issues, mate!\n");
        return;
    }

    sf_readf_int(snd_file, buffer, bufsize);

    s->orig_file_buffer = buffer;
    s->orig_file_bufsize = bufsize;
    s->samplerate = sf_info.samplerate;
    s->channels = sf_info.channels;

}

void sampler_resample_to_loop_size(SAMPLER *sampler)
{
    printf("BUFSIZE is %d\n", sampler->orig_file_bufsize);
    printf("CHANNELS is %d\n", sampler->channels);

    double size_of_one_loop_in_samples = mixr->samples_per_midi_tick * PPL;

    double *resampled_file_buffer = (double*) calloc(size_of_one_loop_in_samples,
                                                     sizeof(double));
    if (resampled_file_buffer == NULL)
    {
        printf("Memory barf in sampler resample\n");
        return;
    }

    int *table = sampler->orig_file_buffer;
    double bufsize = sampler->orig_file_bufsize;

    double position = 0;
    double incr = (double) sampler->orig_file_bufsize / size_of_one_loop_in_samples;
    for ( int i = 0; i < size_of_one_loop_in_samples; i++)
    {
        int base_index = (int)position;
        unsigned long next_index = base_index + 1;
        double frac, slope, val;

        frac = position - base_index;
        val = table[base_index];
        slope = table[next_index] - val;

        val += (frac * slope);
        position += incr;

        if (position >= bufsize) {
            printf("POSITION: %f // BUFSIZR: %f My job here is done\n",
                    position, bufsize);
            break;
        }

        // convert from 16bit int to double between 0 and 1
        resampled_file_buffer[i] = val / 2147483648.0;
    }

    pthread_mutex_lock(&sampler->resample_mutex);
    bool is_previous_buffer = sampler->resampled_file_buffer != NULL ? true : false;
    if (is_previous_buffer)
    {
        double *oldbuf = sampler->resampled_file_buffer;
        int old_relative_position = (100 /  sampler->resampled_file_bufsize)  * sampler->position;

        sampler->resampled_file_buffer = resampled_file_buffer;
        sampler->resampled_file_bufsize = size_of_one_loop_in_samples;

        sampler->position = (size_of_one_loop_in_samples / 100) * old_relative_position;
        free(oldbuf);
    }
    else {
        sampler->resampled_file_buffer = resampled_file_buffer;
        sampler->resampled_file_bufsize = size_of_one_loop_in_samples;
    }
    pthread_mutex_unlock(&sampler->resample_mutex);

}

double sampler_gennext(void *self)
// void sampler_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
    SAMPLER *sampler = self;
    // double val = 0;

    if (sampler->position == 0) {
        printf("SAMPLER START OF LOOP: %d 16Tick %d Tick %d\n",
                sampler->position,
                mixr->sixteenth_note_tick,
                mixr->tick);
    }
    pthread_mutex_lock(&sampler->resample_mutex);
    double val = sampler->resampled_file_buffer[sampler->position++];
    if (sampler->position == sampler->resampled_file_bufsize)
    {
        sampler->position = 0;
    }
    pthread_mutex_unlock(&sampler->resample_mutex);

    if (val > 1 || val < -1)
        printf("BURNIE - SAMPLER OVERLOAD!\n");

    val = effector(&sampler->sound_generator, val);
    val = envelopor(&sampler->sound_generator, val);

    return val * sampler->vol;
}

void sampler_status(void *self, char *status_string)
{
    SAMPLER *sampler = self;
    snprintf(status_string, 119,
             COOL_COLOR_GREEN "[%s]\tvol: %.2lf" ANSI_COLOR_RESET,
             basename(sampler->filename), sampler->vol);
}

double sampler_getvol(void *self)
{
    SAMPLER *sampler = self;
    return sampler->vol;
}

void sampler_setvol(void *self, double v)
{
    SAMPLER *sampler = self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    sampler->vol = v;
}
