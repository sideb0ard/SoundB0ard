#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defjams.h"
#include "mixer.h"
#include "sampler.h"

extern mixer *mixr;

SAMPLER *new_sampler(char *filename, double loop_len)
{
    SAMPLER *sampler = calloc(1, sizeof(SAMPLER));
    sampler->vol = 0.7;
    sampler->started = false;
    sampler->just_been_resampled = false;

    sampler_add_sample(sampler, filename, loop_len);

    sampler->sound_generator.gennext = &sampler_gennext;
    sampler->sound_generator.status = &sampler_status;
    sampler->sound_generator.getvol = &sampler_getvol;
    sampler->sound_generator.setvol = &sampler_setvol;
    sampler->sound_generator.type = SAMPLER_TYPE;

    for (int i = 0; i < MAX_SAMPLES_PER_LOOPER; i++) {
        sampler->sample_num_loops[i] = 1;
    }

    return sampler;
}

double sampler_gennext(void *self)
// void sampler_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
    SAMPLER *sampler = self;
    double val = 0;

    // wait till start of loop to keep patterns synched
    if (!sampler->started) {
        if (mixr->sixteenth_note_tick % 16 == 0) {
            sampler->started = true;
        }
        else {
            return val;
        }
    }

    // resync after a resample/resize
    if (sampler->just_been_resampled && mixr->sixteenth_note_tick % 16 == 0) {
        printf("Resyncing after resample...zzzz\n");
        sampler->samples[sampler->cur_sample]->position = 0;
        sampler->just_been_resampled = false;
    }

    if (sampler->samples[sampler->cur_sample]->position == 0) {

        if (sampler->multi_sample_mode) {
            sampler->cur_sample_iteration--;
            if (sampler->cur_sample_iteration == 0) {
                sampler->cur_sample =
                    (sampler->cur_sample + 1) % sampler->num_samples;
                sampler->cur_sample_iteration =
                    sampler->sample_num_loops[sampler->cur_sample];
            }
        }
    }

    val = sampler->samples[sampler->cur_sample]->resampled_file_bytes
              [sampler->samples[sampler->cur_sample]->position++];

    if (sampler->samples[sampler->cur_sample]->position ==
        sampler->samples[sampler->cur_sample]->resampled_file_size) {
        sampler->samples[sampler->cur_sample]->position = 0;
        // sampler->cur_sample =
        //    (sampler->cur_sample + 1) % sampler->num_samples;
    }
    // pthread_mutex_unlock(&sampler->resample_mutex);

    if (val > 1 || val < -1)
        printf("BURNIE - SAMPLER OVERLOAD!\n");

    val = effector(&sampler->sound_generator, val);
    val = envelopor(&sampler->sound_generator, val);

    return val * sampler->vol;
}

void sampler_add_sample(SAMPLER *s, char *filename, int loop_len)
{
    printf("SAMPLER!, adding a new SAMPLE!\n");

    if (s->num_samples > MAX_SAMPLES_PER_LOOPER) {
        printf("Already have max num samples\n");
        return;
    }

    file_sample *fs = calloc(1, sizeof(file_sample));
    sample_set_file_name(fs, filename);
    fs->position = 0;
    fs->loop_len = loop_len;
    sample_import_file_contents(fs, filename);
    sample_resample_to_loop_size(fs);

    sample_import_file_contents(fs, filename);
    sample_resample_to_loop_size(fs);

    s->samples[s->num_samples++] = fs;
    printf("done adding SAMPLE\n");
}

void sample_set_file_name(file_sample *fs, char *filename)
{
    int fslen = strlen(filename);
    fs->filename = calloc(1, fslen + 1);
    strncpy(fs->filename, filename, fslen);
}

void sample_import_file_contents(file_sample *fs, char *filename)
{
    SNDFILE *snd_file;
    SF_INFO sf_info;

    char cwd[1024];
    getcwd(cwd, 1024);
    char full_filename[strlen(filename) + strlen(cwd) +
                       7]; // 7 == '/wavs/' is 6 and 1 for '\0'
    strcpy(full_filename, cwd);
    strcat(full_filename, "/wavs/");
    strcat(full_filename, filename);

    printf("FLSIL %s\n", full_filename);

    sf_info.format = 0;
    snd_file = sf_open(full_filename, SFM_READ, &sf_info);
    if (!snd_file) {
        printf("Err opening %s : %d\n", full_filename, sf_error(snd_file));
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

    fs->orig_file_bytes = buffer;
    fs->orig_file_size = bufsize;
    fs->samplerate = sf_info.samplerate;
    fs->channels = sf_info.channels;
}

void sampler_resample_to_loop_size(SAMPLER *s)
{
    for (int i = 0; i < s->num_samples; i++) {
        sample_resample_to_loop_size(s->samples[i]);
    }
    s->just_been_resampled = true;
}

void sampler_change_loop_len(SAMPLER *s, int sample_num, int loop_len)
{
    if (loop_len > 0 && sample_num < s->num_samples) {
        file_sample *fs = s->samples[sample_num];
        fs->loop_len = loop_len;
        sample_resample_to_loop_size(fs);
    }
}

void sample_resample_to_loop_size(file_sample *fs)
{
    printf("BUFSIZE is %d\n", fs->orig_file_size);
    printf("CHANNELS is %d\n", fs->channels);

    double loop_len_in_samples =
        mixr->samples_per_midi_tick * PPL * fs->loop_len;

    double *resampled_file_bytes =
        (double *)calloc(loop_len_in_samples, sizeof(double));
    if (resampled_file_bytes == NULL) {
        printf("Memory barf in sampler resample\n");
        return;
    }

    int *table = fs->orig_file_bytes;
    double bufsize = fs->orig_file_size;

    double position = 0;
    double incr = (double)fs->orig_file_size / loop_len_in_samples;
    for (int i = 0; i < loop_len_in_samples; i++) {
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
        resampled_file_bytes[i] = val / 2147483648.0;
    }

    // pthread_mutex_lock(&sampler->resample_mutex);
    bool is_previous_buffer = fs->resampled_file_bytes != NULL ? true : false;
    if (is_previous_buffer) {
        double *oldbuf = fs->resampled_file_bytes;
        int old_relative_position =
            (100 / fs->resampled_file_size) * fs->position;

        fs->resampled_file_bytes = resampled_file_bytes;
        fs->resampled_file_size = loop_len_in_samples;

        fs->position = (loop_len_in_samples / 100) * old_relative_position;
        free(oldbuf);
    }
    else {
        fs->resampled_file_bytes = resampled_file_bytes;
        fs->resampled_file_size = loop_len_in_samples;
    }
    // pthread_mutex_unlock(&sampler->resample_mutex);
}

void sampler_status(void *self, wchar_t *status_string)
{
    SAMPLER *sampler = self;
    swprintf(status_string, MAX_PS_STRING_SZ, WCOOL_COLOR_GREEN
             "[LOOPER] Vol: %.2lf Multi: %d Current Sample: %d",
             sampler->vol, sampler->multi_sample_mode,
             sampler->cur_sample);
    int strlen_left = MAX_PS_STRING_SZ - wcslen(status_string);
    wchar_t looper_details[strlen_left];
    for (int i = 0; i < sampler->num_samples; i++) {
        swprintf(looper_details, 128,
                 L"\n      [%d] %s - looplen: %d numloops: %d", i,
                 basename(sampler->samples[i]->filename),
                 sampler->samples[i]->loop_len, sampler->sample_num_loops[i]);
        wcslcat(status_string, looper_details, strlen_left);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
}

void sampler_set_multi_sample_mode(SAMPLER *s, bool multimode)
{
    s->multi_sample_mode = multimode;
    s->cur_sample_iteration = s->sample_num_loops[s->cur_sample];
}

void sampler_switch_sample(SAMPLER *s, int sample_num)
{
    if (sample_num < s->num_samples)
        s->cur_sample = sample_num;
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

void sampler_change_num_loops(SAMPLER *s, int sample_num, int num_loops)
{
    if (sample_num < s->num_samples && num_loops > 0) {
        s->sample_num_loops[sample_num] = num_loops;
    }
}
