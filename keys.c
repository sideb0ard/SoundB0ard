#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "defjams.h"
#include "keys.h"
#include "midimaaan.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
extern pthread_cond_t midi_tick_cond;
extern pthread_mutex_t midi_tick_lock;

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

    minisynth *ms = (minisynth *)mixr->sound_generators[soundgen_num];

    int ch = 0;
    int quit = 0;

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
            printf("C %d\n", ch);
            int midi_num;
            switch (ch) {
            case 27:
            case 113:
                quit = 1;
                break;
            case 49:
                printf("Down an octave...\n");
                // change_octave(ns, DOWN);
                break;
            case 50:
                printf("Up an octave...\n");
                // change_octave(ns, UP);
                break;
            case 114:
                ms->recording = 1 - ms->recording;
                printf("Toggling REC to %s\n",
                       ms->recording ? "true" : "false");
                break;
            case 122:
                printf("SAW3 mode\n");
                ms->m_voice_mode = 0;
                // printf("Changing WAVE form of synth->osc1\n");
                // minisynth_change_osc_wave_form(ms, 0, 0, true);
                break;
            case 120:
                printf("SQR3 mode\n");
                ms->m_voice_mode = 1;
                // printf("Changing WAVE form of synth->osc2\n");
                // minisynth_change_osc_wave_form(ms, 0, 1, true);
                break;
            case 99:
                printf("SAW2SQR mode\n");
                ms->m_voice_mode = 2;
                // printf("Changing WAVE form of synth->lfo\n");
                // minisynth_change_osc_wave_form(ms, 0, 2, true);
                break;
            case 118:
                printf("TRI2SAW mode\n");
                ms->m_voice_mode = 3;
                // ms->m_filter_keytrack = 1 - ms->m_filter_keytrack;
                // printf("Key tracking toggle - val is %d!\n",
                //       ms->m_filter_keytrack);
                break;
            case 98:
                printf("TRI2SQR mode\n");
                ms->m_voice_mode = 4;
                // printf("Key tracking intensity toggle!\n");
                // if (ms->m_filter_keytrack_intensity == 0.5)
                //    ms->m_filter_keytrack_intensity = 2.0;
                // else if (ms->m_filter_keytrack_intensity == 2.0)
                //    ms->m_filter_keytrack_intensity = 1.0;
                // else
                //    ms->m_filter_keytrack_intensity = 0.5;
                // printf("Key tracking intensity toggle! Val is %f\n",
                //       ms->m_filter_keytrack_intensity);
                break;

            default:
                // play note
                midi_num = ch_midi_lookup(ch, ms);
                int fake_velocity = 126; // TODO real velocity
                if (midi_num != -1) {

                    print_midi_event(midi_num);
                    minisynth_midi_note_on(ms, midi_num, fake_velocity);

                    // TODO: refactor this out - this adds a temp note_off
                    int note_off_tick =
                        ((mixr->tick % PPNS) + PPS * 4) %
                        PPNS; // rough guess - PPS is pulses per quart note
                              // and PPNS is pulses per minisynth Loop
                    midi_event *ev = new_midi_event(note_off_tick, 128,
                                                    midi_num, fake_velocity);
                    ev->delete_after_use = true; // _THIS_ is the magic
                    ms->melodies[ms->cur_melody][note_off_tick] = ev;
                    ////////////////////////

                    if (ms->recording) {
                        printf("Recording note!\n");
                        int note_on_tick = mixr->tick % PPNS;
                        midi_event *ev = new_midi_event(
                            note_on_tick, 144, midi_num, fake_velocity);
                        ms->melodies[ms->cur_melody][note_on_tick] = ev;

                        int note_off_tick =
                            (note_on_tick + PPS * 4) %
                            PPNS; // rough guess - PPS is pulses per quart note
                                  // and PPNS is pulses per minisynth Loop
                        midi_event *ev2 = new_midi_event(
                            note_off_tick, 128, midi_num, fake_velocity);
                        ms->melodies[ms->cur_melody][note_off_tick] = ev2;
                    }
                }
                printf("CCCC %d\n", ch);
            }
            minisynth_update(ms);
        }
    }
    tcsetattr(0, TCSANOW, &old_info);
}

void *play_melody_loop(void *p)
{
    minisynth *ms = (minisynth *)p;

    printf("PLAY melody starting..\n");

    while (1) {
        pthread_mutex_lock(&midi_tick_lock);
        pthread_cond_wait(&midi_tick_cond, &midi_tick_lock);
        pthread_mutex_unlock(&midi_tick_lock);

        int idx = mixr->tick % PPNS;

        // top of the loop, check if we need to progress to next loop
        if (idx == 0) {
            if (ms->multi_melody_mode) {
                ms->cur_melody_iteration--;
                if (ms->cur_melody_iteration == 0) {
                    ms->cur_melody = (ms->cur_melody + 1) % ms->num_melodies;
                    ms->cur_melody_iteration =
                        ms->melody_multiloop_count[ms->cur_melody];
                }
            }
        }

        if (ms->melodies[ms->cur_melody][idx] != NULL) {
            midi_event *ev = ms->melodies[ms->cur_melody][idx];
            midi_parse_midi_event(ms, ev);
        }
    }

    return NULL;
}
