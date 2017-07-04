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
#include "envelope.h"
#include "fx.h"
#include "looper.h"
#include "minisynth.h"
#include "mixer.h"
#include "sample_sequencer.h"
#include "sbmsg.h"
#include "sequencer_utils.h"
#include "sound_generator.h"
#include "spork.h"
#include "synthdrum_sequencer.h"
#include "utils.h"

extern ENVSTREAM *ampstream;

extern mixer *mixr;

extern const char *key_names[NUM_KEYS];
extern const int key_midi_mapping[NUM_KEYS];
extern const compat_key_list compat_keys[NUM_KEYS];

mixer *new_mixer()
{
    mixer *mixr = (mixer *)calloc(1, sizeof(mixer));
    if (mixr == NULL) {
        printf("Nae mixer, fucked up!\n");
        return NULL;
    }

    mixr->volume = 0.7;
    mixer_update_bpm(mixr, DEFAULT_BPM);
    mixr->m_midi_controller_mode =
        KEY_MODE_ONE; // dunno whether this should be on mixer or synth
    mixr->midi_control_destination = NONE;

    // the lifetime of these booleans is a single sample
    mixr->is_midi_tick = true;
    mixr->start_of_loop = true;
    mixr->is_thirtysecond = true;
    mixr->is_sixteenth = true;
    mixr->is_eighth = true;
    mixr->is_quarter = true;

    mixr->scene_mode = false;
    mixr->scene_start_pending = false;
    mixr->scenes[0].num_bars_to_play = 4;
    mixr->num_scenes = 1;
    mixr->current_scene = -1;

    mixr->key = C_MAJOR;

    return mixr;
}

void mixer_ps(mixer *mixr)
{
    printf(COOL_COLOR_MAUVE
           "::::: [" ANSI_COLOR_WHITE "MIXING dESK" COOL_COLOR_MAUVE
           "] Volume: " ANSI_COLOR_WHITE "%.2f" COOL_COLOR_MAUVE
           "] Key: " ANSI_COLOR_WHITE "%s" COOL_COLOR_MAUVE
           " // BPM: " ANSI_COLOR_WHITE "%.2f" COOL_COLOR_MAUVE
           " // TICK: " ANSI_COLOR_WHITE "%d" COOL_COLOR_MAUVE
           " // Qtick: " ANSI_COLOR_WHITE "%d" COOL_COLOR_MAUVE
           " // Scene: " ANSI_COLOR_WHITE "%d" COOL_COLOR_MAUVE
           " // Debug: " ANSI_COLOR_WHITE "%s" COOL_COLOR_MAUVE " :::::\n"
           "::::: PPQN: %d PPSIXTEENTH: %d PPTWENTYFOURTH: %d PPBAR: %d PPNS: "
           "%d " ANSI_COLOR_RESET,
           mixr->volume, key_names[mixr->key], mixr->bpm, mixr->midi_tick,
           mixr->sixteenth_note_tick, mixr->current_scene,
           mixr->debug_mode ? "true" : "false", PPQN, PPSIXTEENTH,
           PPTWENTYFOURTH, PPBAR, PPNS);

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
        printf(ANSI_COLOR_WHITE "::::: [scene mode: %s] .....] - \n",
               mixr->scene_mode ? "true" : "false");
        for (int i = 0; i < mixr->num_scenes; i++) {
            printf("::::: [%d] - %d bars - ", i,
                   mixr->scenes[i].num_bars_to_play);
            for (int j = 0; j < mixr->scenes[i].num_tracks; j++) {
                if (mixr->scenes[i].soundgen_tracks[j].soundgen_num != -1) {
                    printf(
                        "(%d,%d)",
                        mixr->scenes[i].soundgen_tracks[j].soundgen_num,
                        mixr->scenes[i].soundgen_tracks[j].soundgen_track_num);
                }
            }
            printf("\n");
        }
        printf(ANSI_COLOR_RESET "\n");
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
                for (int j = 0; j < mixr->sound_generators[i]->effects_num;
                     j++) {
                    fx *f = mixr->sound_generators[i]->effects[j];
                    if (f->enabled)
                        printf(COOL_COLOR_YELLOW);
                    else
                        printf(ANSI_COLOR_RESET);
                    char fx_status[512];
                    f->status(f, fx_status);
                    printf("\n      [fx %d:%d %s]", i, j, fx_status);
                }
                printf(ANSI_COLOR_RESET);
                printf(COOL_COLOR_GREEN);
                for (int j = 0; j < mixr->sound_generators[i]->envelopes_num;
                     j++) {
                    printf("[envelope]\n");
                }
                printf(ANSI_COLOR_RESET);
            }
            printf("\n\n");
        }
    }

    printf(ANSI_COLOR_RESET);
}

const compat_key_list *mixer_get_compat_notes(mixer *mixr)
{
    return &compat_keys[mixr->key];
}

void mixer_update_bpm(mixer *mixr, int bpm)
{
    printf("Changing bpm to %d\n", bpm);
    mixr->bpm = bpm;
    mixr->samples_per_midi_tick = (60.0 / bpm * SAMPLE_RATE) / PPQN;
    mixr->midi_ticks_per_ms = PPQN * bpm / 60000;
    mixr->loop_len_in_samples = mixr->samples_per_midi_tick * PPBAR;
    mixr->loop_len_in_ticks = PPBAR;
    mixr->sixteenth_note_tick = -1;
    mixr->midi_tick = -1;
    mixr->cur_sample = 0;

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

int add_sound_generator(mixer *mixr, SOUNDGEN *sg)
{
    SOUNDGEN **new_soundgens = NULL;

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

int mixer_add_spork(mixer *mixr, double freq)
{
    printf("Adding an SPORK, mo!\n");
    spork *s = new_spork(freq);
    return add_sound_generator(mixr, (SOUNDGEN *)s);
}

int mixer_add_synthdrum(mixer *mixr, int pattern)
{
    printf("Adding an SYNTHYDRUM, yo!\n");
    synthdrum_sequencer *sds = new_synthdrum_seq();
    // sds->m_seq.patterns[sds->m_seq.num_patterns++] = pattern;
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

    if (mixr->cur_sample % ((PPSIXTEENTH / 2) * mixr->samples_per_midi_tick) ==
        0) { // thirty second
        mixr->is_thirtysecond = true;
    }
    else {
        mixr->is_thirtysecond = false;
    }

    if (mixr->cur_sample % (PPSIXTEENTH * mixr->samples_per_midi_tick) == 0) {
        mixr->sixteenth_note_tick++; // for seq machine resolution
        mixr->is_sixteenth = true;
    }
    else {
        mixr->is_sixteenth = false;
    }

    if (mixr->cur_sample % (PPSIXTEENTH * 2 * mixr->samples_per_midi_tick) ==
        0) {
        mixr->is_eighth = true;
    }
    else {
        mixr->is_eighth = false;
    }

    if (mixr->cur_sample % (PPQN * mixr->samples_per_midi_tick) == 0) {
        mixr->is_quarter = true;
    }
    else {
        mixr->is_quarter = false;
    }

    if (mixr->cur_sample % (PPBAR * mixr->samples_per_midi_tick) == 0) {
        mixr->start_of_loop = true;
        if (mixr->start_of_loop && (mixr->sixteenth_note_tick % 16 != 0)) {
            printf("BUG! START OF LOOP - sample: %d, midi_tick: %d sixteenth: "
                   "%d\n",
                   mixr->cur_sample, mixr->midi_tick,
                   mixr->sixteenth_note_tick);
            mixr->sixteenth_note_tick = 0;
            mixr->midi_tick = 0;
        }
        if (mixr->debug_mode) {
            printf("START OF LOOP - sample: %d, midi_tick: %d sixteenth: %d\n",
                   mixr->cur_sample, mixr->midi_tick,
                   mixr->sixteenth_note_tick);
        }
    }
    else {
        mixr->start_of_loop = false;
    }

    // if (mixr->scene_mode && mixr->start_of_loop) {
    if (mixr->start_of_loop) {
        // printf("Top of the bar\n");

        if (mixr->scene_start_pending) {
            mixer_play_scene(mixr, mixr->current_scene);
            mixr->scene_start_pending = false;
        }

        // if (mixr->current_scene_bar_count >=
        //        mixr->scenes[mixr->current_scene].num_bars_to_play ||
        //    mixr->scene_start_pending) {
        //    mixr->current_scene = (mixr->current_scene + 1) %
        //    mixr->num_scenes;
        //    mixr->current_scene_bar_count = 0;

        //    // printf("SCENE MODE CHANGE %d!\n", mixr->current_scene);
        //}
        // mixr->current_scene_bar_count++;
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

void mixer_play_scene(mixer *mixr, int scene_num)
{
    scene *s = &mixr->scenes[scene_num];
    for (int i = 0; i < mixr->soundgen_num; i++) {
        if (!mixer_is_soundgen_in_scene(i, s) &&
            mixer_is_valid_soundgen_num(mixr, i)) {
            mixr->sound_generators[i]->stop(mixr->sound_generators[i]);
        }
    }

    for (int i = 0; i < s->num_tracks; i++) {
        int soundgen_num = s->soundgen_tracks[i].soundgen_num;
        if (soundgen_num == -1) {
            continue;
        }
        int soundgen_track_num = s->soundgen_tracks[i].soundgen_track_num;
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
            mixr->sound_generators[soundgen_num]->start(
                mixr->sound_generators[soundgen_num]);
            mixr->sound_generators[soundgen_num]->make_active_track(
                mixr->sound_generators[soundgen_num], soundgen_track_num);
        }
        else {
            printf(
                "Oh, a deleted soundgen, better remove that from the scene\n");
            s->soundgen_tracks[i].soundgen_num = -1;
        }
    }
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
            minisynth *ms = (minisynth *)sg;
            minisynth_del_self(ms);
            break;
        case (LOOPER_TYPE):
            printf("DELALOOPER!\n");
            looper *l = (looper *)sg;
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
            synthdrum_sequencer *sds = (synthdrum_sequencer *)sg;
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

bool mixer_is_valid_soundgen_track_num(mixer *mixr, int soundgen_num,
                                       int track_num)
{
    printf("Inside is valid soundgen track num..\n");
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
        track_num < mixr->sound_generators[soundgen_num]->get_num_tracks(
                        mixr->sound_generators[soundgen_num]))
        return true;

    return false;
}

int mixer_add_scene(mixer *mixr, int num_bars)
{
    if (mixr->num_scenes >= MAX_SCENES) {
        printf("Dingie mate\n");
        return false;
    }
    printf("NUM BARSZZ! %d\n", num_bars);

    mixr->scenes[mixr->num_scenes].num_bars_to_play = num_bars;

    return mixr->num_scenes++;
}

bool mixer_add_soundgen_track_to_scene(mixer *mixr, int scene_num,
                                       int soundgen_num, int soundgen_track)
{
    if (!mixer_is_valid_scene_num(mixr, scene_num)) {
        printf("%d is not a valid scene number\n", scene_num);
        return false;
    }
    if (!mixer_is_valid_soundgen_track_num(mixr, soundgen_num,
                                           soundgen_track)) {
        printf("%d is not a valid soundgen number\n", soundgen_num);
        return false;
    }

    if (mixr->scenes[scene_num].num_tracks >= MAX_TRACKS_PER_SCENE) {
        printf("Too many tracks for this scene\n");
        return false;
    }

    scene *s = &mixr->scenes[scene_num];
    s->soundgen_tracks[s->num_tracks].soundgen_num = soundgen_num;
    s->soundgen_tracks[s->num_tracks].soundgen_track_num = soundgen_track;

    s->num_tracks++;

    printf("ALL good here\n");
    return true;
}

bool mixer_rm_soundgen_track_from_scene(mixer *mixr, int scene_num,
                                        int soundgen_num, int soundgen_track)
{
    if (!mixer_is_valid_scene_num(mixr, scene_num)) {
        printf("%d is not a valid scene number\n", scene_num);
        return false;
    }
    if (!mixer_is_valid_soundgen_track_num(mixr, soundgen_num,
                                           soundgen_track)) {
        printf("%d is not a valid soundgen number\n", soundgen_num);
        return false;
    }

    scene *s = &mixr->scenes[scene_num];
    for (int i = 0; i < s->num_tracks; i++) {
        if (s->soundgen_tracks[i].soundgen_num == soundgen_num &&
            s->soundgen_tracks[i].soundgen_track_num == soundgen_track) {
            s->soundgen_tracks[i].soundgen_num = -1;
            return true;
        }
    }

    return false;
}
bool mixer_is_soundgen_in_scene(int soundgen_num, scene *s)
{
    for (int i = 0; i < s->num_tracks; i++) {
        if (soundgen_num == s->soundgen_tracks[i].soundgen_num)
            return true;
    }
    return false;
}

bool mixer_cp_scene(mixer *mixr, int scene_num_from, int scene_num_to)
{
    if (!mixer_is_valid_scene_num(mixr, scene_num_from)) {
        printf("%d is not a valid scene number\n", scene_num_from);
        return false;
    }
    if (!mixer_is_valid_scene_num(mixr, scene_num_to)) {
        printf("%d is not a valid scene number\n", scene_num_from);
        return false;
    }

    mixr->scenes[scene_num_to] = mixr->scenes[scene_num_from];

    return true;
}
