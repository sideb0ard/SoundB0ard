#include <stdio.h>
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

melody_loop mloop;

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
    int recording_started = 0;

    struct pollfd fdz[1];
    int pollret;

    fdz[0].fd = STDIN_FILENO;
    fdz[0].events = POLLIN;

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
                        if ( pthread_create (&melody_looprrr, NULL, play_melody_loop, NULL)) {
                            fprintf(stderr, "Err running loop\n");
                        }
                        pthread_detach(melody_looprrr);
                        quit = 1;
                        break;
                    case 1:
                        printf("Recording Mode ON.\n");
                }
            }

            double freq = chfreqlookup(ch);
            if ( freq != -1 ) {
                if (recording && !recording_started) {
                    recording_started = 1;
                }
                play_note(freq, soundgen_num, recording);
            }
        }
    }
    tcsetattr(0, TCSANOW, &old_info);
}

//melody_event make_melody_event(double freq, int sg_num, int tick) {
//    melody_msg mm;
//    mm.freq = freq;
//    mm.sg_num = sg_num;
//
//    melody_event e;
//    e.tick = tick;
//    e.mm = mm;
//
//    return mm;
//}
    
void add_melody_event(melody_event e)
{
    printf("ADDING melody event..\n");
    if ( mloop.size < 10 ) {
        mloop.melody[mloop.size] = e;
        mloop.size++;
    }
}

void *play_melody_loop(void *m)
{
    (void) m;

    printf("PLAY melody starting..\n");

    int mnum = mloop.melody[0].tick;
    printf("Starting num is %d\n", mnum);
    melody_loop loop;
    loop.size = 0;

    for ( int i = 0; i < mloop.size; i++ ) {
        printf("EVENT: [%d] - %f\n", mloop.melody[i].tick, mloop.melody[i].mm.freq);
        melody_event me;
        me.tick = mloop.melody[i].tick - mnum;
        me.mm = mloop.melody[i].mm;
        loop.melody[i] = me;
        printf("LOOP size %d\n", loop.size);
        loop.size++;
    }

    for ( int i = 0; i < loop.size; i++) {
        printf("NEW EVENT: [%d] - %f\n", loop.melody[i].tick, loop.melody[i].mm.freq);
    }

    int final_len = loop.melody[loop.size-1].tick;
    int diff = final_len % TICK_SIZE;
    int diff2 = TICK_SIZE - diff;
    int final_totes = final_len + diff2;

    while (1) {
        for ( int i = 0; i < loop.size; i++ ) {
            if ( b->cur_tick % final_totes == loop.melody[i].tick )
                play_note(loop.melody[i].mm.freq, loop.melody[i].mm.sg_num, 0);
        }

        //printf("TICKY! %d\n", b->cur_tick);
        pthread_mutex_lock(&bpm_lock);
        pthread_cond_wait(&bpm_cond,&bpm_lock);
        pthread_mutex_unlock(&bpm_lock);
    }
    return NULL;
}

void play_note(double freq, int sg_num, int recording) {

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;
    double vol = 0;

    if ( recording ) {
        melody_msg mm;
        mm.freq = freq;
        mm.sg_num = sg_num;

        melody_event e;
        e.tick = b->cur_tick;
        e.mm = mm;
        printf("FREQ!(%d)[%f]\n", b->cur_tick, freq);
        add_melody_event(e);
    }

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