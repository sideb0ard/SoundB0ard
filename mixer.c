#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <portaudio.h>

#include "bitwize.h"
#include "defjams.h"
#include "drumr.h"
#include "effect.h"
#include "envelope.h"
#include "mixer.h"
#include "nanosynth.h"
#include "sampler.h"
#include "sbmsg.h"
#include "sound_generator.h"

extern ENVSTREAM *ampstream;
extern pthread_mutex_t midi_tick_lock;
extern pthread_cond_t midi_tick_cond;

mixer *new_mixer()
{
    mixer *mixr = NULL;
    mixr = calloc(1, sizeof(mixer));
    mixr->volume = 0.7;
    mixr->bpm = DEFAULT_BPM;
    mixr->samples_per_midi_tick = (60.0 / DEFAULT_BPM * SAMPLE_RATE) / PPQN;
    mixr->tick = 0;
    mixr->cur_sample = 0;
    mixr->keyboard_octave = 3;
    mixr->has_active_nanosynth = 0;
    if (mixr == NULL) {
        printf("Nae mixer, fucked up!\n");
        return NULL;
    }
    return mixr;
}

void mixer_ps(mixer *mixr)
{
    printf(ANSI_COLOR_WHITE
           "::::: Mixing Desk (Volume: %f // BPM: %d // TICK: %d // Qtick: %d) "
           "(Delay On: %d) "
           ":::::\n" ANSI_COLOR_RESET,
           mixr->volume, mixr->bpm, mixr->tick, mixr->sixteenth_note_tick, mixr->delay_on);
    printf(ANSI_COLOR_GREEN "::::: effects: %d :::::\n" ANSI_COLOR_RESET,
           mixr->effects_num);
    for (int i = 0; i < mixr->soundgen_num; i++) {
        char ss[240];
        mixr->sound_generators[i]->status(mixr->sound_generators[i], ss);
        printf("[%d] - %s\n", i, ss);
    }
}

void mixer_update_bpm(mixer *mixr, int bpm)
{
    mixr->bpm = bpm;
    mixr->samples_per_midi_tick = (60.0 / bpm * SAMPLE_RATE) / PPQN;
}

void delay_toggle(mixer *mixr)
{
    mixr->delay_on = abs(1 - mixr->delay_on);
    printf("MIXER VOL DELAY: %d!\n", mixr->delay_on);
}

void mixer_vol_change(mixer *mixr, float vol)
{
    printf("MIXER VOL CHANGE!\n");
    if (vol >= 0.0 && vol <= 1.0) {
        printf("PASSED THA CHALLEND WITH %F!\n", vol);
        mixr->volume = vol;
    }
}

void vol_change(mixer *mixr, int sg, float vol)
{
    if (sg > (mixr->soundgen_size - 1))
        return;
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

        new_effects =
            realloc(mixr->effects, mixr->effects_size * sizeof(EFFECT *));
        if (new_effects == NULL) {
            printf("Ooh, burney - cannae allocate memory for new sounds");
            return -1;
        }
        else {
            mixr->effects = new_effects;
        }
    }

    EFFECT *e = new_delay(0.2, DELAY1);
    if (e == NULL) {
        perror("Couldn't create effect");
        return -1;
    }
    mixr->effects[mixr->effects_num] = e;
    printf("done adding effect\n");
    return mixr->effects_num++;
}

int add_sound_generator(mixer *mixr, SBMSG *sbm)
{
    SOUNDGEN **new_soundgens = NULL;
    if (mixr->soundgen_size <= mixr->soundgen_num) {
        if (mixr->soundgen_size == 0) {
            mixr->soundgen_size = DEFAULT_ARRAY_SIZE;
        }
        else {
            mixr->soundgen_size *= 2;
        }

        new_soundgens = realloc(mixr->sound_generators,
                                mixr->soundgen_size * sizeof(SOUNDGEN *));
        if (new_soundgens == NULL) {
            printf("Ooh, burney - cannae allocate memory for new sounds");
            return -1;
        }
        else {
            mixr->sound_generators = new_soundgens;
        }
    }
    mixr->sound_generators[mixr->soundgen_num] = sbm->sound_generator;
    return mixr->soundgen_num++;
}

int add_bitwize(mixer *mixr, int pattern)
{

    BITWIZE *new_bitw = new_bitwize(pattern);
    if (new_bitw == NULL) {
        printf("BITBARF!\n");
        return -1;
    }

    SBMSG *m = new_sbmsg();
    if (m == NULL) {
        free(new_bitw);
        printf("MBITBARF!\n");
        return -1;
    }

    m->sound_generator = (SOUNDGEN *)new_bitw;
    printf("Added bitwize gen!\n");
    return add_sound_generator(mixr, m);
}

int add_nanosynth(mixer *mixr)
{
    printf("Adding a Nanosynth...\n");
    nanosynth *ns = new_nanosynth();
    if (ns == NULL) {
        printf("Barfed on nanosynth creation\n");
        return -1;
    }
    SBMSG *m = new_sbmsg();
    if (m == NULL) {
        free(ns);
        printf("MBARF!\n");
        return -1;
    }
    m->sound_generator = (SOUNDGEN *)ns;
    return add_sound_generator(mixr, m);
}

int add_drum(mixer *mixr, char *filename, char *pattern)
{
    // preliminary setup
    char cwd[1024];
    getcwd(cwd, 1024);
    char full_filename[strlen(filename) + strlen(cwd) +
                       7]; // 7 == '/wavs/' is 6 and 1 for '\0'
    strcpy(full_filename, cwd);
    strcat(full_filename, "/wavs/");
    strcat(full_filename, filename);

    DRUM *ndrum = new_drumr(full_filename, pattern);
    if (ndrum == NULL) {
        printf("Barfed on drum creation\n");
        return -1;
    }
    SBMSG *m = new_sbmsg();
    if (m == NULL) {
        free(ndrum);
        printf("MBARF!\n");
        return -1;
    }

    m->sound_generator = (SOUNDGEN *)ndrum;
    return add_sound_generator(mixr, m);
}

int add_sampler(mixer *mixr, char *filename, double loop_len)
{
    // preliminary setup
    char cwd[1024];
    getcwd(cwd, 1024);
    char full_filename[strlen(filename) + strlen(cwd) +
                       7]; // 7 == '/wavs/' is 6 and 1 for '\0'
    strcpy(full_filename, cwd);
    strcat(full_filename, "/wavs/");
    strcat(full_filename, filename);

    printf("ADD SAMPLER - LOOP LEN %f\n", loop_len);
    SAMPLER *nsampler = new_sampler(full_filename, loop_len);
    if (nsampler == NULL) {
        printf("Barfed on sampler creation\n");
        return -1;
    }
    SBMSG *m = new_sbmsg();
    if (m == NULL) {
        free(nsampler);
        printf("SAMPLMBARF!\n");
        return -1;
    }

    m->sound_generator = (SOUNDGEN *)nsampler;
    return add_sound_generator(mixr, m);
}

// void gen_next(mixer* mixr, int framesPerBuffer, float* out)
double gen_next(mixer *mixr)
{
    mixr->cur_sample++; // called once ever SAMPLE_RATE -> the basis of my clock
    if (mixr->cur_sample % mixr->samples_per_midi_tick == 0) {
        pthread_mutex_lock(&midi_tick_lock);
        mixr->tick++; // 1 midi tick (or pulse)
        if (mixr->tick % (PPQN / 4) == 0) {
            mixr->sixteenth_note_tick++; // for drum machine resolution
        }
        pthread_cond_broadcast(&midi_tick_cond);
        pthread_mutex_unlock(&midi_tick_lock);
    }

    double output_val = 0.0;
    if (mixr->soundgen_num > 0) {
        for (int i = 0; i < mixr->soundgen_num; i++) {
            output_val +=
                mixr->sound_generators[i]->gennext(mixr->sound_generators[i]);
        }
    }

    return mixr->volume * (output_val / 1.53);
}
