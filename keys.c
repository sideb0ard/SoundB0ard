#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "bpmrrr.h"
#include "defjams.h"
#include "keys.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
extern bpmrrr *b;
extern int kernelq; // for timers

extern pthread_cond_t bpm_cond;
extern pthread_mutex_t bpm_lock;

melody_loop *new_melody_loop(int soundgen_num)
{
    melody_loop *l;
    l = (melody_loop *)calloc(1, sizeof(melody_loop));
    l->sig_num = soundgen_num;
    return l;
}

//static void alarm_handler(int signum)
//{
//    //printf("TIMER!! %d\n", signum);
//    FM *fm = (FM *)mixr->sound_generators[0];
//    eg_release(fm->env);
//}

void keys(int soundgen_num)
{
    printf("Entering Keys Mode for %d\n", soundgen_num);
    printf("Press 'q' or 'Esc' to go back to Run Mode\n");
    struct termios new_info, old_info;
    tcgetattr(0, &old_info);
    new_info = old_info;

    new_info.c_lflag &= (~ICANON & ~ECHO);
    new_info.c_cc[VMIN] = 1;
    new_info.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_info);

    FM *self = (FM *)mixr->sound_generators[soundgen_num];

    //signal(SIGALRM, alarm_handler);

    int ch = 0;
    int quit = 0;
    // int recording = 0;
    // int recording_started = 0;

    struct pollfd fdz[1];
    int pollret;

    fdz[0].fd = STDIN_FILENO;
    fdz[0].events = POLLIN;

    // melody_loop *mloop;

    while (!quit) {

        pollret = poll(fdz, 1, 0);
        if (pollret == -1) {
            perror("poll");
            return;
        }

        if (fdz[0].revents & POLLIN) {
            ch = getchar();
            if (ch == 27 || ch == 113) { // Esc or 'q'
                quit = 1;
            }
            // WRONG! dangerous
            // else if ( ch == 96 ) { // '`'
            //    printf("Changing WAVE form of synth\n");
            //    fm_change_osc_wave_form(self);
            //}
            else if (ch == 49) { // '1'
                change_octave(mixr->sound_generators[soundgen_num], DOWN);
            }
            else if (ch == 50) { // '2'
                change_octave(mixr->sound_generators[soundgen_num], UP);
            }
            else if (ch == 99) { // 'c'
                freqinc(self->lfo, DOWN);
            }
            else if (ch == 67) { // 'C'
                freqinc(self->lfo, UP);
            }
            else if (ch == 122) { // 'z'
                filter_adj_fc_control(self->filter->bc_filter, DOWN);
            }
            else if (ch == 90) { // 'Z'
                filter_adj_fc_control(self->filter->bc_filter, UP);
            }

            else { // try to play note
                double freq =
                    chfreqlookup(ch, mixr->sound_generators[soundgen_num]);
                if (freq != -1) {
                    play_note(soundgen_num, freq);
                }
                //alarm(2);
            }
        }
    }
    tcsetattr(0, TCSANOW, &old_info);
}

// melody_event* make_melody_event(double freq, int sg_num, int tick)
melody_event *make_melody_event(int tick, double freq, char note[4])
{
    melody_event *me;
    me = (melody_event *)calloc(1, sizeof(melody_event));
    me->tick = tick;

    me->freq = freq;
    strcpy(me->note, note);
    // me->note = note;

    return me;
}

void add_melody_event(melody_loop *mloop, melody_event *e)
{
    printf("ADDING melody event..\n");
    if (mloop->size < 100) {
        mloop->melody[mloop->size++] = e;
    }
}

void play_note(int sg_num, double freq)
{

    // struct timespec ts;
    // ts.tv_sec = 0;
    // ts.tv_nsec = 10000;

    keypress_on(mixr->sound_generators[sg_num], freq);
    // nanosleep(&ts, NULL);
    // mixr->sound_generators[sg_num];
    // keypress_off(mixr->sound_generators[sg_num]);
    //alarm(2);
}

void *play_melody_loop(void *m)
{
    melody_loop *mloop = (melody_loop *)m;

    printf("PLAY melody starting..\n");
    static int iteration = 1;

    int loop_started = 0;
    while (!loop_started) {
        pthread_mutex_lock(&bpm_lock);
        pthread_cond_wait(&bpm_cond, &bpm_lock);
        pthread_mutex_unlock(&bpm_lock);
        if (b->cur_tick % TICK_SIZE == 0) {
            loop_started = 1;
        }
    }

    while (1) {
        int note_played = 0;
        int rand_note_played = 0;
        for (int i = 0; i < mloop->size; i++) {
            if ( iteration > 4 ) iteration = 1;
            while (!note_played) {
                double rel_note1, rel_note2;
                related_notes(mloop->melody[i]->note, &rel_note1, &rel_note2);
                //double rel_note;
                if (b->quart_note_tick % 32 == mloop->melody[i]->tick) {
                    //if ((rand() % 100) > 5) {
                    //    if ((rand() % 100) > 95) {
                    //        //rel_note = rel_note1;
                    //        play_note(mloop->sig_num, rel_note1);
                    //    } else {
                    play_note(mloop->sig_num, mloop->melody[i]->freq);
                    //    }
                    //}
                    note_played = 1;
                    //play_note(mloop->sig_num, mloop->melody[i]->freq);
                    //note_played = 1;
                }
                //else if (!rand_note_played) {
                //    rand_note_played = 1;
                //    if ((rand() % 100) > 90) {
                //        if ((rand() % 2) == 1)
                //            rel_note = rel_note1;
                //        if ((rand() % 10) == 1)
                //            rel_note *= 3;
                //        else
                //            rel_note = rel_note2;
                //        play_note(mloop->sig_num, rel_note);
                //    }
                //}

                // printf("WAITING\n");
                pthread_mutex_lock(&bpm_lock);
                pthread_cond_wait(&bpm_cond, &bpm_lock);
                pthread_mutex_unlock(&bpm_lock);
                // keypress_off(mixr->sound_generators[mloop->sig_num]);
                // printf("MLOOPTICK %d\n", b->quart_note_tick);
            }
            note_played = 0;
            rand_note_played = 0;
        }
    }
    // TODO free all this memory!!
    return NULL;
}

void keys_start_melody_player(int sig_num, char *pattern)
{

    melody_loop *mloop = new_melody_loop(sig_num);

    printf("KEYS START MELODY!\n");
    printf("SIG NUM %d - %s\n", sig_num, pattern);
    char *tok, *last_s;
    char *sep = " ";
    for (tok = strtok_r(pattern, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s)) {
        printf("TOKEY! %s\n", tok);
        int tick;
        char ch_freq[4];
        sscanf(tok, "%d:%s", &tick, ch_freq);
        ch_freq[3] = '\0';
        printf("[%d] - %s\n", tick, ch_freq);
        double freq = freqval(ch_freq);
        if (freq != -1) {
            melody_event *me = make_melody_event(tick, freq, ch_freq);
            add_melody_event(mloop, me);
        }
    }
    pthread_t melody_looprrr;
    if (pthread_create(&melody_looprrr, NULL, play_melody_loop, mloop)) {
        fprintf(stderr, "Err running loop\n");
    }
    pthread_detach(melody_looprrr);
}
