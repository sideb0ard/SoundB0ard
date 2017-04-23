#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "defjams.h"
#include "mixer.h"
#include "sample_sequencer.h"
#include "sequencer.h"
#include "sequencer_utils.h"
#include "utils.h"

extern mixer *mixr;
extern wchar_t *sparkchars;

sample_sequencer *new_sample_seq(char *filename)
{
    sample_sequencer *seq =
        (sample_sequencer *)calloc(1, sizeof(sample_sequencer));
    seq_init(&seq->m_seq);

    SF_INFO sf_info;
    memset(&sf_info, 0, sizeof(SF_INFO));
    int bufsize;
    int *buffer = load_file_to_buffer(filename, &bufsize, &sf_info);

    int fslen = strlen(filename);
    seq->filename = (char *)calloc(1, fslen + 1);
    strncpy(seq->filename, filename, fslen);

    seq->buffer = buffer;
    seq->bufsize = bufsize;
    seq->samplerate = sf_info.samplerate;
    seq->channels = sf_info.channels;
    seq->started = false;
    seq->vol = 0.7;

    seq->sound_generator.gennext = &sample_seq_gennext;
    seq->sound_generator.status = &sample_seq_status;
    seq->sound_generator.getvol = &sample_seq_getvol;
    seq->sound_generator.setvol = &sample_seq_setvol;
    seq->sound_generator.type = SEQUENCER_TYPE;

    stereo_delay_prepare_for_play(&seq->m_delay_fx);
    filter_moog_init(&seq->m_filter);
    seq->m_fc_control = 10000;
    moog_update((filter *)&seq->m_filter);

    return seq;
}

double sample_seq_gennext(void *self)
{
    sample_sequencer *seq = (sample_sequencer *)self;
    double val = 0;

    int step_seq_idx = mixr->sixteenth_note_tick % SEQUENCER_PATTERN_LEN;

    // wait till start of loop to keep patterns synched
    if (!seq->started) {
        if (step_seq_idx == 0) {
            seq->started = true;
        }
        else {
            return val;
        }
    }

    int bit_position = 1 << (15 - step_seq_idx);

    if ((seq->m_seq.patterns[seq->m_seq.cur_pattern] & bit_position) &&
        !seq->sample_positions[step_seq_idx].played) {
        if (seq->swing) {
            if (mixr->sixteenth_note_tick % 2) {
                double swing_offset = PPQN * 2 / 100.0;
                switch (seq->swing_setting) {
                case 1:
                    swing_offset = swing_offset * 50 - PPQN;
                    break;
                case 2:
                    swing_offset = swing_offset * 54 - PPQN;
                    break;
                case 3:
                    swing_offset = swing_offset * 58 - PPQN;
                    break;
                case 4:
                    swing_offset = swing_offset * 62 - PPQN;
                    break;
                case 5:
                    swing_offset = swing_offset * 66 - PPQN;
                    break;
                case 6:
                    swing_offset = swing_offset * 71 - PPQN;
                    break;
                default:
                    swing_offset = swing_offset * 50 - PPQN;
                }
                if (mixr->tick % (PPQN / 4) == (int)swing_offset / 4) {
                    seq->sample_positions[step_seq_idx].playing = 1;
                    seq->sample_positions[step_seq_idx].played = 1;
                    if (mixr->debug_mode)
                        printf("SWING SWUNG tick %% PPQN: %d\n",
                               mixr->tick % PPQN);
                }
            }
            else {
                seq->sample_positions[step_seq_idx].playing = 1;
                seq->sample_positions[step_seq_idx].played = 1;
                // printf("SWING NORM tick %% PPQN: %d\n", mixr->tick % PPQN);
            }
        }
        else {
            seq->sample_positions[step_seq_idx].playing = 1;
            seq->sample_positions[step_seq_idx].played = 1;
        }
    }

    for (int i = SEQUENCER_PATTERN_LEN - 1; i >= 0; i--) {
        if (seq->sample_positions[i].playing) {
            val +=
                // seq->buffer[seq->sample_positions[i].position++] /
                seq->buffer[seq->sample_positions[i].position] /
                2147483648.0 // convert from 16bit in to double between 0 and 1
                * seq->m_seq.pattern_position_amp[seq->m_seq.cur_pattern][i];
            seq->sample_positions[i].position =
                seq->sample_positions[i].position + seq->channels;
            if ((int)seq->sample_positions[i].position ==
                seq->bufsize) { // end of playback - so reset
                seq->sample_positions[i].playing = 0;
                seq->sample_positions[i].position = 0;
            }
        }
    }

    if (seq_tick(&seq->m_seq)) {
        int prev_note = step_seq_idx - 1;
        if (prev_note == -1)
            prev_note = 15;

        seq->sample_positions[prev_note].played = 0;
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
    double out = 0.0;
    stereo_delay_process_audio(&seq->m_delay_fx, &val, &val, &out, &out);

    return out * seq->vol;
}

int *load_file_to_buffer(char *filename, int *bufsize, SF_INFO *sf_info)
{
    SNDFILE *snd_file;

    sf_info->format = 0;
    snd_file = sf_open(filename, SFM_READ, sf_info);
    if (!snd_file) {
        printf("Err opening %s : %d\n", filename, sf_error(snd_file));
        return NULL;
    }
    printf("Filename:: %s\n", filename);
    printf("SR: %d\n", sf_info->samplerate);
    printf("Channels: %d\n", sf_info->channels);
    printf("Frames: %lld\n", sf_info->frames);

    *bufsize = sf_info->channels * sf_info->frames;
    printf("Making buffer size of %d\n", *bufsize);

    int *buffer = (int *)calloc(*bufsize, sizeof(int));
    if (buffer == NULL) {
        printf("Ooft, memory issues, mate!\n");
        return NULL;
    }

    sf_readf_int(snd_file, buffer, *bufsize);
    sf_close(snd_file);
    return buffer;
}

sample_sequencer *new_sample_seq_from_int_pattern(char *filename, int pattern)
{
    sample_sequencer *seq = new_sample_seq(filename);
    seq->m_seq.patterns[seq->m_seq.num_patterns++] = pattern;
    return seq;
}

sample_sequencer *new_sample_seq_from_char_pattern(char *filename,
                                                   char *pattern)
{
    sample_sequencer *seq = new_sample_seq(filename);
    pattern_char_to_int(pattern,
                        &seq->m_seq.patterns[seq->m_seq.num_patterns++]);
    return seq;
}

void sample_seq_status(void *self, wchar_t *status_string)
{
    sample_sequencer *seq = (sample_sequencer *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             WANSI_COLOR_BLUE "[SAMPLE SEQ] \"%s\" Vol: %.2lf",
             basename(seq->filename), seq->vol);
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
    if (v < 0.0 || v > 1.0) {
        return;
    }
    seq->vol = v;
}

void swingrrr(void *self, int swing_setting)
// swing_setting in range {1..6}, with same convention as
// Linn seq machines - 50%, 54%, 58%, 62%, 66%, 71%
{
    sample_sequencer *seq = (sample_sequencer *)self;
    if (swing_setting == 0) {
        seq->swing = 0;
    }
    else {
        seq->swing = 1;
        seq->swing_setting = swing_setting;
    }
}

// TODO make this part of SOUND GENERATOR
void sample_seq_parse_midi(sample_sequencer *s, unsigned int data1,
                           unsigned int data2)
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

void sample_seq_del(sample_sequencer *s)
{
    printf("Deleting sample buffer\n");
    free(s->buffer);
    printf("Deleting char buffer for filename\n");
    free(s->filename);
    printf("Deleting SAMPLESEQUENCER SELF- bye!\n");
    free(s);
}
