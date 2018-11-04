#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "defjams.h"
#include "digisynth.h"
#include "dxsynth.h"
#include "keys.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
static int s_keys_octave = 2;

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

    sequence_engine *engine = get_sequence_engine(mixr->sound_generators[soundgen_num]);

    int ch = 0;
    int quit = 0;

    struct pollfd fdz[1];
    int pollret;

    fdz[0].fd = STDIN_FILENO;
    fdz[0].events = POLLIN;

    while (!quit)
    {

        pollret = poll(fdz, 1, 0);
        if (pollret == -1)
        {
            perror("poll");
            return;
        }

        if (fdz[0].revents & POLLIN)
        {
            ch = getchar();
            // printf("C %d\n", ch);
            int midi_num;
            char textnote[4] = {0};
            switch (ch)
            {
            case 27:
            case 113:
                quit = 1;
                break;
            case 49:
                printf("Down an octave...\n");
                s_keys_octave--;
                break;
            case 50:
                printf("Up an octave...\n");
                s_keys_octave++;
                break;
            case 114:
                engine->recording = 1 - engine->recording;
                printf("Toggling REC to %s\n",
                       engine->recording ? "true" : "false");
                break;
            // mixer_toggle_key_mode(mixr);
            // printf("Switching KEY mode -- %d\n",
            //       mixr->m_key_controller_mode);
            // break;
            case 91:
                if (mixr->sound_generators[soundgen_num]->type ==
                    MINISYNTH_TYPE)
                {
                    printf("RANDOM MONDY mode!\n");
                    minisynth *ms =
                        (minisynth *)mixr->sound_generators[soundgen_num];
                    minisynth_rand_settings(ms);
                }
                else if (mixr->sound_generators[soundgen_num]->type ==
                         DIGISYNTH_TYPE)
                {
                    digisynth *ds =
                        (digisynth *)mixr->sound_generators[soundgen_num];
                    printf("RANDOM MONDY mode NAE DIGI YET!\n");
                    (void)ds;
                }
                else if (mixr->sound_generators[soundgen_num]->type ==
                         DXSYNTH_TYPE)
                {
                    dxsynth *dx =
                        (dxsynth *)mixr->sound_generators[soundgen_num];
                    printf("RANDOM MONDY DX!\n");
                    dxsynth_rand_settings(dx);
                }
                break;
            default:
                // play note
                midi_num = input_key_to_char_note(ch, s_keys_octave, textnote);
                printf("MIDI: %s%d [%d]\n", textnote, s_keys_octave, midi_num);
                int fake_velocity = 128; // TODO real velocity
                if (midi_num >= 0)
                {
                    synth_handle_midi_note(mixr->sound_generators[soundgen_num],
                                           midi_num, fake_velocity, true);
                }
                else
                    printf("check yer MIDI notes! that ain't valid shit: %d\n",
                           midi_num);
            }
        }
    }
    tcsetattr(0, TCSANOW, &old_info);
}
