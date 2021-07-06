#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "defjams.h"
#include "dxsynth.h"
#include "keys.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "utils.h"

extern Mixer *mixr;
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

    auto sg = mixr->sound_generators_[soundgen_num];

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
            unsigned int midi_num;
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
            // mixer_toggle_key_mode(mixr);
            // printf("Switching KEY mode -- %d\n",
            //       mixr->m_key_controller_mode);
            // break;
            case 91:
                sg->randomize();
                break;
            default:
                // play note
                midi_num = input_key_to_char_note(ch, s_keys_octave, textnote);
                printf("MIDI: %s%d [%d]\n", textnote, s_keys_octave, midi_num);
                unsigned int fake_velocity = 128; // TODO real velocity
                if (midi_num >= 0)
                {
                    sg->noteOn({.event_type = MIDI_ON,
                                .data1 = midi_num,
                                .data2 = fake_velocity});
                }
                else
                    printf("check yer MIDI notes! that ain't valid shit: %d\n",
                           midi_num);
            }
        }
    }
    tcsetattr(0, TCSANOW, &old_info);
}
