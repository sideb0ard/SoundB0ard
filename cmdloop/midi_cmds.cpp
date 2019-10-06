#include <stdlib.h>
#include <string.h>

#include <drumsynth.h>
#include <midi_cmds.h>
#include <midimaaan.h>

extern mixer *mixr;

void midi_launch_init(mixer *mixr)
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
}

void midi_set_destination(mixer *mixr, int soundgen_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
    {
        if (is_synth(mixr->SoundGenerators[soundgen_num]))
        {
            mixr->midi_control_destination = MIDI_CONTROL_SYNTH_TYPE;
            mixr->active_midi_soundgen_num = soundgen_num;
        }
        else if (mixr->SoundGenerators[soundgen_num]->type == DRUMSYNTH_TYPE)
        {
            mixr->midi_control_destination = MIDI_CONTROL_DRUMSYNTH_TYPE;
            mixr->active_midi_soundgen_num = soundgen_num;
        }
    }
}

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
            SoundGenerator *sg_src =
                (SoundGenerator *)mixr->SoundGenerators[sg_src_num];

            SoundGenerator *sg_dst =
                (SoundGenerator *)mixr->SoundGenerators[sg_dst_num];

            if (sequence_engine_is_valid_pattern(&sg_src->engine,
                                                 sg_src_pattern_num) &&
                sequence_engine_is_valid_pattern(&sg_dst->engine,
                                                 sg_dst_pattern_num))
            {
                pattern_change_info change_info = {.clear_previous = true,
                                                   .temporary = false};
                midi_event *pattern = sequence_engine_get_pattern(
                    &sg_src->engine, sg_src_pattern_num);
                sequence_engine_set_pattern(&sg_dst->engine, sg_dst_pattern_num,
                                            change_info, pattern);
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
            SoundGenerator *sg =
                (SoundGenerator *)mixr->SoundGenerators[sg_num];
            midi_event *pattern =
                sequence_engine_get_pattern(&sg->engine, pattern_num);
            midi_pattern_print(pattern);
        }
        else
        {
            sg_num = atoi(wurds[1]);
            printf("ELSE! sg_num:%d\n", sg_num);
            if (mixer_is_valid_soundgen_num(mixr, sg_num))
            {
                printf("TRUWE!\n");
                SoundGenerator *sg =
                    (SoundGenerator *)mixr->SoundGenerators[sg_num];
                int num_patterns =
                    sequence_engine_get_num_patterns(&sg->engine);
                for (int i = 0; i < num_patterns; i++)
                {
                    printf("PATTERN NUM %d\n", i);
                    midi_event *pattern =
                        sequence_engine_get_pattern(&sg->engine, i);
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
            SoundGenerator *sg =
                (SoundGenerator *)mixr->SoundGenerators[sg_num];
            midi_event *pattern =
                sequence_engine_get_pattern(&sg->engine, pattern_num);
            midi_pattern_set_velocity(pattern, midi_tick, velocity);
        }

        return true;
    }
    else if (strncmp("midi_print", wurds[0], 10) == 0)
    {
        bool b = atoi(wurds[1]);
        mixer_enable_print_midi(mixr, b);
    }
    else if (strncmp("midi_bank", wurds[0], 10) == 0)
    {
        int b = atoi(wurds[1]);
        mixer_set_midi_bank(mixr, b);
    }
    else if (strncmp("midiii", wurds[0], 6) == 0)
    {
        midi_launch_init(mixr);
    }
    else if (strncmp("midi", wurds[0], 4) == 0)
    {
        printf("MIDI Time!\n");
        if (strncmp("init", wurds[1], 4) == 0)
            midi_launch_init(mixr);
        else if (strncmp("moog", wurds[1], 4) == 0)
        {
            midi_launch_init(mixr);
            int sg_num = add_minisynth(mixr);
            if (sg_num != -1)
                midi_set_destination(mixr, sg_num);
        }
        else if (strncmp("drum", wurds[1], 4) == 0)
        {
            midi_launch_init(mixr);
            drumsynth *ds = new drumsynth();
            int sg_num = add_sound_generator(mixr, (SoundGenerator *)ds);
            if (sg_num != -1)
                midi_set_destination(mixr, sg_num);
        }
        else if (strncmp("fm", wurds[1], 2) == 0 ||
                 strncmp("dx", wurds[1], 2) == 0)
        {
            midi_launch_init(mixr);
            int sg_num = add_dxsynth(mixr);
            if (sg_num != -1)
                midi_set_destination(mixr, sg_num);
        }
        else
        {
            int soundgen_num = atoi(wurds[1]);
            midi_set_destination(mixr, soundgen_num);
        }
        return true;
    }

    return false;
}
