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

melody_loop *new_melody_loop()
{
    melody_loop *l;
    l = (melody_loop *)calloc(1, sizeof(melody_loop));
    return l;
}

// static void alarm_handler(int signum)
//{
//    //printf("TIMER!! %d\n", signum);
//    nanosynth *ns = (nanosynth *)mixr->sound_generators[0];
//    eg_release(ns->eg1);
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

    nanosynth *ns = (nanosynth *)mixr->sound_generators[soundgen_num];

    // signal(SIGALRM, alarm_handler);

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
            else if (ch == 49) { // '1'
                printf("Down an octave...\n");
                change_octave(ns, DOWN);
            }
            else if (ch == 50) { // '2'
                printf("Up an octave...\n");
                change_octave(ns, UP);
            }
            // else if (ch == 99) { // 'c'
            //    freqinc(self->lfo, DOWN);
            //}
            // else if (ch == 67) { // 'C'
            //    freqinc(self->lfo, UP);
            //}
            // else if (ch == 122) { // 'z'
            //    filter_adj_fc_control(self->filter->bc_filter, DOWN);
            //}
            // else if (ch == 90) { // 'Z'
            //    filter_adj_fc_control(self->filter->bc_filter, UP);
            //}
             else if (ch == 122) { // 'z'
                printf("Changing WAVE form of synth->osc1\n");
                nanosynth_change_osc_wave_form(ns, 0);
            }
             else if (ch == 120) { // 'x'
                printf("Changing WAVE form of synth->osc2\n");
                nanosynth_change_osc_wave_form(ns, 1);
            }
             else if (ch == 99) { // 'c'
                printf("Changing WAVE form of synth->lfo\n");
                nanosynth_change_osc_wave_form(ns, 2);
            }

            else { // try to play note
                int midi_num = ch_midi_lookup(ch, ns);
                if (midi_num != -1) {
                note_on(ns, midi_num);
                }
                // alarm(2);
            }
        }
    }
    tcsetattr(0, TCSANOW, &old_info);
}

// melody_event* make_melody_event(double freq, int sg_num, int tick)
melody_event *make_melody_event(int tick, unsigned midi_num)
{
    melody_event *me;
    me = (melody_event *)calloc(1, sizeof(melody_event));
    me->tick = tick;
    me->midi_num = midi_num;

    return me;
}

void add_melody_event(melody_loop *mloop, melody_event *e)
{
    printf("ADDING melody event..\n");
    if (mloop->size < 100) {
        mloop->melody[mloop->size++] = e;
    }
}

void *play_melody_loop(void *p)
{
    // melody_loop *mloop = (melody_loop *)m;
    nanosynth *ns = (nanosynth *)p;

    int notes_played_time[32];
    for (int i = 0; i < 32; i++)
        notes_played_time[i] = 0;

    printf("PLAY melody starting..\n");

    int loop_started = 0;
    while (!loop_started) {
        pthread_mutex_lock(&bpm_lock);
        pthread_cond_wait(&bpm_cond, &bpm_lock);
        pthread_mutex_unlock(&bpm_lock);
        if (b->cur_tick % TICK_SIZE == 0) {
            loop_started = 1;
        }
    }

    // nanosynth *ns = (nanosynth *)mixr->sound_generators[mloop->sig_num];

    while (1) {
        for (int j = 0; j < ns->melody_loop_num; j++) {
            melody_loop *mloop = ns->mloops[j];
            int note_played = 0;
            for (int i = 0; i < mloop->size; i++) {
                while (!note_played) {
                    if (b->quart_note_tick % 32 == mloop->melody[i]->tick) {
                        // printf("playing %f\n", mloop->melody[i]->freq);
                        note_on(ns, mloop->melody[i]->midi_num);
                        note_played = 1;
                        if (ns->sustain > 0) // switched on
                            notes_played_time[mloop->melody[i]->tick] = 1;
                    }

                    pthread_mutex_lock(&bpm_lock);
                    pthread_cond_wait(&bpm_cond, &bpm_lock);
                    pthread_mutex_unlock(&bpm_lock);
                    for (int i = 0; i < 32; i++) {
                        if (notes_played_time[i] > 0) {
                            notes_played_time[i]++;
                            // printf("notes played time %d =  %d\n", i,
                            // notes_played_time[i]);
                        }
                        if (notes_played_time[i] > ns->sustain) {
                            notes_played_time[i] = 0;
                            eg_note_off(ns->eg1);
                        }
                    }
                }
                note_played = 0;
                // nanosynth *ns =
                // (nanosynth*)mixr->sound_generators[mloop->sig_num];
                // printf("note off!\n");
                // note_off(ns->env);
            }
        }
    }
    // TODO free all this memory!!
    return NULL;
}

melody_loop *mloop_from_pattern(char *pattern)
{
    melody_loop *mloop = new_melody_loop();

    char *tok, *last_s;
    char *sep = " ";
    for (tok = strtok_r(pattern, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s)) {
        int tick;
        int octave;
        char note[3];
        sscanf(tok, "%d:%d:%s", &tick, &octave, note);
        note[2] = '\0';

        //i need to get midi number for ch_freq^
        int midi_num = notelookup(note) + 12*octave;
        //double freq = freqval(ch_freq);
        if (midi_num != -1) {
            melody_event *me = make_melody_event(tick, midi_num);
            add_melody_event(mloop, me);
        }
    }
    return mloop;
}

void keys_start_melody_player(int sig_num, char *pattern)
{

    // melody_loop *mloop = new_melody_loop(sig_num);
    melody_loop *mloop = mloop_from_pattern(pattern);

    printf("KEYS START MELODY!\n");
    printf("SIG NUM %d - %s\n", sig_num, pattern);

    nanosynth *ns = (nanosynth *)mixr->sound_generators[sig_num];
    nanosynth_add_melody_loop(ns, mloop);

    pthread_t melody_looprrr;
    if (pthread_create(&melody_looprrr, NULL, play_melody_loop, ns)) {
        fprintf(stderr, "Err running loop\n");
    }
    pthread_detach(melody_looprrr);
}
