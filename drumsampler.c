#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include "defjams.h"
#include "drumsampler.h"
#include "mixer.h"
#include "utils.h"

extern wchar_t *sparkchars;
extern mixer *mixr;

drumsampler *new_drumsampler(char *filename)
{
    drumsampler *ds = (drumsampler *)calloc(1, sizeof(drumsampler));

    drumsampler_import_file(ds, filename);

    ds->envelope_enabled = false;
    ds->glitch_mode = false;
    ds->glitch_rand_factor = 20;

    ds->sg.gennext = &drumsampler_gennext;
    ds->sg.status = &drumsampler_status;
    ds->sg.get_volume = &sound_generator_get_volume;
    ds->sg.set_volume = &sound_generator_set_volume;
    ds->sg.get_pan = &sound_generator_get_pan;
    ds->sg.set_pan = &sound_generator_set_pan;
    ds->sg.start = &drumsampler_start;
    ds->sg.stop = &drumsampler_stop;
    ds->sg.get_num_patterns = &drumsampler_get_num_patterns;
    ds->sg.set_num_patterns = &drumsampler_set_num_patterns;
    ds->sg.make_active_track = &drumsampler_make_active_track;
    ds->sg.self_destruct = &drumsampler_del_self;
    ds->sg.event_notify = &sequence_engine_event_notify;
    ds->sg.get_pattern = &drumsampler_get_pattern;
    ds->sg.set_pattern = &drumsampler_set_pattern;
    ds->sg.is_valid_pattern = &drumsampler_is_valid_pattern;
    ds->sg.type = DRUMSAMPLER_TYPE;

    envelope_generator_init(&ds->eg);
    sequence_engine_init(&ds->engine, (void *)ds, DRUMSAMPLER_TYPE);
    sound_generator_init(&ds->sg);

    ds->sg.active = true;
    ds->started = false;

    return ds;
}

bool drumsampler_is_valid_pattern(void *self, int pattern_num)
{
    drumsampler *ds = (drumsampler *)self;
    return is_valid_pattern_num(&ds->engine, pattern_num);
}

midi_event *drumsampler_get_pattern(void *self, int pattern_num)
{
    sequence_engine *engine = get_sequence_engine(self);
    if (engine)
        return sequence_engine_get_pattern(engine, pattern_num);

    return NULL;
}

void drumsampler_set_pattern(void *self, int pattern_num,
                             pattern_change_info change_info,
                             midi_event *pattern)
{
    sequence_engine *engine = get_sequence_engine(self);
    if (engine)
        sequence_engine_set_pattern(engine, pattern_num, change_info, pattern);
}

void drumsampler_import_file(drumsampler *ds, char *filename)
{
    audio_buffer_details deetz = import_file_contents(&ds->buffer, filename);
    strcpy(ds->filename, deetz.filename);
    ds->bufsize = deetz.buffer_length;
    ds->buf_end_pos = ds->bufsize;
    ds->buffer_pitch = 1.0;
    ds->samplerate = deetz.sample_rate;
    ds->channels = deetz.num_channels;
    drumsampler_reset_samples(ds);
}

void drumsampler_reset_samples(drumsampler *ds)
{
    for (int i = 0; i < MAX_CONCURRENT_SAMPLES; i++)
    {
        ds->samples_now_playing[i] = -1;
    }
    for (int i = 0; i < PPBAR; i++)
    {
        ds->sample_positions[i].position = 0;
        ds->sample_positions[i].audiobuffer_cur_pos = 0.;
        ds->sample_positions[i].audiobuffer_inc = 1.0;
        ds->sample_positions[i].playing = 0;
        ds->sample_positions[i].played = 0;
        ds->sample_positions[i].amp = 0;
        ds->sample_positions[i].speed = 1;
    }
}

void drumsampler_note_on(drumsampler *ds)
{
    int idx = mixr->timing_info.midi_tick % PPBAR;
    int seq_position = get_a_drumsampler_position(ds);
    if (seq_position != -1)
    {
        ds->samples_now_playing[seq_position] = idx;
        ds->velocity_now_playing[seq_position] =
            ds->engine.patterns[ds->engine.cur_pattern][idx].data2;
    }
    eg_start_eg(&ds->eg);
}

stereo_val drumsampler_gennext(void *self)
{
    drumsampler *ds = (drumsampler *)self;
    double left_val = 0;
    double right_val = 0;

    for (int i = 0; i < MAX_CONCURRENT_SAMPLES; i++)
    {
        if (ds->samples_now_playing[i] != -1)
        {
            int cur_sample_midi_tick = ds->samples_now_playing[i];
            int velocity = ds->velocity_now_playing[i];
            double amp = scaleybum(0, 127, 0, 1, velocity);
            int idx =
                ds->sample_positions[cur_sample_midi_tick].audiobuffer_cur_pos;
            left_val += ds->buffer[idx] * amp;

            if (ds->channels == 2)
                right_val += ds->buffer[idx + 1] * amp;
            else
                right_val = left_val;

            if (ds->glitch_mode)
            {
                if (ds->sample_positions[cur_sample_midi_tick]
                            .audiobuffer_cur_pos > (ds->bufsize / 2) &&
                    rand() % 100 > ds->glitch_rand_factor)
                {
                    continue;
                }
            }

            ds->sample_positions[cur_sample_midi_tick].audiobuffer_cur_pos =
                ds->sample_positions[cur_sample_midi_tick].audiobuffer_cur_pos +
                (ds->channels * (ds->buffer_pitch));

            if ((int)ds->sample_positions[cur_sample_midi_tick]
                    .audiobuffer_cur_pos >= ds->buf_end_pos)
            { // end of playback - so reset
                ds->samples_now_playing[i] = -1;
                ds->sample_positions[cur_sample_midi_tick].audiobuffer_cur_pos =
                    0;
            }
        }
    }

    double amp_env = eg_do_envelope(&ds->eg, NULL);
    if (ds->eg.m_state == SUSTAIN)
        eg_note_off(&ds->eg);

    double volume = ds->sg.volume;
    if (ds->envelope_enabled)
        volume *= amp_env;

    ds->sg.pan = fmin(ds->sg.pan, 1.0);
    ds->sg.pan = fmax(ds->sg.pan, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(ds->sg.pan, &pan_left, &pan_right);

    stereo_val out = {.left = left_val * volume * pan_left,
                      .right = right_val * volume * pan_right};
    out = effector(&ds->sg, out);
    return out;
}

// drumsampler *new_drumsampler_from_char_pattern(char *filename, char *pattern)
//{
//    drumsampler *ds = new_drumsampler(filename);
//    pattern_char_to_pattern(&ds->m_seq, pattern,
//                            ds->m_seq.patterns[ds->m_seq.num_patterns++]);
//    return ds;
//}

void drumsampler_status(void *self, wchar_t *status_string)
{
    drumsampler *ds = (drumsampler *)self;

    char *INSTRUMENT_COLOR = ANSI_COLOR_RESET;
    if (ds->sg.active)
    {
        INSTRUMENT_COLOR = ANSI_COLOR_BLUE;
    }

    wchar_t local_status_string[MAX_STATIC_STRING_SZ] = {};
    swprintf(local_status_string, MAX_STATIC_STRING_SZ,
             WANSI_COLOR_WHITE
             "%s %s vol:%.2lf pan:%.2lf pitch:%.2f triplets:%d end_pos:%d\n"
             "eg:%d attack_ms:%.0f decay_ms:%.0f sustain:%.2f "
             "release_ms:%.0f glitch:%d gpct:%d\n"
             "multi:%d num_patterns:%d",
             ds->filename, INSTRUMENT_COLOR, ds->sg.volume, ds->sg.pan,
             ds->buffer_pitch, ds->engine.allow_triplets, ds->buf_end_pos,
             ds->envelope_enabled, ds->eg.m_attack_time_msec,
             ds->eg.m_decay_time_msec, ds->eg.m_sustain_level,
             ds->eg.m_release_time_msec, ds->glitch_mode,
             ds->glitch_rand_factor, ds->engine.multi_pattern_mode,
             ds->engine.num_patterns);

    wcscat(status_string, local_status_string);

    wmemset(local_status_string, 0, MAX_STATIC_STRING_SZ);
    sequence_engine_status(&ds->engine, local_status_string);
    wcscat(status_string, local_status_string);
    wcscat(status_string, WANSI_COLOR_RESET);
}

void drumsampler_del_self(void *self)
{
    drumsampler *s = (drumsampler *)self;
    printf("Deleting sample buffer\n");
    free(s->buffer);
    printf("Deleting drumsamplerUENCER SELF- bye!\n");
    free(s);
}

int drumsampler_get_num_patterns(void *self)
{
    drumsampler *s = (drumsampler *)self;
    return sequence_engine_get_num_patterns(&s->engine);
}

void drumsampler_set_num_patterns(void *self, int num_patterns)
{
    drumsampler *s = (drumsampler *)self;
    sequence_engine_set_num_patterns(&s->engine, num_patterns);
}

void drumsampler_make_active_track(void *self, int track_num)
{
    drumsampler *s = (drumsampler *)self;
    sequence_engine_make_active_track(&s->engine, track_num);
}

int get_a_drumsampler_position(drumsampler *ss)
{
    for (int i = 0; i < MAX_CONCURRENT_SAMPLES; i++)
        if (ss->samples_now_playing[i] == -1)
            return i;
    return -1;
}

void drumsampler_start(void *self)
{
    drumsampler *s = (drumsampler *)self;
    if (s->sg.active)
        return; // no-op
    drumsampler_reset_samples(s);
    s->sg.active = true;
    s->engine.cur_step = mixr->timing_info.sixteenth_note_tick % 16;
}

void drumsampler_stop(void *self)
{
    drumsampler *ds = (drumsampler *)self;
    ds->sg.active = false;
}

void drumsampler_set_pitch(drumsampler *ds, double v)
{
    if (v >= 0. && v <= 2.0)
        ds->buffer_pitch = v;
    else
        printf("Must be in the range of 0.0 .. 2.0\n");
}

void drumsampler_set_cutoff_percent(drumsampler *ds, unsigned int percent)
{
    if (percent > 100)
        return;
    ds->buf_end_pos = ds->bufsize / 100. * percent;
}

void drumsampler_enable_envelope_generator(drumsampler *ds, bool b)
{
    ds->envelope_enabled = b;
}
void drumsampler_set_attack_time(drumsampler *ds, double val)
{
    eg_set_attack_time_msec(&ds->eg, val);
}
void drumsampler_set_decay_time(drumsampler *ds, double val)
{
    eg_set_decay_time_msec(&ds->eg, val);
}
void drumsampler_set_sustain_lvl(drumsampler *ds, double val)
{
    eg_set_sustain_level(&ds->eg, val);
}
void drumsampler_set_release_time(drumsampler *ds, double val)
{
    eg_set_release_time_msec(&ds->eg, val);
}

void drumsampler_set_glitch_mode(drumsampler *ds, bool b)
{
    ds->glitch_mode = b;
}

void drumsampler_set_glitch_rand_factor(drumsampler *ds, int pct)
{
    if (pct >= 0 && pct <= 100)
        ds->glitch_rand_factor = pct;
}
