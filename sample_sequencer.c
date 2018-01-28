#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include "defjams.h"
#include "mixer.h"
#include "sample_sequencer.h"
#include "sequencer_utils.h"
#include "step_sequencer.h"
#include "utils.h"

extern wchar_t *sparkchars;
extern mixer *mixr;

sample_sequencer *new_sample_seq(char *filename)
{
    sample_sequencer *seq =
        (sample_sequencer *)calloc(1, sizeof(sample_sequencer));
    seq_init(&seq->m_seq);

    sample_seq_import_file(seq, filename);

    seq->sound_generator.active = true;
    seq->started = false;

    seq->vol = 0.7;

    seq->sound_generator.gennext = &sample_seq_gennext;
    seq->sound_generator.status = &sample_seq_status;
    seq->sound_generator.getvol = &sample_seq_getvol;
    seq->sound_generator.setvol = &sample_seq_setvol;
    seq->sound_generator.start = &sample_start;
    seq->sound_generator.stop = &sample_stop;
    seq->sound_generator.get_num_tracks = &sample_seq_get_num_tracks;
    seq->sound_generator.make_active_track = &sample_seq_make_active_track;
    seq->sound_generator.self_destruct = &sampleseq_del_self;
    seq->sound_generator.event_notify = &sample_seq_event_notify;
    seq->sound_generator.get_pattern = &sample_seq_get_pattern;
    seq->sound_generator.set_pattern = &sample_seq_set_pattern;
    seq->sound_generator.is_valid_pattern = &sample_sequencer_is_valid_pattern;
    seq->sound_generator.type = SEQUENCER_TYPE;

    return seq;
}

bool sample_sequencer_is_valid_pattern(void *self, int pattern_num)
{
    sample_sequencer *seq = (sample_sequencer *)self;
    printf("INSIDE SAMPLE SEQ IS VALID - haz %d patterns\n", seq->m_seq.num_patterns);
    return seq_is_valid_pattern_num(&seq->m_seq, pattern_num);
}

parceled_pattern sample_seq_get_pattern(void *self, int pattern_num)
{
    sample_sequencer *seq = (sample_sequencer *)self;
    return seq_get_pattern(&seq->m_seq, pattern_num);
}
void sample_seq_set_pattern(void *self, int pattern_num, parceled_pattern pattern)
{
    sample_sequencer *seq = (sample_sequencer *)self;
    return seq_set_pattern(&seq->m_seq, pattern_num, pattern);
}

void sample_seq_import_file(sample_sequencer *seq, char *filename)
{
    audio_buffer_details deetz = import_file_contents(&seq->buffer, filename);
    strcpy(seq->filename, deetz.filename);
    seq->bufsize = deetz.buffer_length;
    seq->buffer_pitch = 1.0;
    seq->samplerate = deetz.sample_rate;
    seq->channels = deetz.num_channels;
    sample_sequencer_reset_samples(seq);
}

void sample_sequencer_reset_samples(sample_sequencer *seq)
{
    for (int i = 0; i < MAX_CONCURRENT_SAMPLES; i++)
    {
        seq->samples_now_playing[i] = -1;
    }
    for (int i = 0; i < PPBAR; i++)
    {
        seq->sample_positions[i].position = 0;
        seq->sample_positions[i].audiobuffer_cur_pos = 0.;
        seq->sample_positions[i].audiobuffer_inc = 1.0;
        seq->sample_positions[i].playing = 0;
        seq->sample_positions[i].played = 0;
        seq->sample_positions[i].speed = 1;
        seq->sample_positions[i].speed = 1;
    }
}

void sample_seq_event_notify(void *self, unsigned int event_type)
{
    sample_sequencer *seq = (sample_sequencer *)self;

    if (!seq->sound_generator.active)
        return;

    int idx;
    switch (event_type)
    {
    case (TIME_START_OF_LOOP_TICK):
        seq->started = true;
        if (seq->morph)
        {
            if (seq->morph_every_n_loops > 0)
            {
                if (seq->morph_generation % seq->morph_every_n_loops == 0)
                    sample_sequencer_morph(seq);
                else
                    sample_sequencer_morph_restore(seq);
            }
            else
                sample_sequencer_morph(seq);
            seq->morph_generation++;
        }
        break;
    case (TIME_SIXTEENTH_TICK):
        if (seq->started)
            seq_tick(&seq->m_seq);
        break;
    case (TIME_MIDI_TICK):
        if (seq->started)
        {
            idx = mixr->timing_info.midi_tick % PPBAR;
            if (seq->m_seq.patterns[seq->m_seq.cur_pattern][idx])

            {
                int seq_position = get_a_sample_seq_position(seq);
                if (seq_position != -1)
                {
                    seq->samples_now_playing[seq_position] = idx;
                }
            }
        }
        break;
    }
}

stereo_val sample_seq_gennext(void *self)
{
    sample_sequencer *seq = (sample_sequencer *)self;
    double left_val = 0;
    double right_val = 0;

    // wait till start of loop to keep patterns synched
    if (!seq->started)
        return (stereo_val){0, 0};

    for (int i = 0; i < MAX_CONCURRENT_SAMPLES; i++)
    {
        if (seq->samples_now_playing[i] != -1)
        {
            int cur_sample_midi_tick = seq->samples_now_playing[i];
            int idx =
                seq->sample_positions[cur_sample_midi_tick].audiobuffer_cur_pos;
            left_val += seq->buffer[idx] *
                        seq->m_seq.pattern_position_amp[seq->m_seq.cur_pattern]
                                                       [cur_sample_midi_tick];
            if (seq->channels == 2)
            {
                right_val +=
                    seq->buffer[idx + 1] *
                    seq->m_seq.pattern_position_amp[seq->m_seq.cur_pattern]
                                                   [cur_sample_midi_tick];
            }
            seq->sample_positions[cur_sample_midi_tick].audiobuffer_cur_pos =
                seq->sample_positions[cur_sample_midi_tick]
                    .audiobuffer_cur_pos +
                (seq->channels * (seq->buffer_pitch));

            if ((int)seq->sample_positions[cur_sample_midi_tick]
                    .audiobuffer_cur_pos >= seq->bufsize)
            { // end of playback - so reset
                seq->samples_now_playing[i] = -1;
                seq->sample_positions[cur_sample_midi_tick]
                    .audiobuffer_cur_pos = 0;
            }
        }
    }

    left_val = effector(&seq->sound_generator, left_val);
    left_val = envelopor(&seq->sound_generator, left_val);
    if (seq->channels == 2)
    {
        right_val = effector(&seq->sound_generator, right_val);
        right_val = effector(&seq->sound_generator, right_val);
    }
    else
        right_val = left_val;

    return (stereo_val){.left = left_val * seq->vol,
                        .right = right_val * seq->vol};
}

sample_sequencer *new_sample_seq_from_char_pattern(char *filename,
                                                   char *pattern)
{
    sample_sequencer *seq = new_sample_seq(filename);
    pattern_char_to_pattern(&seq->m_seq, pattern,
                            seq->m_seq.patterns[seq->m_seq.num_patterns++]);
    return seq;
}

void sample_seq_status(void *self, wchar_t *status_string)
{
    printf("STATUS!!!\n");
    sample_sequencer *seq = (sample_sequencer *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             L"[SAMPLE SEQ] \"%s\" Vol:%.2lf Active:%s Morph:%s Pitch:%.2f",
             seq->filename, seq->vol,
             seq->sound_generator.active ? "true" : "false",
             seq->morph ? "true" : "false", seq->buffer_pitch);
    wchar_t seq_status_string[MAX_PS_STRING_SZ];
    memset(seq_status_string, 0, MAX_PS_STRING_SZ);
    seq_status(&seq->m_seq, seq_status_string);
    wcscat(status_string, seq_status_string);
    wcscat(status_string, WANSI_COLOR_RESET);
    printf("DONESTATUS!!!\n");
}

double sample_seq_getvol(void *self)
{
    sample_sequencer *seq = (sample_sequencer *)self;
    return seq->vol;
}

void sample_seq_setvol(void *self, double v)
{
    sample_sequencer *seq = (sample_sequencer *)self;
    if (v < 0.0 || v > 1.0)
    {
        return;
    }
    seq->vol = v;
}

void sampleseq_del_self(void *self)
{
    sample_sequencer *s = (sample_sequencer *)self;
    printf("Deleting sample buffer\n");
    free(s->buffer);
    printf("Deleting SAMPLESEQUENCER SELF- bye!\n");
    free(s);
}

int sample_seq_get_num_tracks(void *self)
{
    sample_sequencer *s = (sample_sequencer *)self;
    return s->m_seq.num_patterns;
}

void sample_seq_make_active_track(void *self, int track_num)
{
    sample_sequencer *s = (sample_sequencer *)self;
    s->m_seq.cur_pattern = track_num;
}

int get_a_sample_seq_position(sample_sequencer *ss)
{
    for (int i = 0; i < MAX_CONCURRENT_SAMPLES; i++)
        if (ss->samples_now_playing[i] == -1)
            return i;
    return -1;
}

void sample_start(void *self)
{
    printf("START SAMP!\n");
    sample_sequencer *s = (sample_sequencer *)self;
    s->sound_generator.active = true;
    // sample_sequencer_reset_samples(s);
}

void sample_stop(void *self)
{
    printf("STOP SAMP!\n");
    sample_sequencer *s = (sample_sequencer *)self;
    s->sound_generator.active = false;
    sample_sequencer_reset_samples(s);
}

void sample_sequencer_morph(sample_sequencer *seq)
{
    int multi1 = (seq->morph_generation % 3) + 1;
    int multi2 = (seq->morph_generation % 4) + 1;

    const int quart = PPBAR / 4;
    for (int i = 0; i < quart; i++)
    {
        if (rand() % 2 == 0)
            seq->sample_positions[i].speed = 1 * multi1;
        else
            seq->sample_positions[i].speed = 1 * multi2;
    }
    for (int i = quart; i < quart * 2; i++)
    {
        if (rand() % 2 == 0)
            seq->sample_positions[i].speed = 1 * multi2;
        else
            seq->sample_positions[i].speed = 1 * multi1;
    }
    for (int i = quart * 2; i < quart * 3; i++)
    {
        if (rand() % 2 == 0)
            seq->sample_positions[i].speed = 3 * multi1;
        else
            seq->sample_positions[i].speed = 2 * multi2;
    }
    for (int i = quart * 2; i < quart * 3; i++)
    {
        if (rand() % 2 == 0)
            seq->sample_positions[i].speed = 4 * multi2;
        else
            seq->sample_positions[i].speed = 4 * multi1;
    }

    // seq->morph_generation++;
}

void sample_sequencer_morph_restore(sample_sequencer *seq)
{
    for (int i = 0; i < PPBAR; i++)
        seq->sample_positions[i].speed = 1;
}

void sample_sequencer_set_pitch(sample_sequencer *seq, double v)
{
    if (v >= 0. && v <= 2.0)
        seq->buffer_pitch = v;
    else
        printf("Must be in the range of 0.0 .. 2.0\n");
}
