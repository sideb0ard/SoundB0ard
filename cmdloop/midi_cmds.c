#include <stdlib.h>
#include <string.h>

#include <midi_cmds.h>
#include <mixer.h>

extern mixer *mixr;

bool parse_midi_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{

    if (strncmp("cp", wurds[0], 2) == 0)
    {
        int sg_src_num;
        int sg_src_pattern_num;
        int sg_dst_num;
        int sg_dst_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_src_num, &sg_src_pattern_num);
        sscanf(wurds[2], "%d:%d", &sg_dst_num, &sg_dst_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_src_num) &&
            mixer_is_valid_soundgen_num(mixr, sg_dst_num))
        {
            soundgenerator *sg_src =
                (soundgenerator *)mixr->sound_generators[sg_src_num];
            soundgenerator *sg_dst =
                (soundgenerator *)mixr->sound_generators[sg_dst_num];
            if (sg_src->is_valid_pattern(sg_src, sg_src_pattern_num) &&
                sg_dst->is_valid_pattern(sg_dst, sg_dst_pattern_num))
            {
                // printf("Copying from %d:%d to %d:%d\n", sg_src_num,
                //       sg_src_pattern_num, sg_dst_num, sg_dst_pattern_num);
                midi_event *pattern =
                    sg_src->get_pattern(sg_src, sg_src_pattern_num);
                sg_dst->set_pattern(sg_dst, sg_dst_pattern_num, pattern);
            }
            else
                printf("Tried copying - something went wrong..\n");
        }
        return true;
    }
    else if (strncmp("pprint", wurds[0], 6) == 0)
    {
        printf("PPRINT!\n");
        int sg_num = -1;
        int pattern_num = -1;
        sscanf(wurds[1], "%d:%d", &sg_num, &pattern_num);
        printf("SG_NUM! %d\n", sg_num);
        if (mixer_is_valid_soundgen_track_num(mixr, sg_num, pattern_num))
        {
            printf("SG:%d Pattern %d\n", sg_num, pattern_num);
            soundgenerator *sg =
                (soundgenerator *)mixr->sound_generators[sg_num];
            midi_event *pattern = sg->get_pattern(sg, pattern_num);
            midi_pattern_print(pattern);
        }
        else
        {
            sg_num = atoi(wurds[1]);
            printf("ELSE! sg_num:%d\n", sg_num);
            if (mixer_is_valid_soundgen_num(mixr, sg_num))
            {
                printf("TRUWE!\n");
                soundgenerator *sg =
                    (soundgenerator *)mixr->sound_generators[sg_num];
                int num_patterns = sg->get_num_patterns(sg);
                for (int i = 0; i < num_patterns; i++)
                {
                    printf("PATTERN NUM %d\n", i);
                    midi_event *pattern = sg->get_pattern(sg, i);
                    midi_pattern_print(pattern);
                }
            }
        }

        return true;
    }
    else if (strncmp("vel", wurds[0], 3) == 0)
    {
        printf("VEL!\n");
        int sg_num = -1;
        int pattern_num = -1;
        int midi_tick = -1;
        sscanf(wurds[1], "%d:%d:%d", &sg_num, &pattern_num, &midi_tick);
        int velocity = atoi(wurds[2]);
        if (mixer_is_valid_soundgen_track_num(mixr, sg_num, pattern_num))
        {
            printf("SG:%d Pattern %d Tick:%d Velocity:%d\n", sg_num,
                   pattern_num, midi_tick, velocity);
            soundgenerator *sg =
                (soundgenerator *)mixr->sound_generators[sg_num];
            midi_event *pattern = sg->get_pattern(sg, pattern_num);
            midi_pattern_set_velocity(pattern, midi_tick, velocity);
        }

        return true;
    }
    else if (strncmp("midi_print", wurds[0], 10) == 0)
    {
        bool b = atoi(wurds[1]);
        mixer_enable_print_midi(mixr, b);
    }
    else if (strncmp("midi", wurds[0], 4) == 0)
    {
        printf("MIDI Time!\n");
        if (strncmp("init", wurds[1], 4) == 0)
        {
            if (!mixr->have_midi_controller)
            {
                pthread_t midi_th;
                if (pthread_create(&midi_th, NULL, midi_init, NULL))
                {
                    fprintf(stderr, "Errrr, wit tha midi..\n");
                }
                pthread_detach(midi_th);
            }
            else
                printf("Already initialized\n");
        }
        else
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                is_synth(mixr->sound_generators[soundgen_num]))
            {
                mixr->midi_control_destination = SYNTH;
                mixr->active_midi_soundgen_num = soundgen_num;
            }
        }
        return true;
    }

    return false;
}
