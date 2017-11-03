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
#include "sequencer.h"
#include "sequencer_utils.h"
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
    seq->sound_generator.type = SEQUENCER_TYPE;

    stereo_delay_prepare_for_play(&seq->m_delay_fx);
    filter_moog_init(&seq->m_filter);
    seq->m_fc_control = 10000;
    moog_update((filter *)&seq->m_filter);

    return seq;
}

void sample_seq_import_file(sample_sequencer *seq, char *filename)
{
    audio_buffer_details deetz = import_file_contents(&seq->buffer, filename);
    strcpy(seq->filename, deetz.filename);
    seq->bufsize = deetz.buffer_length;
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
        seq->sample_positions[i].playing = 0;
        seq->sample_positions[i].played = 0;
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
    double val = 0;

    // wait till start of loop to keep patterns synched
    if (!seq->started)
        return (stereo_val){0, 0};

    for (int i = 0; i < MAX_CONCURRENT_SAMPLES; i++)
    {
        if (seq->samples_now_playing[i] != -1)
        {
            int cur_sample_midi_tick = seq->samples_now_playing[i];
            val += seq->buffer[seq->sample_positions[cur_sample_midi_tick]
                                   .position] *
                   seq->m_seq.pattern_position_amp[seq->m_seq.cur_pattern]
                                                  [cur_sample_midi_tick];
            seq->sample_positions[cur_sample_midi_tick].position =
                seq->sample_positions[cur_sample_midi_tick].position +
                seq->channels;
            if ((int)seq->sample_positions[cur_sample_midi_tick].position >=
                seq->bufsize)
            { // end of playback - so reset
                seq->samples_now_playing[i] = -1;
                seq->sample_positions[cur_sample_midi_tick].position = 0;
            }
        }
    }

    val = effector(&seq->sound_generator, val);
    val = envelopor(&seq->sound_generator, val);

    // update delay and filter
    seq->m_filter.f.m_fc_control = seq->m_fc_control;
    moog_set_qcontrol((filter *)&seq->m_filter, seq->m_q_control);
    moog_update((filter *)&seq->m_filter);
    val = moog_gennext((filter *)&seq->m_filter, val);

    stereo_delay_set_mode(&seq->m_delay_fx, seq->m_delay_mode);
    stereo_delay_set_delay_time_ms(&seq->m_delay_fx, seq->m_delay_time_msec);
    stereo_delay_set_feedback_percent(&seq->m_delay_fx, seq->m_feedback_pct);
    stereo_delay_set_delay_ratio(&seq->m_delay_fx, seq->m_delay_ratio);
    stereo_delay_set_wet_mix(&seq->m_delay_fx, seq->m_wet_mix);
    stereo_delay_update(&seq->m_delay_fx);
    double out_left = 0.0;
    double out_right = 0.0;
    stereo_delay_process_audio(&seq->m_delay_fx, &val, &val, &out_left,
                               &out_right);

    return (stereo_val){.left = out_left * seq->vol,
                        .right = out_right * seq->vol};
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
    sample_sequencer *seq = (sample_sequencer *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             L"[SAMPLE SEQ] \"%s\" Vol: %.2lf Active: %s", seq->filename,
             seq->vol, seq->sound_generator.active ? "true" : "false");
    wchar_t seq_status_string[MAX_PS_STRING_SZ];
    memset(seq_status_string, 0, MAX_PS_STRING_SZ);
    seq_status(&seq->m_seq, seq_status_string);
    wcscat(status_string, seq_status_string);
    wcscat(status_string, WANSI_COLOR_RESET);
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

// TODO make this part of SOUND GENERATOR
void sample_seq_parse_midi(sample_sequencer *s, unsigned int data1,
                           unsigned int data2)
{
    printf("YA BEEZER, MIDI DRUM SEQUENCER!\n");

    double scaley_val = 0.;
    switch (data1)
    {
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
        // s->swing_setting = scaley_val;
        break;
    case 4:
        scaley_val = scaleybum(0, 127, 0., 1., data2);
        printf("Volume! %f\n", scaley_val);
        s->vol = scaley_val;
        break;
    case 5:
        scaley_val = scaleybum(0, 128, 0, 2000, data2);
        printf("Delay Feedback Msec %f!\n", scaley_val);
        s->m_delay_time_msec = scaley_val;
        break;
    case 6:
        scaley_val = scaleybum(0, 128, 20, 100, data2);
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
    {
        if (ss->samples_now_playing[i] == -1)
        {
            return i;
        }
    }
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
