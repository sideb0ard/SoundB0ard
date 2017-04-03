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
                ms->m_octave--;
                break;
            case 50:
                printf("Up an octave...\n");
                ms->m_octave++;
                break;
            case 114:
                ms->recording = 1 - ms->recording;
                printf("Toggling REC to %s\n",
                       ms->recording ? "true" : "false");
                break;
            case 122:
                printf("SAW3 mode\n");
                ms->m_voice_mode = 0;
                break;
            case 120:
                printf("SQR3 mode\n");
                ms->m_voice_mode = 1;
                break;
            case 99:
                printf("SAW2SQR mode\n");
                ms->m_voice_mode = 2;
                break;
            case 118:
                printf("TRI2SAW mode\n");
                ms->m_voice_mode = 3;
                break;
            case 98:
                printf("TRI2SQR mode\n");
                ms->m_voice_mode = 4;
                break;
            case 110:
                ms->m_lfo1_waveform = (++ms->m_lfo1_waveform) % MAX_LFO_OSC;
                printf("LFO! Mode Toggle: %d MaxLFO: %d\n", ms->m_lfo1_waveform,
                       MAX_LFO_OSC);
                break;
            case 109:
                mixer_toggle_key_mode(mixr);
                printf("Switching KEY mode -- %d\n",
                       mixr->m_key_controller_mode);
                break;
            default:
                // play note
                midi_num = ch_midi_lookup(ch, ms);
                int fake_velocity = 126; // TODO real velocity
                if (midi_num != -1) {
                    minisynth_handle_midi_note(ms, midi_num, fake_velocity);
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

    pthread_setname_np("Melody looper");
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
                    // TODO - this doesn't seem to have intended effect
                    // minisynth_reset_voices(ms);
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
