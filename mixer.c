#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include <portaudio.h>

#include "algorithm.h"
#include "chaosmonkey.h"
#include "defjams.h"
#include "effect.h"
#include "envelope.h"
#include "looper.h"
#include "minisynth.h"
#include "mixer.h"
#include "sample_sequencer.h"
#include "sbmsg.h"
#include "sequencer_utils.h"
#include "sound_generator.h"
#include "spork.h"
#include "synthdrum_sequencer.h"

extern ENVSTREAM *ampstream;

extern mixer *mixr;

mixer *new_mixer()
{
    mixer *mixr = (mixer *)calloc(1, sizeof(mixer));
    if (mixr == NULL) {
        printf("Nae mixer, fucked up!\n");
        return NULL;
    }

    mixr->volume = 0.7;
    mixer_update_bpm(mixr, DEFAULT_BPM);
    mixr->midi_tick = -1;
    mixr->cur_sample = 0;
    mixr->m_midi_controller_mode =
        KEY_MODE_ONE; // dunno whether this should be on mixer or synth
    mixr->midi_control_destination = NONE;

    // the lifetime of these booleans is a single sample
    mixr->is_midi_tick = true;
    mixr->start_of_loop = true;
    mixr->is_sixteenth = true;
    mixr->is_quarter = true;

    mixr->scenes[0].num_bars_to_play = 4;
    mixr->num_scenes = 1;
    mixr->current_scene = 0;

    return mixr;
}

void mixer_ps(mixer *mixr)
{
    printf(COOL_COLOR_MAUVE
           "::::: [" ANSI_COLOR_WHITE "MIXING dESK" COOL_COLOR_MAUVE
           "] Volume: " ANSI_COLOR_WHITE "%.2f" COOL_COLOR_MAUVE
           " // BPM: " ANSI_COLOR_WHITE "%.2f" COOL_COLOR_MAUVE
           " // TICK: " ANSI_COLOR_WHITE "%d" COOL_COLOR_MAUVE
           " // Qtick: " ANSI_COLOR_WHITE "%d" COOL_COLOR_MAUVE
           " // Debug: " ANSI_COLOR_WHITE "%s" COOL_COLOR_MAUVE " :::::\n"
           "::::: PPQN: %d PPSIXTEENTH: %d PPBAR: %d PPNS: %d " ANSI_COLOR_RESET,
           mixr->volume, mixr->bpm, mixr->midi_tick, mixr->sixteenth_note_tick,
           mixr->debug_mode ? "true" : "false", PPQN, PPSIXTEENTH, PPBAR, PPNS);

    if (mixr->env_var_count > 0) {
        printf(COOL_COLOR_GREEN "::::: Environment :::::\n");
        for (int i = 0; i < mixr->env_var_count; i++) {
            printf("%s - %d\n", mixr->environment[i].key,
                   mixr->environment[i].val);
        }
        printf(ANSI_COLOR_RESET);
    }
    printf("\n");

    if (mixr->num_scenes > 0) {
        printf("::::: [scene mode: %s] .....] - \n", mixr->scene_mode ? "true" : "false");
        for (int i = 0; i < mixr->num_scenes; i++) {
            printf("::::: [%d] - %d bars - ", i, mixr->scenes[i].num_bars_to_play);
            for (int j = 0; j < mixr->scenes[i].num_tracks; j++) {
                printf("(%d,%d)", mixr->scenes[i].soundgen_tracks[j].soundgen_num,
                                  mixr->scenes[i].soundgen_tracks[j].soundgen_track_num);
            }
           printf("\n"); 
        }
    }

    for (int i = 0; i < mixr->soundgen_num; i++) {
        if (mixr->sound_generators[i] != NULL) {
            wchar_t wss[MAX_PS_STRING_SZ];
            memset(wss, 0, MAX_PS_STRING_SZ);
            mixr->sound_generators[i]->status(mixr->sound_generators[i], wss);
            wprintf(WANSI_COLOR_WHITE "[%2d]" WANSI_COLOR_RESET "  %ls\n", i,
                    wss);
            if (mixr->sound_generators[i]->effects_num > 0 ||
                mixr->sound_generators[i]->envelopes_num > 0) {
                printf("      ");
                printf(COOL_COLOR_YELLOW);
                for (int j = 0; j < mixr->sound_generators[i]->effects_num;
                     j++) {
                    printf("[effect]-");
                }
                printf(ANSI_COLOR_RESET);
                printf(COOL_COLOR_GREEN);
                for (int j = 0; j < mixr->sound_generators[i]->envelopes_num;
                     j++) {
                    printf("[envelope]-");
                }
                printf(ANSI_COLOR_RESET);
                printf(">[out]");
            }
            printf("\n\n");
        }
    }

    printf(ANSI_COLOR_RESET);
}

void mixer_update_bpm(mixer *mixr, int bpm)
{
    printf("Changing bpm to %d\n", bpm);
    mixr->bpm = bpm;
    mixr->samples_per_midi_tick = (60.0 / bpm * SAMPLE_RATE) / PPQN;
    mixr->midi_ticks_per_ms = PPQN / ((60.0 / bpm) * 1000);
    mixr->loop_len_in_samples = mixr->samples_per_midi_tick * PPBAR;
    mixr->loop_len_in_ticks = PPBAR;
    for (int i = 0; i < mixr->soundgen_num; i++) {
        if (mixr->sound_generators[i] != NULL) {
            for (int j = 0; j < mixr->sound_generators[i]->envelopes_num; j++) {
                update_envelope_stream_bpm(
                    mixr->sound_generators[i]->envelopes[j]);
            }
            if (mixr->sound_generators[i]->type == LOOPER_TYPE) {
                looper_resample_to_loop_size(
                    (looper *)mixr->sound_generators[i]);
            }
        }
    }
}

// TODO - this has moved to minisynth
void mixer_toggle_midi_mode(mixer *mixr)
{
    mixr->m_midi_controller_mode =
        ++(mixr->m_midi_controller_mode) % MAX_NUM_KNOB_MODES;
}

void mixer_toggle_key_mode(mixer *mixr)
{
    mixr->m_key_controller_mode =
        ++(mixr->m_key_controller_mode) % MAX_NUM_KEY_MODES;
}

void delay_toggle(mixer *mixr)
{
    mixr->delay_on = abs(1 - mixr->delay_on);
    printf("MIXER VOL DELAY: %d!\n", mixr->delay_on);
}

void mixer_vol_change(mixer *mixr, float vol)
{
    printf("Changing volume to %f\n", vol);
    if (vol >= 0.0 && vol <= 1.0) {
        mixr->volume = vol;
    }
}

void vol_change(mixer *mixr, int sg, float vol)
{
    printf("SG: %d // soungen_num : %d\n", sg, mixr->soundgen_num);
    if (!mixer_is_valid_soundgen_num(mixr, sg)) {
        printf("Nah mate, returning\n");
        return;
    }
    mixr->sound_generators[sg]->setvol(mixr->sound_generators[sg], vol);
}

int add_effect(mixer *mixr)
{
    printf("Booya, adding a new effect!\n");
    EFFECT **new_effects = NULL;
    if (mixr->effects_size <= mixr->effects_num) {
        if (mixr->effects_size == 0) {
            mixr->effects_size = DEFAULT_ARRAY_SIZE;
        }
        else {
            mixr->effects_size *= 2;
        }

        new_effects = (EFFECT **)realloc(mixr->effects,
                                         mixr->effects_size * sizeof(EFFECT *));
        if (new_effects == NULL) {
            printf("Ooh, burney - cannae allocate memory for new sounds");
            return -1;
        }
        else {
            mixr->effects = new_effects;
        }
    }

    EFFECT *e = new_delay(200);
    if (e == NULL) {
        perror("Couldn't create effect");
        return -1;
    }
    mixr->effects[mixr->effects_num] = e;
    printf("done adding effect\n");
    return mixr->effects_num++;
}

int add_sound_generator(mixer *mixr, SOUNDGEN *sg)
{
    SOUNDGEN **new_soundgens = NULL;
    // TODO -- reuse any NULLs
    if (mixr->soundgen_size <= mixr->soundgen_num) {
        if (mixr->soundgen_size == 0) {
            mixr->soundgen_size = DEFAULT_ARRAY_SIZE;
        }
        else {
            mixr->soundgen_size *= 2;
        }

        new_soundgens = (SOUNDGEN **)realloc(
            mixr->sound_generators, mixr->soundgen_size * sizeof(SOUNDGEN *));
        if (new_soundgens == NULL) {
            printf("Ooh, burney - cannae allocate memory for new sounds");
            return -1;
        }
        else {
            mixr->sound_generators = new_soundgens;
        }
    }
    mixr->sound_generators[mixr->soundgen_num] = sg;
    return mixr->soundgen_num++;
}

int mixer_add_spork(mixer *mixr)
{
    printf("Adding an SPORK, mo!\n");
    spork *s = new_spork();
    return add_sound_generator(mixr, (SOUNDGEN *)s);
}

int mixer_add_synthdrum(mixer *mixr, int pattern)
{
    printf("Adding an SYNTHYDRUM, yo!\n");
    synthdrum_sequencer *sds = new_synthdrum_seq();
    //sds->m_seq.patterns[sds->m_seq.num_patterns++] = pattern;
    return add_sound_generator(mixr, (SOUNDGEN *)sds);
}

int add_algorithm(char *line)
{
    algorithm *a = new_algorithm(line);
    return add_sound_generator(mixr, (SOUNDGEN *)a);
}

int add_chaosmonkey(int soundgen)
{
    chaosmonkey *cm = new_chaosmonkey(soundgen);
    return add_sound_generator(mixr, (SOUNDGEN *)cm);
}

int add_minisynth(mixer *mixr)
{
    printf("Adding a MINISYNTH!!...\n");
    minisynth *ms = new_minisynth();
    return add_sound_generator(mixr, (SOUNDGEN *)ms);
}

int add_seq_char_pattern(mixer *mixr, char *filename, char *pattern)
{
    // preliminary setup
    char cwd[1024];
    getcwd(cwd, 1024);
    char full_filename[strlen(filename) + strlen(cwd) +
                       7]; // 7 == '/wavs/' is 6 and 1 for '\0'
    strcpy(full_filename, cwd);
    strcat(full_filename, "/wavs/");
    strcat(full_filename, filename);

    sample_sequencer *nseq =
        new_sample_seq_from_char_pattern(full_filename, pattern);
    if (nseq == NULL) {
        printf("Barfed on seq creation\n");
        return -1;
    }
    return add_sound_generator(mixr, (SOUNDGEN *)nseq);
}

int add_looper(mixer *mixr, char *filename, double loop_len)
{
    printf("ADD looper - LOOP LEN %f\n", loop_len);
    looper *l = new_looper(filename, loop_len);
    if (l == NULL) {
        printf("Barfed on looper creation\n");
        return -1;
    }
    return add_sound_generator(mixr, (SOUNDGEN *)l);
}

double mixer_gennext(mixer *mixr)
{
    if (mixr->cur_sample % mixr->samples_per_midi_tick == 0) {
        mixr->midi_tick++; // 1 midi tick (or pulse)
        mixr->is_midi_tick = true;
    }
    else {
        mixr->is_midi_tick = false;
    }

    if (mixr->cur_sample % (PPBAR * mixr->samples_per_midi_tick) == 0) {
        mixr->start_of_loop = true;
    }
    else {
        mixr->start_of_loop = false;
    }

    if (mixr->cur_sample % (PPSIXTEENTH * mixr->samples_per_midi_tick) == 0) {
        mixr->is_sixteenth = true;
        mixr->sixteenth_note_tick++; // for seq machine resolution
    }
    else {
        mixr->is_sixteenth = false;
    }

    double output_val = 0.0;
    if (mixr->soundgen_num > 0) {
        for (int i = 0; i < mixr->soundgen_num; i++) {
            if (mixr->sound_generators[i] != NULL) {
                output_val += mixr->sound_generators[i]->gennext(
                    mixr->sound_generators[i]);
            }
        }
    }

    mixr->cur_sample++;

    return mixr->volume * (output_val / 1.53);
}

void update_environment(char *key, int val)
{
    int env_item_index = 0;
    bool is_update = false;
    for (int i = 0; i < mixr->env_var_count; i++) {
        if (strncmp(key, mixr->environment[i].key, ENVIRONMENT_KEY_SIZE) == 0) {
            is_update = true;
            env_item_index = i;
        }
    }
    if (is_update) {
        mixr->environment[env_item_index].val = val;
    }
    else {
        strncpy((char *)&mixr->environment[mixr->env_var_count].key, key,
                ENVIRONMENT_KEY_SIZE);
        mixr->environment[mixr->env_var_count].val = val;
        mixr->env_var_count++;
    }
}

int get_environment_val(char *key, int *return_val)
{
    for (int i = 0; i < mixr->env_var_count; i++) {
        if (strncmp(key, mixr->environment[i].key, ENVIRONMENT_KEY_SIZE) == 0) {
            *return_val = mixr->environment[i].val;
            return 0;
        }
    }
    return 1;
}

bool mixer_del_soundgen(mixer *mixr, int soundgen_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
        printf("MIXR!! Deleting SOUND GEN %d\n", soundgen_num);
        SOUNDGEN *sg = mixr->sound_generators[soundgen_num];
        mixr->sound_generators[soundgen_num] = NULL;
        switch (sg->type) {
        case (SYNTH_TYPE):
            printf("DELASYNTH!\n");
            minisynth *ms = (minisynth*) sg;
            minisynth_del_self(ms);
            break;
        case (LOOPER_TYPE):
            printf("DELALOOPER!\n");
            looper *l = (looper*) sg;
            looper_del_self(l);
            break;
        case (BITWIZE_TYPE):
            printf("DELABIT!\n");
            break;
        case (SEQUENCER_TYPE):
            printf("DELASEQ!\n");
            sample_sequencer *s = (sample_sequencer *)sg;
            sample_seq_del(s);
            break;
        case (SYNTHDRUM_TYPE):
            printf("DELASYNTHDRUM!\n");
            synthdrum_sequencer *sds = (synthdrum_sequencer*) sg;
            synthdrum_del_self(sds);
            break;
        case (ALGORITHM_TYPE):
            printf("DELALGO!\n");
            break;
        case (CHAOSMONKEY_TYPE):
            printf("DELAMONKEY!\n");
            break;
        case (SPORK_TYPE):
            printf("DELASPORKT!\n");
            break;
        case (NUM_SOUNDGEN_TYPE):
            // should never happen
            break;
        }
    }
    return true;
}

bool mixer_is_valid_soundgen_num(mixer *mixr, int soundgen_num)
{
    if (soundgen_num >= 0 && soundgen_num < mixr->soundgen_num &&
        mixr->sound_generators[soundgen_num] != NULL) {
        return true;
    }
    return false;
}

bool mixer_is_valid_scene_num(mixer *mixr, int scene_num)
{
    if (mixr->num_scenes > 0 && scene_num < mixr->num_scenes)
        return true;
    return false;
}

bool mixer_is_valid_soundgen_track_num(mixer *mixr, int soundgen_num, int track_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num)
        && track_num < mixr->sound_generators[soundgen_num]->get_num_tracks(mixr->sound_generators[soundgen_num]))
        return true;

    return false;
}

bool mixer_add_scene(mixer *mixr, int num_bars)
{
    if (mixr->num_scenes >= MAX_SCENES)
    {
        printf("Dingie mate\n");
        return false;
    }

    mixr->num_scenes++;
    mixr->scenes[mixr->current_scene++].num_bars_to_play = num_bars;
    // not setting scene mode true -- do that separately

    return true;
}

bool mixer_add_soundgen_track_to_scene(mixer *mixr, int scene_num, int soundgen_num, int soundgen_track)
{
    if (!mixer_is_valid_scene_num(mixr, scene_num)) {
        printf("%d is not a valid scene number\n", scene_num);
        return false;
    }
    if (!mixer_is_valid_soundgen_track_num(mixr, soundgen_num, soundgen_track)) {
        printf("%d is not a valid soundgen number\n", soundgen_num);
        return false;
    }

    if (mixr->scenes[scene_num].num_tracks < MAX_TRACKS_PER_SCENE) {
        printf("Too many tracks for this scene\n");
        return false;
    }

    scene *s = &mixr->scenes[scene_num];
    s->soundgen_tracks[s->num_tracks].soundgen_num = soundgen_num;
    s->soundgen_tracks[s->num_tracks].soundgen_track_num = soundgen_track;

    return true;
}
