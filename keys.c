#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

#include "bpmrrr.h"
#include "defjams.h"
#include "keys.h"
#include "utils.h"
#include "mixer.h"

extern mixer *mixr;
extern bpmrrr *b;

extern pthread_cond_t bpm_cond;
extern pthread_mutex_t bpm_lock;

//// TODO - make this instance variable - not global
//melody_loop mloop;

melody_loop* new_melody_loop(int soundgen_num)
{
    melody_loop* l;
    l = (melody_loop*) calloc(1, sizeof(melody_loop));
    l->sig_num = soundgen_num;
    return l;
}

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

    int ch = 0;
    int quit = 0;
    int recording = 0;
    //int recording_started = 0;

    struct pollfd fdz[1];
    int pollret;

    fdz[0].fd = STDIN_FILENO;
    fdz[0].events = POLLIN;

    melody_loop* mloop;

    while (!quit) {

        pollret = poll (fdz, 1, 0);
        if ( pollret == -1 ) {
            perror("poll");
            return;
        }

        if (fdz[0].revents & POLLIN) {
            ch = getchar();
            if ( ch == 27 || ch == 113 ) {
                quit = 1;
            } else if ( ch == 32 ) {
                recording = 1 - recording;
                switch (recording) {
                    case 0:
                        printf("Recording Mode OFF.\n");
                        pthread_t melody_looprrr;
                        if ( pthread_create (&melody_looprrr, NULL, play_melody_loop, mloop)) {
                            fprintf(stderr, "Err running loop\n");
                        }
                        pthread_detach(melody_looprrr);
                        quit = 1;
                        break;
                    case 1:
                        printf("Recording Mode ON.\n");
                        mloop = new_melody_loop(soundgen_num);
                        //melody_event* e = make_melody_event(0, 0, b->cur_tick);
                        melody_event* e = make_melody_event(b->cur_tick, 0);
                        add_melody_event(mloop, e);
                }
            }

            double freq = chfreqlookup(ch);
            if ( freq != -1 ) {
                //if (recording && !recording_started) {
                //    recording_started = 1;
                //}
                play_note(soundgen_num, freq);
            }
        }
    }
    tcsetattr(0, TCSANOW, &old_info);
}

//melody_event* make_melody_event(double freq, int sg_num, int tick)
melody_event* make_melody_event(int tick, double freq)
{
    melody_event* me;
    me = (melody_event*) calloc(1, sizeof(melody_event));
    me->tick = tick;

    me->freq = freq;

    return me;
}
    
void add_melody_event(melody_loop* mloop, melody_event* e)
{
    printf("ADDING melody event..\n");
    if ( mloop->size < 100 ) {
        mloop->melody[mloop->size++] = e;
    }
}

void play_note(int sg_num, double freq)
{

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;
    double vol = 0;

    //printf("FREQ!(%d)[%f]\n", b->cur_tick, freq);
    //if ( recording ) {

    //    melody_event* e = make_melody_event(b->cur_tick, freq);
    //    printf("RECCCCFREQ!(%d)[%f]\n", b->cur_tick, freq);
    //    add_melody_event(mloop, e);
    //}

    mfm(mixr->sound_generators[sg_num], "car", freq);
    
    while (vol < 0.6) {
      vol += 0.00001;
      mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], vol);
    }
    mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], 0.6);
    
    nanosleep(&ts, NULL);
    
    while (vol > 0.0) {
      vol -= 0.00001;
      mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], vol);
    }
    mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], 0.0);

}

void *play_melody_loop(void *m)
{
    melody_loop* mloop = (melody_loop*) m;

    printf("\nPLAY melody starting..\n");

    //int mnum = mloop->melody[0]->tick;
    //printf("Starting num is %d\n", mnum);
    //melody_loop loop;
    //loop.size = 0;

    //for ( int i = 0; i < mloop->size; i++ ) {
    //    printf("EVENT: [%d] - %f\n", mloop->melody[i]->tick, mloop->melody[i]->freq);
    //    melody_event* me = (melody_event*) calloc(1, sizeof(melody_event));
    //    me->tick = mloop->melody[i]->tick - mnum;
    //    me->mm = mloop->melody[i]->mm;
    //    loop.melody[i] = me;
    //    printf("LOOP size %d\n", loop.size);
    //    loop.size++;
    //}

    //for ( int i = 0; i < loop.size; i++) {
    //    printf("NEW EVENT: [%d] - %f\n", loop.melody[i]->tick, loop.melody[i]->mm.freq);
    //}

    //int final_len = loop.melody[loop.size-1]->tick;
    //int diff = final_len % TICK_SIZE;
    //int diff2 = TICK_SIZE - diff;
    //int final_totes = final_len + diff2;
    //printf("FINAL TOTES %d\n", final_totes);
    //printf("LOOP LEN in BARS %d\n", final_totes / TICK_SIZE);

    // detect the start of a new bar
    int loop_started = 0;
    while (!loop_started) {
        pthread_mutex_lock(&bpm_lock);
        pthread_cond_wait(&bpm_cond,&bpm_lock);
        pthread_mutex_unlock(&bpm_lock);
        if ( b->cur_tick % TICK_SIZE == 0 ) {
            loop_started = 1;
        }
    }

    printf("STARTED!!!!!\n\n");
    while (1) {
        //for ( int i = 0 ;; i = i +1 % (TICK_SIZE*8) ) {
        int note_played = 0;
        for ( int i = 0 ; i < mloop->size ; i++ ) {
            while (!note_played) {
                if ( b->cur_tick % 120 == mloop->melody[i]->tick ) {
                    play_note(mloop->sig_num, mloop->melody[i]->freq);
                    note_played = 1;
                }
                pthread_mutex_lock(&bpm_lock);
                pthread_cond_wait(&bpm_cond,&bpm_lock);
                pthread_mutex_unlock(&bpm_lock);
            }
            note_played = 0;
        }
    }
    // TODO free all this memory!!
    return NULL;
}

void keys_start_melody_player(int sig_num, char *pattern)
{
    melody_loop* mloop = new_melody_loop(sig_num);

    printf("KEYS START MELODY!\n");
    printf("SIG NUM %d - %s\n", sig_num, pattern);
    char *tok, *last_s;
    char *sep = " ";
    for (tok = strtok_r(pattern, sep, &last_s);
         tok;
         tok = strtok_r(NULL, sep, &last_s))
    {
        printf("TOKEY! %s\n", tok);
        int tick;
        char ch_freq[4];
        sscanf(tok, "%d:%s", &tick, ch_freq);
        ch_freq[3] = '\0';
        printf("[%d] - %s\n", tick, ch_freq);
        double freq = freqval(ch_freq);
        if ( freq != -1 ) {
            melody_event* me = make_melody_event(tick, freq);
            add_melody_event(mloop, me);
        }
    }
    pthread_t melody_looprrr;
    if ( pthread_create (&melody_looprrr, NULL, play_melody_loop, mloop)) {
        fprintf(stderr, "Err running loop\n");
    }
    pthread_detach(melody_looprrr);
}
