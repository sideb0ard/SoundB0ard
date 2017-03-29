#include <libgen.h>
#include <math.h>
#include <pthread.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "defjams.h"
#include "sequencer.h"
#include "sequencer_utils.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
extern wchar_t *sparkchars;

sequencer *new_seq(char *filename)
{
    sequencer *seq = (sequencer *)calloc(1, sizeof(sequencer));

    SF_INFO sf_info;
    memset(&sf_info, 0, sizeof(SF_INFO));
    int bufsize;
    int *buffer = load_file_to_buffer(filename, &bufsize, &sf_info);

    int fslen = strlen(filename);
    seq->filename = (char *)calloc(1, fslen + 1);
    strncpy(seq->filename, filename, fslen);

    memset(seq->matrix1, 0, sizeof seq->matrix1);
    memset(seq->matrix2, 0, sizeof seq->matrix2);

    seq->buffer = buffer;
    seq->bufsize = bufsize;
    seq->samplerate = sf_info.samplerate;
    seq->channels = sf_info.channels;
    seq->started = false;
    seq->multi_pattern_mode = true;
    seq->cur_pattern_iteration = 1;
    seq->vol = 0.7;

    seq->tickedyet = false;
    for (int i = 0; i < NUM_SEQUENCER_PATTERNS; i++) {

        seq->pattern_num_loops[i] = 1;

        for (int j = 0; j < SEQUENCER_PATTERN_LEN; j++) {
            seq->pattern_position_amp[i][j] = DEFAULT_AMP;
        }
    }

    seq->sound_generator.gennext = &seq_gennext;
    seq->sound_generator.status = &seq_status;
    seq->sound_generator.getvol = &seq_getvol;
    seq->sound_generator.setvol = &seq_setvol;
    seq->sound_generator.type = SEQUENCER_TYPE;

    stereo_delay_prepare_for_play(&seq->m_delay_fx);
    filter_moog_init(&seq->m_filter);
    seq->m_fc_control = 10000;
    moog_update((filter*) &seq->m_filter);

    return seq;
}

double seq_gennext(void *self)
{
    sequencer *seq = (sequencer *)self;
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

    if ((seq->patterns[seq->cur_pattern] & bit_position) &&
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
                * seq->pattern_position_amp[seq->cur_pattern][i];
            seq->sample_positions[i].position =
                seq->sample_positions[i].position + seq->channels;
            if ((int)seq->sample_positions[i].position ==
                seq->bufsize) { // end of playback - so reset
                seq->sample_positions[i].playing = 0;
                seq->sample_positions[i].position = 0;
            }
        }
    }

    if (mixr->sixteenth_note_tick != seq->tick) {
        int prev_note = step_seq_idx - 1;
        if (prev_note == -1)
            prev_note = 15;

        seq->sample_positions[prev_note].played = 0;
        seq->tick = mixr->sixteenth_note_tick;

        if (seq->tick % 16 == 0) {

            if (mixr->debug_mode) {
                printf("Top of loop [%d] - 16tick = %d, tik = %d\n",
                       step_seq_idx, mixr->sixteenth_note_tick, mixr->tick);
            }

            if (seq->multi_pattern_mode) {
                seq->cur_pattern_iteration--;
                if (seq->cur_pattern_iteration == 0) {
                    seq->cur_pattern =
                        (seq->cur_pattern + 1) % seq->num_patterns;
                    seq->cur_pattern_iteration =
                        seq->pattern_num_loops[seq->cur_pattern];
                }
            }

            // seq->cur_pattern =
            //     (seq->cur_pattern + 1) % seq->num_patterns;

            if (seq->game_of_life_on) {
                next_life_generation(seq);
                if (seq->game_generation++ > 4) {
                    seq->patterns[seq->cur_pattern] = seed_pattern();
                    seq->game_generation = 0;
                }
                if (seq->max_generation != 0 &&
                    seq->game_generation >= seq->max_generation) {
                    if (mixr->debug_mode)
                        printf("passed max generation of life - stopping\n");
                    seq->game_generation = 0;
                    seq_set_game_of_life(seq, 0);
                }
            }
            else if (seq->markov_on) {
                next_markov_generation(seq);
                seq->markov_generation++;
                if (seq->max_generation != 0 &&
                    seq->markov_generation >= seq->max_generation) {
                    if (mixr->debug_mode)
                        printf("passed max generation of markov - stopping\n");
                    seq->game_generation = 0;
                    seq_set_markov(seq, 0);
                }
            }
        }
    }

    val = effector(&seq->sound_generator, val);
    val = envelopor(&seq->sound_generator, val);

    // update delay and filter
    seq->m_filter.f.m_fc_control = seq->m_fc_control;
    moog_set_qcontrol((filter*) &seq->m_filter, seq->m_q_control);
    moog_update((filter*) &seq->m_filter);
    val = moog_gennext((filter*) &seq->m_filter, val);

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

void int_pattern_to_array(int pattern, int *pat_array)
{
    for (int i = 0, p = 1; p < 65535; i++, p *= 2) {
        if (pattern & p) {
            pat_array[i] = 1;
        }
        else {
            pat_array[i] = 0;
        }
    }
}

void pattern_char_to_int(char *char_pattern, int *final_pattern)
{
    int sp_count = 0;
    char *sp, *sp_last, *spattern[32];
    char const *sep = " ";

    printf("CHARPATT %s\n", char_pattern);
    // extract numbers from string into spattern
    for (sp = strtok_r(char_pattern, sep, &sp_last); sp;
         sp = strtok_r(NULL, sep, &sp_last)) {
        spattern[sp_count++] = sp;
    }

    *final_pattern = 0;
    for (int i = 0; i < sp_count; i++) {
        // TODO - make get rid of magic number - loop length
        int pat_num = 15 - atoi(spattern[i]);
        if (pat_num < SEQUENCER_PATTERN_LEN) {
            printf("PAT_NUM: %d is %d\n", pat_num, (1 << pat_num));
            *final_pattern = (1 << pat_num) | *final_pattern;
            printf("NOW SET %d\n", *final_pattern);
        }
    }
    printf("FINAL PATTERN %d\n", *final_pattern);
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
    return buffer;
}

sequencer *new_seq_from_int_pattern(char *filename, int pattern)
{
    sequencer *seq = new_seq(filename);
    seq->patterns[seq->num_patterns++] = pattern;
    return seq;
}

sequencer *new_seq_from_char_pattern(char *filename, char *pattern)
{
    sequencer *seq = new_seq(filename);
    pattern_char_to_int(pattern, &seq->patterns[seq->num_patterns++]);
    return seq;
}

// game of life algo helpers
void int_to_matrix(int pattern, int matrix[GRIDWIDTH][GRIDWIDTH])
{
    int row = 0;
    for (int i = 0, p = 1; i < INTEGER_LENGTH; i++, p *= 2) {

        if (i != 0 && (i % GRIDWIDTH == 0))
            row++;

        int col = i % GRIDWIDTH;
        // printf("POS %d %d\n", row, col);
        if (pattern & p) {
            matrix[row][col] = 1;
        }
    }
}

int matrix_to_int(int matrix[GRIDWIDTH][GRIDWIDTH])
{
    int return_pattern = 0;

    int row = 0;
    for (int i = 0, p = 1; i < SEQUENCER_PATTERN_LEN; i++, p *= 2) {

        if (i != 0 && (i % GRIDWIDTH == 0))
            row++;
        int col = i % GRIDWIDTH;

        if (matrix[row][col] == 1) {
            return_pattern = return_pattern | p;
        }
    }

    return return_pattern;
}

// game of life algo
void next_life_generation(sequencer *self)
{
    // printf("NEXT LIFE GEN!\n");
    memset(self->matrix1, 0, sizeof self->matrix1);
    memset(self->matrix2, 0, sizeof self->matrix2);
    int_to_matrix(self->patterns[self->cur_pattern], self->matrix1);

    for (int y = 0; y < GRIDWIDTH; y++) {
        for (int x = 0; x < GRIDWIDTH; x++) {

            int neighbors = 0;

            // printf("My co-ords y:%d x:%d\n", y, x);
            for (int rel_y = y - 1; rel_y <= y + 1; rel_y++) {
                for (int rel_x = x - 1; rel_x <= x + 1; rel_x++) {
                    int n_y = rel_y;
                    int n_x = rel_x;
                    if (n_y < 0)
                        n_y += GRIDWIDTH;
                    if (n_y == GRIDWIDTH)
                        n_y -= GRIDWIDTH;
                    if (n_x < 0)
                        n_x += GRIDWIDTH;
                    if (n_x == GRIDWIDTH)
                        n_x -= GRIDWIDTH;
                    if (!(n_x == x && n_y == y)) {
                        // printf("My neighbs y:%d x:%d val - %d\n", n_y, n_x);
                        if (self->matrix1[n_y][n_x] == 1)
                            neighbors += 1;
                    }
                }
            }
            // printf("[%d][%d] - I gots %d neighbors\n", y, x, neighbors);

            // the RULES
            if (self->matrix1[y][x] == 0 && neighbors == 3)
                self->matrix2[y][x] = 1;

            if (self->matrix1[y][x] == 1 && (neighbors == 2 || neighbors == 3))
                self->matrix2[y][x] = 1;

            if (self->matrix1[y][x] == 1 && (neighbors > 3 || neighbors < 2))
                self->matrix2[y][x] = 0;
        }
    }
    // matrix_print(4, matrix2);

    // int return_pattern = matrix_to_int(self->matrix2);
    int new_pattern = matrix_to_int(self->matrix2);
    // printf("NEW PATTERN! %d\n", new_pattern);
    if (new_pattern == 0)
        new_pattern = seed_pattern();
    self->patterns[self->cur_pattern] = new_pattern;
}

void next_markov_generation(sequencer *d)
{
    int new_pattern = 0;

    for (int i = 32768; i > 0; i = i >> 1) {

        int randy = rand() % 100;
        if (mixr->debug_mode)
            printf("Iiii %d Randy! %d\n", i, randy);

        if (d->markov_mode == MARKOVHAUS) {
            if (mixr->debug_mode)
                printf("HAUS MUSIC ALL NIGHT LONG!\n");

            switch (i) {
            case 32768:
                randy < 95 ? new_pattern = new_pattern | i : 0;
                break;
            case 16384:
                randy < 5 ? new_pattern = new_pattern | i : 0;
                break;
            case 8192:
                randy < 5 ? new_pattern = new_pattern | i : 0;
                break;
            case 4096:
                randy < 5 ? new_pattern = new_pattern | i : 0;
                break;
            case 2048:
                randy < 95 ? new_pattern = new_pattern | i : 0;
                break;
            case 1024:
                randy < 5 ? new_pattern = new_pattern | i : 0;
                break;
            case 512:
                randy < 5 ? new_pattern = new_pattern | i : 0;
                break;
            case 256:
                randy < 2 ? new_pattern = new_pattern | i : 0;
                break;
            case 128:
                randy < 95 ? new_pattern = new_pattern | i : 0;
                break;
            case 64:
                randy < 2 ? new_pattern = new_pattern | i : 0;
                break;
            case 32:
                randy < 5 ? new_pattern = new_pattern | i : 0;
                break;
            case 16:
                randy < 2 ? new_pattern = new_pattern | i : 0;
                break;
            case 8:
                randy < 95 ? new_pattern = new_pattern | i : 0;
                break;
            case 4:
                randy < 2 ? new_pattern = new_pattern | i : 0;
                break;
            case 2:
                randy < 9 ? new_pattern = new_pattern | i : 0;
                break;
            case 1:
                randy < 11 ? new_pattern = new_pattern | i : 0;
                break;
            }
        }
        else if (d->markov_mode == MARKOVBOOMBAP) {
            if (mixr->debug_mode)
                printf("88 y'all!!\n");
            switch (i) {
            case 32768:
                randy < 95 ? new_pattern = new_pattern | i : 0;
                break;
            case 16384:
                randy < 2 ? new_pattern = new_pattern | i : 0;
                break;
            case 8192:
                randy < 5 ? new_pattern = new_pattern | i : 0;
                break;
            case 4096:
                randy < 85 ? new_pattern = new_pattern | i : 0;
                break;
            case 2048:
                randy < 2 ? new_pattern = new_pattern | i : 0;
                break;
            case 1024:
                randy < 2 ? new_pattern = new_pattern | i : 0;
                break;
            case 512:
                randy < 15 ? new_pattern = new_pattern | i : 0;
                break;
            case 256:
                randy < 2 ? new_pattern = new_pattern | i : 0;
                break;
            case 128:
                randy < 95 ? new_pattern = new_pattern | i : 0;
                break;
            case 64:
                randy < 12 ? new_pattern = new_pattern | i : 0;
                break;
            case 32:
                randy < 12 ? new_pattern = new_pattern | i : 0;
                break;
            case 16:
                randy < 22 ? new_pattern = new_pattern | i : 0;
                break;
            case 8:
                randy < 15 ? new_pattern = new_pattern | i : 0;
                break;
            case 4:
                randy < 12 ? new_pattern = new_pattern | i : 0;
                break;
            case 2:
                randy < 9 ? new_pattern = new_pattern | i : 0;
                break;
            case 1:
                randy < 41 ? new_pattern = new_pattern | i : 0;
                break;
            }
        }
    }

    if (mixr->debug_mode)
        printf("New pattern! %d\n", new_pattern);
    d->patterns[d->cur_pattern] = new_pattern;
}

// void update_pattern(void *self, int newpattern)
// {
//     sequencer *seq = self;
//     seq->pattern = newpattern;
// }

void seq_status(void *self, wchar_t *status_string)
{
    sequencer *seq = (sequencer *)self;
    swprintf(status_string, MAX_PS_STRING_SZ, WANSI_COLOR_BLUE
             "[SEQUENCER] \"%s\" Vol: %.2lf Cur: %d life_mode: %d "
             "markov_on: %d markov_mode: %s Multi: %d Swing: %d Max Gen: %d",
             basename(seq->filename), seq->vol, seq->cur_pattern,
             seq->game_of_life_on, seq->markov_on,
             seq->markov_mode ? "boombap" : "haus", seq->multi_pattern_mode,
             seq->swing_setting, seq->max_generation);
    wchar_t pattern_details[128];
    char spattern[17];
    wchar_t apattern[17];
    for (int i = 0; i < seq->num_patterns; i++) {
        char_binary_version_of_int(seq->patterns[i], spattern);
        wchar_version_of_amp(seq, i, apattern);
        swprintf(pattern_details, 127, L"\n      [%d] - [%s] %ls  numloops: %d",
                 i, spattern, apattern, seq->pattern_num_loops[i]);
        wcscat(status_string, pattern_details);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
}

// TODO - fix this - being lazy and want it finished NOW!
void wchar_version_of_amp(sequencer *d, int pattern_num, wchar_t apattern[17])
{
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
        double amp = d->pattern_position_amp[pattern_num][i];
        int idx = (int)floor(scaleybum(0, 1.1, 0, wcslen(sparkchars), amp));
        apattern[i] = sparkchars[idx];
        // wprintf(L"\n%ls\n", sparkchars[3]);
    }
    apattern[16] = '\0';
}

double seq_getvol(void *self)
{
    sequencer *seq = (sequencer *)self;
    return seq->vol;
}

void seq_setvol(void *self, double v)
{
    sequencer *seq = (sequencer *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    seq->vol = v;
}

void swingrrr(void *self, int swing_setting)
// swing_setting in range {1..6}, with same convention as
// Linn seq machines - 50%, 54%, 58%, 62%, 66%, 71%
{
    sequencer *seq = (sequencer *)self;
    if (swing_setting == 0) {
        seq->swing = 0;
    }
    else {
        seq->swing = 1;
        seq->swing_setting = swing_setting;
    }
}

void add_char_pattern(sequencer *d, char *pattern)
{
    pattern_char_to_int(pattern, &d->patterns[d->num_patterns++]);
    // TODO this and the next function need to be combined or something.
    // Something smells funny.
    d->cur_pattern++;
}

void add_int_pattern(sequencer *d, int pattern)
{
    d->patterns[d->num_patterns++] = pattern;
    d->cur_pattern++;
}

void change_char_pattern(sequencer *d, int pattern_num, char *pattern)
{
    pattern_char_to_int(pattern, &d->patterns[pattern_num]);
}

void change_int_pattern(sequencer *d, int pattern_num, int pattern)
{
    d->patterns[pattern_num] = pattern;
}

int seed_pattern()
{
    int pattern = 0;
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
        int randy = rand() % 100;
        // printf("RANDY %d\n", randy);
        if (randy > 50) {
            pattern = pattern | (1 << i);
        }
    }
    return pattern;
}

void seq_set_sample_amp(sequencer *d, int pattern_num, int pattern_position,
                         double v)
{
    d->pattern_position_amp[pattern_num][pattern_position] = v;
}

void seq_set_random_sample_amp(sequencer *d, int pattern_num)
{
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
        double randy = (double)rand() / (double)RAND_MAX;
        printf("Setting random to %f\n", randy);
        seq_set_sample_amp(d, pattern_num, i, randy);
    }
}

void seq_set_sample_amp_from_char_pattern(sequencer *d, int pattern_num,
                                           char *amp_pattern)
{
    printf("Ooh, setting amps to %s\n", amp_pattern);

    int sp_count = 0;
    char *sp, *sp_last, *spattern[32];
    char const *sep = " ";

    printf("CHARPATT %s\n", amp_pattern);
    // extract numbers from string into spattern
    for (sp = strtok_r(amp_pattern, sep, &sp_last); sp;
         sp = strtok_r(NULL, sep, &sp_last)) {
        spattern[sp_count++] = sp;
    }

    for (int i = 0; i < sp_count; i++) {
        printf("[%d] -- %s\n", i, spattern[i]);
        seq_set_sample_amp(d, pattern_num, atof(spattern[i]), 1);
    }
}

void seq_set_multi_pattern_mode(sequencer *d, bool multi)
{
    d->multi_pattern_mode = multi;
    d->cur_pattern_iteration = d->pattern_num_loops[d->cur_pattern];
}

void seq_change_num_loops(sequencer *d, int pattern_num, int num_loops)
{
    if (pattern_num < d->num_patterns && num_loops > 0) {
        d->pattern_num_loops[pattern_num] = num_loops;
    }
}

void seq_set_game_of_life(sequencer *d, bool b)
{
    d->game_generation = 0;
    if (b) {
        d->game_of_life_on = true;
        seq_set_backup_mode(d, b);
    }
    else {
        d->game_of_life_on = false;
        seq_set_backup_mode(d, b);
    }
}

void seq_set_markov(sequencer *d, bool b)
{
    d->markov_generation = 0;
    if (b) {
        d->markov_on = true;
        seq_set_backup_mode(d, b);
    }
    else {
        d->markov_on = false;
        seq_set_backup_mode(d, b);
    }
}

void seq_set_backup_mode(sequencer *d, bool on)
{
    if (on) {
        d->backup_pattern_while_getting_crazy = d->patterns[0];
        d->multi_pattern_mode = false;
        d->cur_pattern = 0;
    }
    else {
        d->patterns[0] = d->backup_pattern_while_getting_crazy;
        d->multi_pattern_mode = true;
    }
}

void seq_set_max_generations(sequencer *d, int max) { d->max_generation = max; }

void seq_set_markov_mode(sequencer *d, unsigned int mode) { d->markov_mode = mode; }

void seq_parse_midi(sequencer *s, unsigned int data1, unsigned int data2)
{
    printf("YA BEEZER, MIDI DRUM SEQUENCER!\n");

    double scaley_val = 0.;
    switch (data1) {
    case 1:
        scaley_val = scaleybum(1, 128, FILTER_FC_MIN, FILTER_FC_MAX, data2);
        printf("Filter FREQ Control! %f\n", scaley_val);
        s->m_fc_control = scaley_val;
        break;
    case 2:
        scaley_val = scaleybum(1, 128, 1, 10, data2);
        printf("Filter Q Control! %f\n", scaley_val);
        s->m_q_control = scaley_val;
        break;
    case 3:
        scaley_val = scaleybum(1, 128, 1, 6, data2);
        printf("SWIIIiiing!! %f\n", scaley_val);
        s->swing_setting = scaley_val;
        break;
    case 4:
        scaley_val = scaleybum(1, 128, 0., 1., data2);
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
        scaley_val = scaleybum(1, 128, -0.9, 0.9, data2);
        printf("Delay Ratio! %f\n", scaley_val);
        s->m_delay_ratio = scaley_val;
        break;
    case 8:
        scaley_val = scaleybum(1, 128, 0, 1, data2);
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
