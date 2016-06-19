#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <time.h>

#include "defjams.h"
#include "keys.h"
#include "utils.h"
#include "mixer.h"

extern mixer *mixr;

void keys(int soundgen_num)
{
    printf("PLAYING keys for %d\n", soundgen_num);
    struct termios new_info, old_info;
    tcgetattr(0, &old_info);
    new_info = old_info;

    new_info.c_lflag &= (~ICANON & ~ECHO);
    new_info.c_cc[VMIN] = 1;
    new_info.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_info);

    int ch = 0;
    int quit = 0;

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
            if ( ch == 27 || ch == 113 )
                quit = 1;
            double freq = chfreqlookup(ch);
            if ( freq != -1 ) {
                play_note(freq, soundgen_num);
            }
        }
    }
    tcsetattr(0, TCSANOW, &old_info);
}

void play_note(double freq, int sg_num) {

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;
    double vol = 0;

    printf("SETTING FREQ TO %f\n", freq);
    mfm(mixr->sound_generators[sg_num], "car", freq);
    
    printf("VOL UP TO SOUNDGEN %d\n", sg_num);
    while (vol < 0.6) {
      vol += 0.00001;
      mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], vol);
    }
    mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], 0.6);
    
    printf("PAUSE\n");
    nanosleep(&ts, NULL);
    
    printf("VOL DOWN\n");
    while (vol > 0.0) {
      vol -= 0.00001;
      mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], vol);
    }
    mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], 0.0);

}
