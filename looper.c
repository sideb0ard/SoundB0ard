#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defjams.h"
#include "looper.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

looper *new_looper(char *filename, double loop_len)
{
    looper *l = (looper *)calloc(1, sizeof(looper));
    l->vol = 0.7;
    l->sound_generator.active = true;
    l->started = false;
    l->just_been_resampled = false;
    l->loop_len = loop_len;

    looper_add_sample(l, filename, loop_len);

    l->scramblrrr = looper_create_sample("none", loop_len);

    l->sound_generator.gennext = &looper_gennext;
    l->sound_generator.status = &looper_status;
    l->sound_generator.getvol = &looper_getvol;
    l->sound_generator.setvol = &looper_setvol;
    l->sound_generator.start = &looper_start;
    l->sound_generator.stop = &looper_stop;
    l->sound_generator.get_num_tracks = &looper_get_num_tracks;
    l->sound_generator.make_active_track = &looper_make_active_track;
    l->sound_generator.self_destruct = &looper_del_self;
    l->sound_generator.type = LOOPER_TYPE;

    for (int i = 0; i < MAX_SAMPLES_PER_LOOPER; i++)
    {
        l->sample_num_loops[i] = 1;
    }

    return l;
}

stereo_val looper_gennext(void *self)
{
    looper *l = (looper *)self;
    stereo_val val = {0, 0};

    if (!l->sound_generator.active)
        return val;

    // wait till start of loop to keep patterns synched
    if (!l->started)
    {
        if (mixr->start_of_loop)
        {
            printf("Starting now! 16th tick is %d\n",
                   mixr->sixteenth_note_tick % 16);
            l->started = true;
        }
        else
        {
            return val;
        }
    }

    if (mixr->start_of_loop && l->resample_pending)
    {
        looper_resample_to_loop_size(l);
    }

    if (mixr->start_of_loop && l->change_loopsize_pending)
    {
        printf("PENDING LOOPSIZE FOUND! %.2f loops for loop num: %d\n",
               l->pending_loop_size, l->pending_loop_num);
        looper_change_loop_len(l, l->pending_loop_num, l->pending_loop_size);
        l->change_loopsize_pending = false;
    }

    // UPDATE MODES IF NEEDED
    // if (mixr->start_of_loop)
    // {
    //     if (l->stutter_mode)
    //     {
    //         if (l->stutter_every_n_loops > 0)
    //         {
    //             if (l->stutter_generation % l->stutter_every_n_loops == 0)
    //             {
    //                 l->stutter_active = true;
    //             }
    //             else
    //             {
    //                 l->stutter_active = false;
    //             }
    //         }
    //         else if (l->max_generation > 0)
    //         {
    //             if (l->stutter_generation >= l->max_generation)
    //             {
    //                 looper_set_stutter_mode(l, false);
    //             }
    //         }
    //         l->stutter_generation++;
    //     }

    //     if (l->scramblrrr_mode)
    //     {
    //         if (l->scramble_every_n_loops > 0)
    //         {
    //             if (l->scramble_generation % l->scramble_every_n_loops == 0)
    //                 l->scramblrrr_active = true;
    //             else
    //                 l->scramblrrr_active = false;
    //         }
    //         else if (l->max_generation > 0)
    //         {
    //             if (l->scramble_generation >= l->max_generation)
    //             {
    //                 looper_set_scramble_mode(l, false);
    //             }
    //         }

    //         if (l->scramblrrr_active)
    //         {
    //             looper_scramble(l);
    //         }

    //         l->scramble_generation++;
    //     }
    // }

    //if (l->stutter_active)
    //{
    //    if (mixr->cur_sample % (l->scramblrrr->resampled_file_size / 16) == 0)
    //    {
    //        if (mixr->debug_mode)
    //            printf("Stutututututter! Current: %d\n",
    //                   l->stutter_current_16th);

    //        if (rand() % 100 > 60)
    //        {
    //            if (mixr->debug_mode)
    //                printf("Advancing stutter 16th..\n");
    //            l->stutter_current_16th++;
    //            if (l->stutter_current_16th == 16)
    //            {
    //                l->stutter_current_16th = 0;
    //            }
    //        }
    //    }
    //}

    // resync after a resample/resize
    if (l->just_been_resampled && mixr->sixteenth_note_tick % 16 == 0)
    {
        printf("Resyncing after resample...zzzz\n");
        l->samples[l->cur_sample]->position = 0;
        l->scramblrrr->position = 0;
        l->just_been_resampled = false;
    }

    if (l->samples[l->cur_sample]->position == 0)
    {

        if (l->multi_sample_mode)
        {
            l->cur_sample_iteration--;
            if (l->cur_sample_iteration == 0)
            {
                l->cur_sample = (l->cur_sample + 1) % l->num_samples;
                l->cur_sample_iteration = l->sample_num_loops[l->cur_sample];
            }
        }
    }

    if (l->stutter_active)
    {
        int len16th = l->scramblrrr->resampled_file_size / 16;
        int stutidx = (l->samples[l->cur_sample]->position % len16th) +
                      l->stutter_current_16th * len16th;
        val.left = l->samples[l->cur_sample]->resampled_file_bytes[stutidx];
        val.right = l->samples[l->cur_sample]->resampled_file_bytes[stutidx];
    }
    else if (l->scramblrrr_active)
    {
        val.left =
            l->scramblrrr->resampled_file_bytes[l->scramblrrr->position++];
        val.right =
            l->scramblrrr->resampled_file_bytes[l->scramblrrr->position++];
    }
    else
    { // THE NORM __
        val.left =
            l->samples[l->cur_sample]
                ->resampled_file_bytes[l->samples[l->cur_sample]->position];

        if (l->samples[l->cur_sample]->channels > 0)
        {
            val.right =
                l->samples[l->cur_sample]
                    ->resampled_file_bytes[l->samples[l->cur_sample]->position];
        }
        else
        {
            val.right =
                l->samples[l->cur_sample]
                    ->resampled_file_bytes[l->samples[l->cur_sample]->position +
                                           1];
        }
    }

    if (l->scramblrrr->position == l->scramblrrr->resampled_file_size)
    {
        l->scramblrrr->position = 0;
    }

    l->samples[l->cur_sample]->position++;
    if (l->samples[l->cur_sample]->channels > 0)
        l->samples[l->cur_sample]->position++;

    if (l->samples[l->cur_sample]->position ==
        l->samples[l->cur_sample]->resampled_file_size)
    {
        l->samples[l->cur_sample]->position = 0;
    }

    // if (val > 1 || val < -1)
    //    printf("BURNIE - looper OVERLOAD!\n");

    double scratch = 0;
    scratch = effector(&l->sound_generator, val.left);
    scratch = envelopor(&l->sound_generator, val.left);
    val.left = scratch * l->vol;

    if (l->samples[l->cur_sample]->channels > 0)
    {
        scratch = effector(&l->sound_generator, val.right);
        scratch = envelopor(&l->sound_generator, val.right);
    }
    val.right = scratch * l->vol;

    return val;
}

void looper_add_sample(looper *s, char *filename, int loop_len)
{
    printf("looper!, adding a new SAMPLE!\n");

    if (s->num_samples > MAX_SAMPLES_PER_LOOPER)
    {
        printf("Already have max num samples\n");
        return;
    }

    file_sample *fs = looper_create_sample(filename, loop_len);
    s->samples[s->num_samples++] = fs;
    printf("done adding SAMPLE\n");
}

file_sample *looper_create_sample(char *filename, int loop_len)
{
    file_sample *fs = (file_sample *)calloc(1, sizeof(file_sample));
    sample_set_file_name(fs, filename);
    fs->position = 0;
    fs->loop_len = loop_len;

    if (strncmp(filename, "none", 4) != 0)
        sample_import_file_contents(fs, filename);

    sample_resample_to_loop_size(fs);

    return fs;
}

void sample_set_file_name(file_sample *fs, char *filename)
{
    int fslen = strlen(filename);
    fs->filename = (char *)calloc(1, fslen + 1);
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
    if (!snd_file)
    {
        printf("Err opening %s : %d\n", full_filename, sf_error(snd_file));
        return;
    }

    int bufsize = sf_info.channels * sf_info.frames;
    printf("Making buffer size of %d\n", bufsize);

    double *buffer = (double *)calloc(bufsize, sizeof(double));
    if (buffer == NULL)
    {
        perror("Ooft, memory issues, mate!\n");
        sf_close(snd_file);
        return;
    }

    sf_readf_double(snd_file, buffer, bufsize);
    sf_close(snd_file);

    fs->orig_file_bytes = buffer;
    fs->orig_file_size = bufsize;
    fs->samplerate = sf_info.samplerate;
    fs->channels = sf_info.channels;
    fs->frames = sf_info.frames;
}

void looper_resample_to_loop_size(looper *l)
{
    for (int i = 0; i < l->num_samples; i++)
    {
        sample_resample_to_loop_size(l->samples[i]);
    }
    sample_resample_to_loop_size(l->scramblrrr);
    l->just_been_resampled = true;
}

void looper_change_loop_len(looper *l, int sample_num, double loop_len)
{
    printf("LOOP CHANGE CALLED! sampleNUm: %d and looplen: %.2f\n", sample_num,
           loop_len);
    if (loop_len > 0 && sample_num < l->num_samples)
    {
        file_sample *fs = l->samples[sample_num];

        fs->loop_len = loop_len;
        sample_resample_to_loop_size(fs);

        l->scramblrrr->loop_len = loop_len;
        sample_resample_to_loop_size(l->scramblrrr);
    }
}

void sample_resample_to_loop_size(file_sample *fs)
{
    printf("FILe is %s\n", fs->filename);
    printf("BUFSIZE is %d\n", fs->orig_file_size);
    printf("FRAMES is %d\n", fs->frames);
    printf("CHANNELS is %d\n", fs->channels);
    printf("LOOPLEN is %.2f\n", fs->loop_len);

    int resampled_buffer_size = mixr->loop_len_in_samples * fs->loop_len * fs->channels;

    double *resampled_file_bytes =
        (double *)calloc(resampled_buffer_size, sizeof(double));

    if (resampled_file_bytes == NULL)
    {
        printf("Memory barf in looper resample\n");
        return;
    }

    if (strncmp("none", fs->filename, 4) != 0)
    {
        double *table = fs->orig_file_bytes;
        double bufsize = fs->orig_file_size;

        double position = 0;
        double incr = bufsize / resampled_buffer_size;
        printf("INCREMENT! %f\n", incr);

        for (int i = 0; i < resampled_buffer_size; i++)
        {
            int base_index = (int)position;
            unsigned long next_index = base_index + fs->channels;
            double frac, slope, val;

            frac = position - base_index;
            val = table[base_index];
            slope = table[next_index] - val;

            val += (frac * slope);
            position += incr;

            if (position >= bufsize)
            {
                printf("POSITION: %f // BUFSIZR: %f My job here is "
                       "done\n",
                       position, bufsize);
                break;
            }

            resampled_file_bytes[i] = val;

            //if (fs->channels > 1)
            //{
            //    base_index++;
            //    next_index++;
            //    double right_val = table[base_index];
            //    slope = table[next_index] - right_val;
            //    right_val += (frac * slope);
            //    resampled_file_bytes[++i] = right_val;
            //}
        }

        bool is_previous_buffer =
            fs->resampled_file_bytes != NULL ? true : false;
        if (is_previous_buffer)
        {
            double *oldbuf = fs->resampled_file_bytes;
            int old_relative_position =
                (100 / fs->resampled_file_size) * fs->position;

            fs->position = (resampled_buffer_size / 100) * old_relative_position;
            free(oldbuf);
        }
    }

    fs->resampled_file_bytes = resampled_file_bytes;
    fs->resampled_file_size = resampled_buffer_size;
}

void looper_status(void *self, wchar_t *status_string)
{
    looper *l = (looper *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             L"[LOOPER] Vol:%.2lf MultiMode:%s CurSample:%d MaxGen:%d  "
             "Active:%s Position:%d SampleLength:%d\n"
             "      ScramblrrrMode:%s ScrambleGen:%d ScrambleEveryN:%d "
             "StutterMode:%s StutterGen:%d StutterEveryN:%d\n",
             l->vol, l->multi_sample_mode ? "true" : "false", l->cur_sample,
             l->max_generation, l->sound_generator.active ? " true" : "false",
             l->samples[l->cur_sample]->position,
             l->samples[l->cur_sample]->resampled_file_size,
             l->scramblrrr_mode ? "true" : "false", l->scramble_generation,
             l->scramble_every_n_loops, l->stutter_mode ? "true" : "false",
             l->stutter_generation, l->stutter_every_n_loops);

    int strlen_left = MAX_PS_STRING_SZ - wcslen(status_string);
    wchar_t looper_details[strlen_left];
    for (int i = 0; i < l->num_samples; i++)
    {
        swprintf(looper_details, 128,
                 L"\n      [%d] %s - looplen:%.2f numloops:%d stereo:%d", i,
                 l->samples[i]->filename, l->samples[i]->loop_len,
                 l->sample_num_loops[i], l->samples[i]->channels > 1 ? 1 : 0);
        wcslcat(status_string, looper_details, strlen_left);
    }
}

void looper_start(void *self)
{
    looper *l = (looper *)self;
    l->sound_generator.active = true;
    // l->started = false;
    // l->samples[l->cur_sample]->position = 0;
    // l->scramblrrr->position = 0;
}

void looper_stop(void *self)
{
    looper *l = (looper *)self;
    l->sound_generator.active = false;
    l->started = false;
    l->samples[l->cur_sample]->position = 0;
    l->scramblrrr->position = 0;
}

void looper_set_multi_sample_mode(looper *s, bool multi)
{
    s->multi_sample_mode = multi;
    s->cur_sample_iteration = s->sample_num_loops[s->cur_sample];
}

void looper_switch_sample(looper *s, int sample_num)
{
    if (sample_num < s->num_samples)
        s->cur_sample = sample_num;
}

double looper_getvol(void *self)
{
    looper *l = (looper *)self;
    return l->vol;
}

void looper_setvol(void *self, double v)
{
    looper *l = (looper *)self;
    if (v < 0.0 || v > 1.0)
    {
        return;
    }
    l->vol = v;
}

void looper_change_num_loops(looper *s, int sample_num, int num_loops)
{
    if (sample_num < s->num_samples && num_loops > 0)
    {
        s->sample_num_loops[sample_num] = num_loops;
    }
}

void looper_set_scramble_mode(looper *s, bool b)
{
    s->scramblrrr_active = b;
    s->scramblrrr_mode = b;
    s->scramble_counter = 0;
    s->scramble_generation = 0;
    if (s->scramblrrr_mode == true)
    {
        int len = s->samples[s->cur_sample]->resampled_file_size;
        for (int i = 0; i < len; i++)
            s->scramblrrr->resampled_file_bytes[i] =
                s->samples[s->cur_sample]->resampled_file_bytes[i];
        s->scramblrrr->position = s->samples[s->cur_sample]->position;
    }
    printf("Changed looper scramblrrr mode to %s\n",
           s->scramblrrr_mode ? "true" : "false");
}

void looper_set_max_generation(looper *s, int max) { s->max_generation = max; }

void looper_set_stutter_mode(looper *s, bool b)
{
    s->stutter_generation = 0;
    s->stutter_mode = b;
    s->stutter_active = b;
}

void looper_scramble(looper *s)
{
    double *scrambled = s->scramblrrr->resampled_file_bytes;
    int len = s->scramblrrr->resampled_file_size;
    int len16th;
    // if (s->scramblrrr->loop_len != 0)
    //    len16th = len / s->scramblrrr->loop_len / 16;
    // else
    len16th = mixr->loop_len_in_samples / 16;

    // take a copy of the first 16th that we can randomly inject below.
    double first16th[len16th];
    // lets grab a reverse copy too!
    double rev16th[len16th];
    // let's go all in!
    double third16th[len16th];
    double fourth16th[len16th];
    double seventh16th[len16th];

    for (int i = 0; i < len16th; i++)
    {
        first16th[i] = scrambled[i];
        rev16th[(len16th - 1) - i] = scrambled[i];
        third16th[i] = scrambled[i + len16th * 3];
        fourth16th[i] = scrambled[i + len16th * 4];
        seventh16th[i] = scrambled[i + len16th * 7];
    }

    s->scramble_counter = 0;
    bool should_run = false;
    bool yolo = false;
    bool reverse = false;
    bool copy_first_half = false;
    bool silence_last_quarter = false;
    int PCT_CHANCE_YOLO = 0;
    int PCT_CHANCE_REV = 0;
    int PCT_CHANCE_COPY_FIRST_HALF = 0;
    int PCT_CHANCE_SILENCE_LAST_QUARTER = 0;
    int PCT_CHANCE_16TH = 0;

    if (s->scramble_every_n_loops > 0)
    { // this is more punchy and extreme as it happens only once every n
        should_run = true;
        PCT_CHANCE_YOLO = 45;
        PCT_CHANCE_REV = 25;
        PCT_CHANCE_COPY_FIRST_HALF = 22;
        PCT_CHANCE_SILENCE_LAST_QUARTER = 1;
        PCT_CHANCE_16TH = 7;
    }
    else if (mixr->cur_sample % (s->scramblrrr->resampled_file_size * 4) == 0)
    { // this is the evolver
        should_run = true;
        PCT_CHANCE_YOLO = 25;
        PCT_CHANCE_REV = 25;
        PCT_CHANCE_COPY_FIRST_HALF = 10;
        PCT_CHANCE_SILENCE_LAST_QUARTER = 5;
        PCT_CHANCE_16TH = 7;
    }

    if (should_run)
    {
        bool we_third16th = false;
        bool we_fourth16th = false;
        bool we_seventh16th = false;

        for (int i = 0; i < len; i++)
        {

            if (i % len16th == 0)
            {
                s->scramble_counter++;
                if (yolo || reverse)
                {
                    yolo = false;
                    reverse = false;
                }
                else
                {
                    if (rand() % 100 < PCT_CHANCE_YOLO)
                        yolo = true;
                    else if (rand() % 100 < PCT_CHANCE_REV)
                        reverse = true;
                }

                we_third16th = false;
                we_fourth16th = false;
                we_seventh16th = false;
                if (rand() % 100 < PCT_CHANCE_16TH)
                    we_third16th = true;
                else if (rand() % 100 < PCT_CHANCE_16TH)
                    we_fourth16th = true;
                else if (rand() % 100 < PCT_CHANCE_16TH)
                    we_seventh16th = true;
            }

            if (yolo)
            {
                scrambled[i] = first16th[i % len16th];
            }
            else if (reverse)
            {
                scrambled[i] = rev16th[i % len16th];
            }
            else
            {
                if (s->scramble_counter % 2 == 0)
                    // s->scramblrrr->resampled_file_bytes[i] =
                    scrambled[i] =
                        s->samples[s->cur_sample]->resampled_file_bytes[i];
                else
                {
                    if (we_third16th)
                        scrambled[(i + len16th * 2) % len] =
                            third16th[i % len16th];
                    else if (we_fourth16th)
                        scrambled[(i + len16th * 2) % len] =
                            fourth16th[i % len16th];
                    else if (we_seventh16th)
                        scrambled[(i + len16th * 2) % len] =
                            seventh16th[i % len16th];
                    else
                        scrambled[(i + len16th * 2) % len] =
                            s->samples[s->cur_sample]->resampled_file_bytes[i];
                }
            }
        }

        copy_first_half =
            rand() % 100 < PCT_CHANCE_COPY_FIRST_HALF ? true : false;
        silence_last_quarter = rand() % 100 < PCT_CHANCE_YOLO ? true : false;

        if (copy_first_half)
        {
            int halflen = len / 2;
            for (int i = 0; i < halflen; i++)
                scrambled[i + halflen] =
                    s->samples[s->cur_sample]->resampled_file_bytes[i];
        }

        if (silence_last_quarter)
        {
            int quartlen = len / 4;
            for (int i = quartlen * 3; i < len; i++)
                scrambled[i] = 0;
        }
    }

    if (mixr->debug_mode)
        printf("Looper: Max Gen: %d // Current Gen: %d\n", s->max_generation,
               s->scramble_generation);

    if (s->max_generation > 0 && s->scramble_generation >= s->max_generation)
    {
        printf("Max GEn! We outta here... peace\n");
        looper_set_scramble_mode(s, false);
    }
}

void looper_del_self(void *self)
{
    looper *l = (looper *)self;
    for (int i = 0; i < l->num_samples; i++)
    {
        printf("Dleeeting samples\n");
        file_sample_free(l->samples[i]);
    }
    free(l);
}

void file_sample_free(file_sample *fs)
{
    if (strncmp(fs->filename, "none", 4) != 0)
    {
        printf("Dleeeting original file bytes\n");
        free(fs->orig_file_bytes);
        printf("Dleeeting resampeld file bytes\n");
        free(fs->resampled_file_bytes);
    }
    printf("Dleeeting filename \n");
    free(fs->filename);
    free(fs);
}

void looper_make_active_track(void *self, int track_num)
{
    looper *l = (looper *)self;
    l->cur_sample = track_num;
}

int looper_get_num_tracks(void *self)
{
    looper *l = (looper *)self;
    return l->num_samples;
}
