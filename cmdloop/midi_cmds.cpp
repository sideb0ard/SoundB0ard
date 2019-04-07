#include <stdlib.h>
#include <string.h>

#include <drumsynth.h>
#include <midimaaan.h>
#include <midi_cmds.h>
#include <mixer.h>

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
        if (is_synth(mixr->sound_generators[soundgen_num]))
        {
            mixr->midi_control_destination = MIDI_CONTROL_SYNTH_TYPE;
            mixr->active_midi_soundgen_num = soundgen_num;
        }
        else if (mixr->sound_generators[soundgen_num]->type == DRUMSYNTH_TYPE)
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
            sound_generator *sg_src =
                (sound_generator *)mixr->sound_generators[sg_src_num];
            sequence_engine *src_engine = get_sequence_engine(sg_src);

            sound_generator *sg_dst =
                (sound_generator *)mixr->sound_generators[sg_dst_num];
            sequence_engine *dst_engine = get_sequence_engine(sg_dst);

            if (sequence_engine_is_valid_pattern(src_engine,
                                                 sg_src_pattern_num) &&
                sequence_engine_is_valid_pattern(dst_engine,
                                                 sg_dst_pattern_num))
            {
                pattern_change_info change_info = {.clear_previous = true,
                                                   .temporary = false};
                midi_event *pattern =
                    sequence_engine_get_pattern(src_engine, sg_src_pattern_num);
                sequence_engine_set_pattern(dst_engine, sg_dst_pattern_num,
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
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            midi_event *pattern =
                sequence_engine_get_pattern(engine, pattern_num);
            midi_pattern_print(pattern);
        }
        else
        {
            sg_num = atoi(wurds[1]);
            printf("ELSE! sg_num:%d\n", sg_num);
            if (mixer_is_valid_soundgen_num(mixr, sg_num))
            {
                printf("TRUWE!\n");
                sound_generator *sg =
                    (sound_generator *)mixr->sound_generators[sg_num];
                sequence_engine *engine = get_sequence_engine(sg);
                int num_patterns = sequence_engine_get_num_patterns(engine);
                for (int i = 0; i < num_patterns; i++)
                {
                    printf("PATTERN NUM %d\n", i);
                    midi_event *pattern =
                        sequence_engine_get_pattern(engine, i);
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
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            midi_event *pattern =
                sequence_engine_get_pattern(engine, pattern_num);
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
            drumsynth *ds = new_drumsynth();
            int sg_num = add_sound_generator(mixr, (sound_generator *)ds);
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