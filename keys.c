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

    int ch = 0;
    int quit = 0;

    int recording = 0;
    int pattern_loop[32] = {0};
    // int recording_started = 0;

    struct pollfd fdz[1];
    int pollret;

    fdz[0].fd = STDIN_FILENO;
    fdz[0].events = POLLIN;

    while (!quit) {

        pollret = poll(fdz, 1, 0);
        if (pollret == -1) {
            perror("poll");
            return;
        }

        if (fdz[0].revents & POLLIN) {
            ch = getchar();
            switch (ch) {
            case 27:
            case 113:
                quit = 1;
                break;
            case 49:
                printf("Down an octave...\n");
                change_octave(ns, DOWN);
                break;
            case 50:
                printf("Up an octave...\n");
                change_octave(ns, UP);
                break;
            case 114:
                printf("Now recording.\n");
                recording = 1 - recording;
                if ( !recording ) // i.e. must have just stopped
                {
                    melody_loop *mloop = new_melody_loop();
                    for (int i = 0; i < 32; i++ ) {
                        if ( pattern_loop[i] != 0 ) {
                            printf("creating pattern event: [%d] : %d\n", i, pattern_loop[i]);
                            melody_event *me = make_melody_event(i, pattern_loop[i]);
                            add_melody_event(mloop, me);
                        }
                    }
                    nanosynth_add_melody_loop(ns, mloop);

                    pthread_t melody_looprrr;
                    if (pthread_create(&melody_looprrr, NULL, play_melody_loop, ns)) {
                        fprintf(stderr, "Err running loop\n");
                    }
                    pthread_detach(melody_looprrr);
                }
                break;
            case 122:
                printf("Changing WAVE form of synth->osc1\n");
                nanosynth_change_osc_wave_form(ns, 0);
                break;
            case 120:
                printf("Changing WAVE form of synth->osc2\n");
                nanosynth_change_osc_wave_form(ns, 1);
                break;
            case 99:
                printf("Changing WAVE form of synth->lfo\n");
                nanosynth_change_osc_wave_form(ns, 2);
                break;
            default: // play note
                printf("default!\n");
                int midi_num = ch_midi_lookup(ch, ns);
                if (midi_num != -1) {
                    print_midi_event(midi_num);
                    note_on(ns, midi_num);
                    if ( recording ) {
                        printf("noted.");
                        pattern_loop[b->quart_note_tick % 32] = midi_num;
                    }
                }
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
        int midi_num;
        sscanf(tok, "%d:%d", &tick, &midi_num);

        // i need to get midi number for ch_freq^
        // int midi_num = notelookup(note) + 12*octave;
        // double freq = freqval(ch_freq);
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
