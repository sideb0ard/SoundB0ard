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
    l->started = false;
    l->just_been_resampled = false;

    looper_add_sample(l, filename, loop_len);

    l->scramblrrr = looper_create_sample("none", loop_len);

    l->sound_generator.gennext = &looper_gennext;
    l->sound_generator.status = &looper_status;
    l->sound_generator.getvol = &looper_getvol;
    l->sound_generator.setvol = &looper_setvol;
    l->sound_generator.type = LOOPER_TYPE;

    for (int i = 0; i < MAX_SAMPLES_PER_LOOPER; i++) {
        l->sample_num_loops[i] = 1;
    }

    stereo_delay_prepare_for_play(&l->m_delay_fx);
    filter_moog_init(&l->m_filter);
    l->m_fc_control = 10000;
    moog_update((filter *)&l->m_filter);

    return l;
}

double looper_gennext(void *self)
// void looper_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
    looper *l = (looper *)self;
    double val = 0;

    // wait till start of loop to keep patterns synched
    if (!l->started) {
        if (mixr->sixteenth_note_tick % 16 == 0) {
            l->started = true;
        }
        else {
            return val;
        }
    }

    if (mixr->sixteenth_note_tick % 16 == 0 && l->resample_pending) {
        looper_resample_to_loop_size(l);
    }

    if (mixr->sixteenth_note_tick % 16 == 0 && l->change_loopsize_pending) {
        printf("PENDING LOOPSIZE FOUND! %d loops for loop num: %d\n",
               l->pending_loop_size, l->pending_loop_num);
        looper_change_loop_len(l, l->pending_loop_num, l->pending_loop_size);
        l->change_loopsize_pending = false;
    }

    if (l->stutter_mode &&
        (mixr->cur_sample % (l->scramblrrr->resampled_file_size / 16) == 0)) {
        if (mixr->debug_mode)
            printf("Stutututututter! Current: %d\n", l->stutter_current_16th);
        if (rand() % 100 > 60) {
            if (mixr->debug_mode)
                printf("Advancing stutter 16th..\n");
            l->stutter_current_16th++;
            if (l->stutter_current_16th == 16) {
                l->stutter_current_16th = 0;
                l->stutter_generation++;
                if (l->max_generation != 0 &&
                    l->stutter_generation == l->max_generation) {
                    printf("Max stutter reached - resettinggggzz..\n");
                    l->stutter_generation = 0;
                    l->max_generation = 0;
                    l->stutter_mode = false;
                }
            }
        }
    }

    if (l->scramblrrr_mode &&
        (mixr->cur_sample % (l->scramblrrr->resampled_file_size * 4) == 0)) {
        looper_scramble(l);
    }

    // resync after a resample/resize
    if (l->just_been_resampled && mixr->sixteenth_note_tick % 16 == 0) {
        printf("Resyncing after resample...zzzz\n");
        l->samples[l->cur_sample]->position = 0;
        l->scramblrrr->position = 0;
        l->just_been_resampled = false;
    }

    if (l->samples[l->cur_sample]->position == 0) {

        if (l->multi_sample_mode) {
            l->cur_sample_iteration--;
            if (l->cur_sample_iteration == 0) {
                l->cur_sample = (l->cur_sample + 1) % l->num_samples;
                l->cur_sample_iteration = l->sample_num_loops[l->cur_sample];
            }
        }
    }

    if (l->stutter_mode) {
        int len16th = l->scramblrrr->resampled_file_size / 16;
        int stutidx = (l->samples[l->cur_sample]->position % len16th) +
                      l->stutter_current_16th * len16th;
        val = l->samples[l->cur_sample]->resampled_file_bytes[stutidx];
        l->samples[l->cur_sample]->position++;
    }
    else if (l->scramblrrr_mode) {
        val = l->scramblrrr->resampled_file_bytes[l->scramblrrr->position++];
        l->samples[l->cur_sample]->position++; // keep increasing normal sample
    }
    else {
        val = l->samples[l->cur_sample]
                  ->resampled_file_bytes[l->samples[l->cur_sample]->position++];
    }

    if (l->scramblrrr->position == l->scramblrrr->resampled_file_size) {
        l->scramblrrr->position = 0;
    }

    if (l->samples[l->cur_sample]->position ==
        l->samples[l->cur_sample]->resampled_file_size) {
        l->samples[l->cur_sample]->position = 0;
    }

    if (val > 1 || val < -1)
        printf("BURNIE - looper OVERLOAD!\n");

    val = effector(&l->sound_generator, val);
    val = envelopor(&l->sound_generator, val);

    // update delay and filter
    l->m_filter.f.m_fc_control = l->m_fc_control;
    moog_set_qcontrol((filter *)&l->m_filter, l->m_q_control);
    moog_update((filter *)&l->m_filter);
    val = moog_gennext((filter *)&l->m_filter, val);

    stereo_delay_set_mode(&l->m_delay_fx, l->m_delay_mode);
    stereo_delay_set_delay_time_ms(&l->m_delay_fx, l->m_delay_time_msec);
    stereo_delay_set_feedback_percent(&l->m_delay_fx, l->m_feedback_pct);
    stereo_delay_set_delay_ratio(&l->m_delay_fx, l->m_delay_ratio);
    stereo_delay_set_wet_mix(&l->m_delay_fx, l->m_wet_mix);
    stereo_delay_update(&l->m_delay_fx);
    double out = 0.0;
    stereo_delay_process_audio(&l->m_delay_fx, &val, &val, &out, &out);

    return out * l->vol;
}

void looper_add_sample(looper *s, char *filename, int loop_len)
{
    printf("looper!, adding a new SAMPLE!\n");

    if (s->num_samples > MAX_SAMPLES_PER_LOOPER) {
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

    if (strncmp(filename, "none", 4) != 0) {
        sample_import_file_contents(fs, filename);
    }
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
    if (!snd_file) {
        printf("Err opening %s : %d\n", full_filename, sf_error(snd_file));
        return;
    }

    int bufsize = sf_info.channels * sf_info.frames;
    printf("Making buffer size of %d\n", bufsize);

    int *buffer = (int *)calloc(bufsize, sizeof(int));
    if (buffer == NULL) {
        perror("Ooft, memory issues, mate!\n");
        return;
    }

    sf_readf_int(snd_file, buffer, bufsize);
    sf_close(snd_file);

    fs->orig_file_bytes = buffer;
    fs->orig_file_size = bufsize;
    fs->samplerate = sf_info.samplerate;
    fs->channels = sf_info.channels;
}

void looper_resample_to_loop_size(looper *l)
{
    for (int i = 0; i < l->num_samples; i++) {
        sample_resample_to_loop_size(l->samples[i]);
    }
    sample_resample_to_loop_size(l->scramblrrr);
    l->just_been_resampled = true;
}

void looper_change_loop_len(looper *l, int sample_num, int loop_len)
{
    printf("LOOP CHANGE CALLED! sampleNUm: %d and looplen: %d\n", sample_num,
           loop_len);
    if (loop_len > 0 && sample_num < l->num_samples) {
        file_sample *fs = l->samples[sample_num];

        fs->loop_len = loop_len;
        sample_resample_to_loop_size(fs);

        l->scramblrrr->loop_len = loop_len;
        sample_resample_to_loop_size(l->scramblrrr);
    }
}

void sample_resample_to_loop_size(file_sample *fs)
{
    printf("BUFSIZE is %d\n", fs->orig_file_size);
    printf("CHANNELS is %d\n", fs->channels);

    int loop_len_in_samples = mixr->samples_per_midi_tick * PPBAR * fs->loop_len;

    double *resampled_file_bytes =
        (double *)calloc(loop_len_in_samples, sizeof(double));
    if (resampled_file_bytes == NULL) {
        printf("Memory barf in looper resample\n");
        return;
    }

    if (strncmp("none", fs->filename, 4) != 0)
    {
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
}

void looper_status(void *self, wchar_t *status_string)
{
    looper *l = (looper *)self;
    swprintf(status_string, MAX_PS_STRING_SZ, WCOOL_COLOR_GREEN
             "[LOOPER] Vol: %.2lf MultiMode: %s Current Sample: %d "
             "ScramblrrrMode: %s ScrambleGen: %d StutterMode: %s "
             "Stutter Gen: %d MaxGen: %d",
             l->vol, l->multi_sample_mode ? "true" : "false", l->cur_sample,
             l->scramblrrr_mode ? "true" : "false", l->scramble_generation,
             l->stutter_mode ? "true" : "false", l->stutter_generation,
             l->max_generation);
    int strlen_left = MAX_PS_STRING_SZ - wcslen(status_string);
    wchar_t looper_details[strlen_left];
    for (int i = 0; i < l->num_samples; i++) {
        swprintf(looper_details, 128,
                 L"\n      [%d] %s - looplen: %d numloops: %d", i,
                 basename(l->samples[i]->filename), l->samples[i]->loop_len,
                 l->sample_num_loops[i]);
        wcslcat(status_string, looper_details, strlen_left);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
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
    if (v < 0.0 || v > 1.0) {
        return;
    }
    l->vol = v;
}

void looper_change_num_loops(looper *s, int sample_num, int num_loops)
{
    if (sample_num < s->num_samples && num_loops > 0) {
        s->sample_num_loops[sample_num] = num_loops;
    }
}

void looper_set_scramble_mode(looper *s, bool b)
{
    s->scramblrrr_mode = b;
    s->scramble_counter = 0;
    s->scramble_generation = 0;
    if (s->scramblrrr_mode == true) {
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
}

void looper_scramble(looper *s)
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
        third16th[i] = scrambled[i + len16th * 3];
        fourth16th[i] = scrambled[i + len16th * 4];
        seventh16th[i] = scrambled[i + len16th * 7];
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
            else {
                if (dice2 < 7)
                    s->scramblrrr
                        ->resampled_file_bytes[(i + len16th * 2) % len] =
                        third16th[i % len16th];
                else if (dice2 >= 8 && dice2 < 15)
                    s->scramblrrr
                        ->resampled_file_bytes[(i + len16th * 2) % len] =
                        fourth16th[i % len16th];
                else if (dice2 >= 16 && dice2 < 22)
                    s->scramblrrr
                        ->resampled_file_bytes[(i + len16th * 2) % len] =
                        seventh16th[i % len16th];
                else if (dice2 >= 42 && dice2 < 87 &&
                         s->scramble_counter % 3 == 0) {
                    copyfirsthalf = true;
                    break;
                }
                else
                    s->scramblrrr
                        ->resampled_file_bytes[(i + len16th * 2) % len] =
                        s->samples[s->cur_sample]->resampled_file_bytes[i];
            }
        }
    }
    if (copyfirsthalf) {
        int halflen = len / 2;
        for (int i = 0; i < halflen; i++)
            s->scramblrrr->resampled_file_bytes[i + halflen] =
                s->samples[s->cur_sample]->resampled_file_bytes[i];
        copyfirsthalf = false;
    }

    if (mixr->debug_mode)
        printf("Looper: Max Gen: %d // Current Gen: %d\n", s->max_generation,
               s->scramble_generation);

    if (s->max_generation > 0 && s->scramble_generation >= s->max_generation) {
        printf("Max GEn! We outta here... peace\n");
        looper_set_scramble_mode(s, false);
    }
}

// TODO - move this to SOUND GENERATOR
void looper_parse_midi(looper *s, unsigned int data1, unsigned int data2)
{
    printf("YA BEEZER, MIDI DRUM SEQUENCER!\n");

    double scaley_val = 0.;
    switch (data1) {
    case 1:
        scaley_val = scaleybum(0, 127, FILTER_FC_MIN, FILTER_FC_MAX, data2);
        printf("Filter FREQ Control! %f\n", scaley_val);
        s->m_fc_control = scaley_val;
        break;
    case 2:
        scaley_val = scaleybum(0, 127, 1, 10, data2);
        printf("Filter Q Control! %f\n", scaley_val);
        s->m_q_control = scaley_val;
        break;
    case 3:
        scaley_val = scaleybum(0, 127, 1, 6, data2);
        printf("SWIIIiiing!! %f\n", scaley_val);
        s->swing_setting = scaley_val;
        break;
    case 4:
        scaley_val = scaleybum(0, 127, 0., 1., data2);
        printf("Volume! %f\n", scaley_val);
        s->vol = scaley_val;
        break;
    case 5:
        scaley_val = scaleybum(0, 127, 0, 2000, data2);
        printf("Delay Feedback Msec %f!\n", scaley_val);
        s->m_delay_time_msec = scaley_val;
        break;
    case 6:
        scaley_val = scaleybum(0, 127, 20, 100, data2);
        printf("Delay Feedback Pct! %f\n", scaley_val);
        s->m_feedback_pct = scaley_val;
        break;
    case 7:
        scaley_val = scaleybum(0, 127, -0.9, 0.9, data2);
        printf("Delay Ratio! %f\n", scaley_val);
        s->m_delay_ratio = scaley_val;
        break;
    case 8:
        scaley_val = scaleybum(0, 127, 0, 1, data2);
        printf("DELAY Wet mix %f!\n", scaley_val);
        s->m_wet_mix = scaley_val;
        break;
    case 9: // PAD 5
        printf("Toggle Delay Mode!\n");
        s->m_delay_mode = (++s->m_delay_mode) % MAX_NUM_DELAY_MODE;
        break;
    default:
        break;
    }
}

void looper_del_self(looper *s)
{
    for (int i = 0; i < s->num_samples; i++)
    {
        printf("Dleeeting samples\n");
        file_sample_free(s->samples[i]);
    }
    free(s);
}

void file_sample_free(file_sample *fs)
{
    if (strncmp(fs->filename, "none", 4) != 0) {
        printf("Dleeeting original file bytes\n");
        free(fs->orig_file_bytes);
        printf("Dleeeting resampeld file bytes\n");
        free(fs->resampled_file_bytes);
    }
    printf("Dleeeting filename \n");
    free(fs->filename);
    free(fs);
}
