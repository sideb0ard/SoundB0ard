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

    sampler->scramblrrr = sampler_create_sample("none", loop_len);

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

    if (sampler->scramblrrr_mode &&
        (mixr->cur_sample % (sampler->scramblrrr->resampled_file_size * 4) ==
         0)) {
        sampler_scramble(sampler);
    }

    // resync after a resample/resize
    if (sampler->just_been_resampled && mixr->sixteenth_note_tick % 16 == 0) {
        printf("Resyncing after resample...zzzz\n");
        sampler->samples[sampler->cur_sample]->position = 0;
        sampler->scramblrrr->position = 0;
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

    if (sampler->scramblrrr_mode) {
        val = sampler->scramblrrr
                  ->resampled_file_bytes[sampler->scramblrrr->position++];
        // printf("Val! %f // Position! %d\n", val,
        // sampler->scramblrrr->position);
        sampler->samples[sampler->cur_sample]
            ->position++; // keep increasing normal sample
    }
    else {
        val = sampler->samples[sampler->cur_sample]->resampled_file_bytes
                  [sampler->samples[sampler->cur_sample]->position++];
    }

    if (sampler->scramblrrr->position ==
        sampler->scramblrrr->resampled_file_size) {
        sampler->scramblrrr->position = 0;
    }

    if (sampler->samples[sampler->cur_sample]->position ==
        sampler->samples[sampler->cur_sample]->resampled_file_size) {
        sampler->samples[sampler->cur_sample]->position = 0;
    }

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

    file_sample *fs = sampler_create_sample(filename, loop_len);
    s->samples[s->num_samples++] = fs;
    printf("done adding SAMPLE\n");
}

file_sample *sampler_create_sample(char *filename, int loop_len)
{
    file_sample *fs = calloc(1, sizeof(file_sample));
    sample_set_file_name(fs, filename);
    fs->position = 0;
    fs->loop_len = loop_len;

    if (strncmp(filename, "none", 4) != 0) {
        sample_import_file_contents(fs, filename);
    }

    sample_resample_to_loop_size(fs);
    return fs;
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
    sample_resample_to_loop_size(s->scramblrrr);
    s->just_been_resampled = true;
}

void sampler_change_loop_len(SAMPLER *s, int sample_num, int loop_len)
{
    if (loop_len > 0 && sample_num < s->num_samples) {
        file_sample *fs = s->samples[sample_num];

        fs->loop_len = loop_len;
        sample_resample_to_loop_size(fs);

        s->scramblrrr->loop_len = loop_len;
        sample_resample_to_loop_size(s->scramblrrr);
    }
}

void sample_resample_to_loop_size(file_sample *fs)
{
    printf("BUFSIZE is %d\n", fs->orig_file_size);
    printf("CHANNELS is %d\n", fs->channels);

    int loop_len_in_samples = mixr->samples_per_midi_tick * PPL * fs->loop_len;

    double *resampled_file_bytes =
        (double *)calloc(loop_len_in_samples, sizeof(double));
    if (resampled_file_bytes == NULL) {
        printf("Memory barf in sampler resample\n");
        return;
    }

    if (strncmp(fs->filename, "none", 4) != 0) {

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
        bool is_previous_buffer =
            fs->resampled_file_bytes != NULL ? true : false;
        if (is_previous_buffer) {
            double *oldbuf = fs->resampled_file_bytes;
            int old_relative_position =
                (100 / fs->resampled_file_size) * fs->position;

            fs->position = (loop_len_in_samples / 100) * old_relative_position;
            free(oldbuf);
        }
    }

    fs->resampled_file_bytes = resampled_file_bytes;
    fs->resampled_file_size = loop_len_in_samples;
    // pthread_mutex_unlock(&sampler->resample_mutex);
}

void sampler_status(void *self, wchar_t *status_string)
{
    SAMPLER *sampler = self;
    swprintf(
        status_string, MAX_PS_STRING_SZ, WCOOL_COLOR_GREEN
        "[LOOPER] Vol: %.2lf Multi: %d Current Sample: %d ScramblrrrMode: %s MaxGen: %d",
        sampler->vol, sampler->multi_sample_mode, sampler->cur_sample,
        sampler->scramblrrr_mode ? "true" : "false", sampler->max_scramble_generation);
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

void sampler_set_multi_sample_mode(SAMPLER *s, bool multi)
{
    s->multi_sample_mode = multi;
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

void sampler_set_scramble_mode(SAMPLER *s, bool b)
{
    s->scramblrrr_mode = b;
    s->scramble_counter = 0;
    if (s->scramblrrr_mode == true) {
        int len = s->samples[s->cur_sample]->resampled_file_size;
        for (int i = 0; i < len; i++)
            s->scramblrrr->resampled_file_bytes[i] =
                s->samples[s->cur_sample]->resampled_file_bytes[i];
        s->scramblrrr->position = s->samples[s->cur_sample]->position;
    }
    printf("Changed sampler scramblrrr mode to %s\n",
           s->scramblrrr_mode ? "true" : "false");
}

void sampler_set_max_scramble_generation(SAMPLER *s, int max)
{
    s->max_scramble_generation = max;
}

void sampler_scramble(SAMPLER *s)
{
    s->scramble_generation++;

    double *scrambled = s->scramblrrr->resampled_file_bytes;
    int len = s->scramblrrr->resampled_file_size;
    int len16th = len / s->scramblrrr->loop_len / 16;

    // take a copy of the first 16th that we can randomly inject below.
    double first16th[len16th];
    // lets grab a reverse copy too!
    double rev16th[len16th];
    // let's go all in!
    double third16th[len16th];
    double fourth16th[len16th];
    double seventh16th[len16th];

    for (int i = 0; i < len16th; i++) {
        first16th[i] = scrambled[i];
        rev16th[(len16th - 1) - i] = scrambled[i];
        third16th[i] = scrambled[i+len16th*3];
        fourth16th[i] = scrambled[i+len16th*4];
        seventh16th[i] = scrambled[i+len16th*7];
    }

    bool yolo = false;
    bool reverse = false;
    bool copyfirsthalf = false;
    int dice1, dice2;

    for (int i = 0; i < len; i++) {

        if (i % len16th == 0) {
            s->scramble_counter++;
            if (yolo || reverse) {
                yolo = false;
                reverse = false;
            }
            else {
                dice1 = rand() % 100;
                if (dice1 >= 85 && dice1 < 95)
                    yolo = true;
                else if (dice1 >= 95)
                    reverse = true;
            }
            dice2 = rand() % 100;

        }

        if (yolo) {
            s->scramblrrr->resampled_file_bytes[i] = first16th[i % len16th];
        }
        else if (reverse) {
            s->scramblrrr->resampled_file_bytes[i] = rev16th[i % len16th];
        }
        else {
            if (s->scramble_counter % 2 != 0)
                s->scramblrrr->resampled_file_bytes[i] =
                    s->samples[s->cur_sample]->resampled_file_bytes[i];
            else
            {
                if (dice2 < 7)
                    s->scramblrrr->resampled_file_bytes[(i + len16th * 2) % len] = third16th[i%len16th];
                else if (dice2 >= 8 && dice2 < 15)
                    s->scramblrrr->resampled_file_bytes[(i + len16th * 2) % len] = fourth16th[i%len16th];
                else if (dice2 >= 16 && dice2 < 22)
                    s->scramblrrr->resampled_file_bytes[(i + len16th * 2) % len] = seventh16th[i%len16th];
                // TODO -- maybe later
                //else if (dice2 >= 22 && dice2 < 87 && s->scramble_counter % 3 == 0) {
                //    printf("CRAZY SHIT!\n");
                //    copyfirsthalf = true;
                //    break;
                //}
                else
                    s->scramblrrr->resampled_file_bytes[(i + len16th * 2) % len] =
                        s->samples[s->cur_sample]->resampled_file_bytes[i];
            }
        }
    }
    if (copyfirsthalf)
    {
        int halflen = len / 2;
        for (int i = 0; i < halflen; i++)
           s->scramblrrr->resampled_file_bytes[i+halflen] = s->samples[s->cur_sample]->resampled_file_bytes[i]; 
    }

    if (mixr->debug_mode)
        printf("Looper: Max Gen: %d // Current Gen: %d\n", s->max_scramble_generation, s->scramble_generation);

    if (s->max_scramble_generation > 0 && s->scramble_generation >= s->max_scramble_generation)
    {
        printf("Max GEn! We outta here... peace\n");
        s->scramble_generation = 0;
        s->max_scramble_generation = 0;
        s->scramblrrr_mode = false;
    }
}
