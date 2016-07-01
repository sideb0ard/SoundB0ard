#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "algoriddim.h"
#include "bpmrrr.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
extern bpmrrr *b;
extern GTABLE *sine_table;
extern GTABLE *tri_table;
extern GTABLE *square_table;
extern GTABLE *saw_up_table;
extern GTABLE *saw_down_table;

extern pthread_cond_t bpm_cond;
extern pthread_mutex_t bpm_lock;

static char *rev_lookup[12] = {"c",  "c#", "d",  "d#", "e",  "f",
                               "f#", "g",  "g#", "a",  "a#", "b"};

algo *new_algo()
{
    algo *a = NULL;
    a = calloc(1, sizeof(algo));
    if (a == NULL) {
        fprintf(stderr, "Nae memory for that song, mate..\n");
        return NULL;
    }
    return a;
}

void play_melody(const int osc_num, int *mlock, int *note, double *notes,
                 const int note_len)
{
    if (!*mlock) {

        *mlock = 1;
        *note = (*note + 1) % note_len;
        int randy = rand() % 100;
        if (randy > 90)
            freq_change(mixr, osc_num, (2 * notes[*note]));
        else if (randy < 10)
            freq_change(mixr, osc_num, 0);
        else
            freq_change(mixr, osc_num, notes[*note]);
    }
}

void fplay_melody(const int sg_num, int *mlock, int *note, double *notes,
                  const int note_len)
{
    if (!*mlock) {

        *mlock = 1;
        *note = (*note + 1) % note_len;
        // freq_change(mixr, osc_num, (notes[*note]));
        int randy = rand() % 100;
        if (randy > 90)
            mfm(mixr->sound_generators[sg_num], "car", (2 * notes[*note]));
        else
            mfm(mixr->sound_generators[sg_num], "car", (notes[*note]));
    }
}

melody_msg *new_melody_msg(double *freqs, int melody_note_len, int loop_len)
{
    melody_msg *m = calloc(1, sizeof(melody_msg));
    m->melody = freqs;
    m->osc_num = 0;
    m->melody_note_len = melody_note_len;
    m->melody_loop_len = loop_len;
    m->melody_play_lock = 0;
    m->melody_cur_note = 0;
    return m;
}

void *loop_run(void *m)
{
    melody_msg *mmsg = (melody_msg *)m;
    printf("LOOP RUN CALLED - got me a msg: %f, %f, %d\n", mmsg->melody[0],
           mmsg->melody[1], mmsg->melody_note_len);

    int osc_num = add_osc(mixr, mmsg->melody[0], sine_table);
    mmsg->osc_num = osc_num;
    do {
    } while (b->cur_tick % TICKS_PER_BAR != 0);
    faderrr(osc_num, UP);

    srand(time(0));

    while (1) {
        if (b->cur_tick % (TICKS_PER_BAR * (mmsg->melody_loop_len)) == 0) {
            play_melody(mmsg->osc_num, &mmsg->melody_play_lock,
                        &mmsg->melody_cur_note, mmsg->melody,
                        mmsg->melody_note_len);
        }
        else if (mmsg->melody_play_lock) {
            mmsg->melody_play_lock = 0;
        }
        pthread_mutex_lock(&bpm_lock);
        pthread_cond_wait(&bpm_cond, &bpm_lock);
        pthread_mutex_unlock(&bpm_lock);
    }
}

void *randdrum_run(void *m)
{
    SBMSG *msg = (SBMSG *)m;
    int drum_num = msg->sound_gen_num;
    int looplen = msg->looplen;
    printf(
        "RANDRUN CALLED - got me a msg: drumnum %d - with length of %d bars\n",
        drum_num, looplen);
    int changed = 0;

    while (1) {
        if (b->cur_tick % (TICKS_PER_BAR * looplen) == 0) {
            if (!changed) {
                changed = 1;
                int pattern = rand() % 65535; // max for an unsigned int
                // printf("My rand num %d\n", pattern);
                update_pattern(mixr->sound_generators[drum_num], pattern);
            }
        }
        else {
            changed = 0;
        }
        pthread_mutex_lock(&bpm_lock);
        pthread_cond_wait(&bpm_cond, &bpm_lock);
        pthread_mutex_unlock(&bpm_lock);
    }
}

void *floop_run(void *m)
{
    melody_msg *mmsg = (melody_msg *)m;
    printf("FLOOP RUN CALLED - got me a msg: %f, %f, %d MOD FREQ: %f\n",
           mmsg->melody[0], mmsg->melody[1], mmsg->melody_note_len,
           mmsg->mod_freq);

    int fm_one = add_fm(mixr, mmsg->mod_freq, mmsg->melody[0]);
    mmsg->osc_num = fm_one;
    do {
    } while (b->cur_tick % TICKS_PER_BAR != 0);
    faderrr(fm_one, UP);
    // sleep(3);
    srand(time(NULL));

    while (1) {
        if (b->cur_tick % (TICKS_PER_BAR * (mmsg->melody_loop_len)) == 0) {
            fplay_melody(mmsg->osc_num, &mmsg->melody_play_lock,
                         &mmsg->melody_cur_note, mmsg->melody,
                         mmsg->melody_note_len);
        }
        else if (mmsg->melody_play_lock) {
            mmsg->melody_play_lock = 0;
        }
        pthread_mutex_lock(&bpm_lock);
        pthread_cond_wait(&bpm_cond, &bpm_lock);
        pthread_mutex_unlock(&bpm_lock);
    }
}

void *algo_run(void *a)
{
    (void)a;

    srandom(time(0));

    static double ma_numbers[3];
    double ma_notes[9];

    char *note = "c";
    // chordie(note);
    int root_note_num = notelookup(note);
    int second_note_num = (root_note_num + 4) % 12;
    int third_note_num = (second_note_num + 3) % 12;

    char rootnote[4];
    char sec_note[4];
    char thr_note[4];
    strcpy(rootnote, rev_lookup[root_note_num]);
    strcpy(sec_note, rev_lookup[second_note_num]);
    strcpy(thr_note, rev_lookup[third_note_num]);

    ma_numbers[0] = freqval(strcat(rootnote, "4"));
    ma_numbers[1] = freqval(strcat(sec_note, "4"));
    ma_numbers[2] = freqval(strcat(thr_note, "4"));

    int divs[3] = {7, 4, 3};
    int notes_idx = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            // printf("[%d] %.2f = %d\n", notes_idx++, ma_numbers[i], divs[j]);
            ma_notes[notes_idx++] = ma_numbers[i] / divs[j];
        }
    }

    for (int i = 0; i < 9; i++) {
        printf("NOTESZZZ %.2f\n", ma_notes[i]);
    }

    int num_of_sigs = (random() % 4) + 1;
    printf("Num of sigs: %d\n", num_of_sigs);

    for (int i = 0; i < num_of_sigs; i++) {
        int fm = add_fm(mixr, ma_notes[random() % 9], ma_notes[random() % 9]);
        int env_type = 0;
        int bars = (random() % 3) + 1;
        add_delay_soundgen(mixr->sound_generators[fm], bars, env_type);
        do {
        } while (b->cur_tick % TICKS_PER_BAR != 0);
        faderrr(fm, UP);
    }

    int changed_lock;

    while (1) {
        if (b->cur_tick % TICKS_PER_BAR == 0) {
            if (!changed_lock) {
                changed_lock = 1;
                int ma_sig = random() % num_of_sigs;
                double ma_new_note = ma_notes[random() % 9];
                if (random() % 100 > 50) {
                    ma_new_note *= 2;
                    printf("DOUBLE! %.2f\n", ma_new_note);
                }
                int car_or_mod = (random() % 100 > 70) ? 0 : 1; // favor mod
                printf("Change sig %d to %.2f\n", ma_sig, ma_new_note);
                if (car_or_mod) {
                    printf("CAR!\n");
                    mfm(mixr->sound_generators[ma_sig], "car", ma_new_note);
                }
                else {
                    printf("MOD!\n");
                    mfm(mixr->sound_generators[ma_sig], "mod", ma_new_note);
                }

                // delay
                // if ( random() % 100 > 30 ) {
                //  ma_sig = random()%num_of_sigs;
                //  float delay_time = (float)random()/RAND_MAX + (random() %
                //  10);
                //  add_delay_soundgen(mixr->sound_generators[ma_sig],
                //  delay_time, DELAY);
                //  printf("Added delay for %d -> %.2f\n", ma_sig, delay_time);
                //}

                // envelope
                // if ( random() % 100 > 40 ) {
                //  ma_sig = random()%num_of_sigs;
                //  //int env_type = random() % 3;
                //  int env_type = 0;
                //  int bars = (random() % 3) + 1;

                //  add_delay_soundgen(mixr->sound_generators[ma_sig], bars,
                //  env_type);
                //  printf("Added Env for %d - type %d for %d bars\n", ma_sig,
                //  env_type, bars);
                //}

                // duck
                if (random() % 100 > 60) {
                    ma_sig = random() % num_of_sigs;
                    SBMSG *msg = new_sbmsg();
                    msg->sound_gen_num = ma_sig;
                    strncpy(msg->cmd, "duckrrr", 19);
                    thrunner(msg);
                }

                // 1 pick random sig_gen, 2 pick random ma_note, pick rand car
                // or mod, change
                //
                // pick > 70 duck one at random
                // pick > 40 delay one at random
                // pick > 60 env one at random
                // new_note
                // if (new_note >= notes_len)
                //  note_index = 0;
            }
        }
        else {
            changed_lock = 0;
        }
        pthread_mutex_lock(&bpm_lock);
        pthread_cond_wait(&bpm_cond, &bpm_lock);
        pthread_mutex_unlock(&bpm_lock);
    }
    return NULL;
}
