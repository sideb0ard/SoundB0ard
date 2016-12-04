#include <libgen.h>
#include <math.h>
#include <pthread.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "drumr.h"
#include "drumr_utils.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

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
    char *sep = " ";

    printf("CHARPATT %s\n", char_pattern);
    // extract numbers from string into spattern
    for (sp = strtok_r(char_pattern, sep, &sp_last); sp;
         sp = strtok_r(NULL, sep, &sp_last)) {
        spattern[sp_count++] = sp;
    }

    for (int i = 0; i < sp_count; i++) {
        // TODO - make get rid of magic number - loop length
        int pat_num = 15 - atoi(spattern[i]);
        if (pat_num < DRUM_PATTERN_LEN) {
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

    int *buffer = calloc(*bufsize, sizeof(int));
    if (buffer == NULL) {
        printf("Ooft, memory issues, mate!\n");
        return NULL;
    }

    sf_readf_int(snd_file, buffer, *bufsize);
    return buffer;
}

DRUM *new_drumr_from_int_pattern(char *filename, int pattern)
{
    DRUM *drumr = new_drumr(filename);
    drumr->patterns[drumr->num_patterns++] = pattern;
    return drumr;
}

DRUM *new_drumr_from_char_pattern(char *filename, char *pattern)
{
    DRUM *drumr = new_drumr(filename);
    pattern_char_to_int(pattern, &drumr->patterns[drumr->num_patterns++]);
    return drumr;
}

DRUM *new_drumr(char *filename)
{
    DRUM *drumr = calloc(1, sizeof(DRUM));

    SF_INFO sf_info;
    memset(&sf_info, 0, sizeof(SF_INFO));
    int bufsize;
    int *buffer = load_file_to_buffer(filename, &bufsize, &sf_info);

    int fslen = strlen(filename);
    drumr->filename = calloc(1, fslen + 1);
    strncpy(drumr->filename, filename, fslen);

    memset(drumr->matrix1, 0, sizeof drumr->matrix1);
    memset(drumr->matrix2, 0, sizeof drumr->matrix2);

    drumr->buffer = buffer;
    drumr->bufsize = bufsize;
    drumr->samplerate = sf_info.samplerate;
    drumr->channels = sf_info.channels;
    drumr->started = false;
    drumr->vol = 0.7;

    drumr->tickedyet = false;

    drumr->sound_generator.gennext = &drum_gennext;
    drumr->sound_generator.status = &drum_status;
    drumr->sound_generator.getvol = &drum_getvol;
    drumr->sound_generator.setvol = &drum_setvol;
    drumr->sound_generator.type = DRUM_TYPE;


    // TODO: do i need to free pattern ?
    return drumr;
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
    for (int i = 0, p = 1; i < DRUM_PATTERN_LEN; i++, p *= 2) {

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
void next_life_generation(void *d)
{
    // printf("NEXT LIFE GEN!\n");
    DRUM *self = d;
    memset(self->matrix1, 0, sizeof self->matrix1);
    memset(self->matrix2, 0, sizeof self->matrix2);
    int_to_matrix(self->patterns[self->cur_pattern_num], self->matrix1);

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
    self->patterns[self->cur_pattern_num] = new_pattern;
}

// void update_pattern(void *self, int newpattern)
// {
//     DRUM *drumr = self;
//     drumr->pattern = newpattern;
// }

double drum_gennext(void *self)
// void drum_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
    DRUM *drumr = self;
    double val = 0;

    int step_seq_idx = mixr->sixteenth_note_tick % DRUM_PATTERN_LEN;

    // wait till start of loop to keep patterns synched
    if (!drumr->started) {
        if (step_seq_idx == 0) {
            printf("Starting LOOP / STEP SEQ [%d]\n", step_seq_idx);
            drumr->started = true;
        }
        else {
            return val;
        }
    }

    int bit_position = 1 << (15 - step_seq_idx);


    if ((drumr->patterns[drumr->cur_pattern_num] & bit_position) &&
        !drumr->sample_positions[step_seq_idx].played) {


        if (step_seq_idx == 0)
            printf("Top of loop [%d] - 16tick = %d, tik = %d\n",
                    step_seq_idx,
                    mixr->sixteenth_note_tick,
                    mixr->tick);

        if (drumr->swing) {
            if (mixr->sixteenth_note_tick % 2) {
                double swing_offset = PPQN * 2 / 100.0;
                switch (drumr->swing_setting) {
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
                    drumr->sample_positions[step_seq_idx].playing = 1;
                    drumr->sample_positions[step_seq_idx].played = 1;
                    // printf("SWING SWUNG tick %% PPQN: %d\n", mixr->tick %
                    // PPQN);
                }
            }
            else {
                drumr->sample_positions[step_seq_idx].playing = 1;
                drumr->sample_positions[step_seq_idx].played = 1;
                // printf("SWING NORM tick %% PPQN: %d\n", mixr->tick % PPQN);
            }
        }
        else {
            drumr->sample_positions[step_seq_idx].playing = 1;
            drumr->sample_positions[step_seq_idx].played = 1;
        }
    }

    //for (int i = 0; i < DRUM_PATTERN_LEN; i++) {
    for (int i = DRUM_PATTERN_LEN - 1; i >= 0; i--) {
        if (drumr->sample_positions[i].playing) {
            val +=
                // drumr->buffer[drumr->sample_positions[i].position++] /
                drumr->buffer[drumr->sample_positions[i].position] /
                2147483648.0; // convert from 16bit in to double between 0 and 1
            drumr->sample_positions[i].position =
                drumr->sample_positions[i].position + drumr->channels;
            if ((int)drumr->sample_positions[i].position ==
                drumr->bufsize) { // end of playback - so reset
                drumr->sample_positions[i].playing = 0;
                drumr->sample_positions[i].position = 0;
            }
        }
    }

    if (mixr->sixteenth_note_tick != drumr->tick) {
        int prev_note = step_seq_idx - 1;
        if (prev_note == -1)
            prev_note = 15;

        drumr->sample_positions[prev_note].played = 0;
        drumr->tick = mixr->sixteenth_note_tick;

        if (drumr->tick % 16 == 0) {
            drumr->cur_pattern_num =
                (drumr->cur_pattern_num + 1) % drumr->num_patterns;

            // printf("MOD16LIFE CHANGE!\n");
            // printf("GAMEOFLIFE_SETTING %d", drumr->game_of_life_on);
            if (drumr->game_of_life_on) {
                // printf("LIFE CHANGE!\n");
                next_life_generation(drumr);
                if (drumr->game_generation++ > 4) {
                    drumr->patterns[drumr->cur_pattern_num] = seed_pattern();
                    drumr->game_generation = 0;
                }
            }
            else {
                // printf("NOT ON LIFE\n");
            }
        }
    }

    val = effector(&drumr->sound_generator, val);
    val = envelopor(&drumr->sound_generator, val);

    return val * drumr->vol;
}

void drum_status(void *self, char *status_string)
{
    DRUM *drumr = self;
    char spattern[17];
    char_binary_version_of_int(drumr->patterns[drumr->cur_pattern_num],
                               spattern);
    snprintf(status_string, 119, ANSI_COLOR_CYAN
             "[%s]\t[%s] vol: %.2lf life_on: %d" ANSI_COLOR_RESET,
             basename(drumr->filename), spattern, drumr->vol,
             drumr->game_of_life_on);
}

double drum_getvol(void *self)
{
    DRUM *drumr = self;
    return drumr->vol;
}

void drum_setvol(void *self, double v)
{
    DRUM *drumr = self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    drumr->vol = v;
}

void swingrrr(void *self, int swing_setting)
{
    DRUM *drumr = self;
    // printf("SWING CALLED FOR %d\n", swing_setting);
    if (drumr->swing) {
        printf("swing OFF\n");
        drumr->swing = 0;
    }
    else {
        printf("Swing ON to %d\n", swing_setting);
        drumr->swing = 1;
        drumr->swing_setting = swing_setting;
    }
}

void add_char_pattern(void *self, char *pattern)
{
    DRUM *drumr = self;
    pattern_char_to_int(pattern, &drumr->patterns[drumr->num_patterns++]);
}

void add_int_pattern(void *self, int pattern)
{
    DRUM *drumr = self;
    drumr->patterns[drumr->num_patterns++] = pattern;
}

int seed_pattern()
{
    int pattern = 0;
    for (int i = 0; i < DRUM_PATTERN_LEN; i++) {
        int randy = rand() % 100;
        // printf("RANDY %d\n", randy);
        if (randy > 50) {
            pattern = pattern | (1 << i);
        }
    }
    return pattern;
}
