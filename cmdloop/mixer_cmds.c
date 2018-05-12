#include <stdlib.h>
#include <string.h>

#include <fx_cmds.h>
#include <looper_cmds.h>
#include <mixer.h>
#include <mixer_cmds.h>
#include <new_item_cmds.h>
#include <obliquestrategies.h>
#include <pattern_transformers.h>
#include <stepper_cmds.h>
#include <synth_cmds.h>
#include <utils.h>

extern mixer *mixr;
extern char *key_names[NUM_KEYS];

extern wtable *wave_tables[5];

bool parse_mixer_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{

    if (strncmp("bpm", wurds[0], 3) == 0)
    {
        int bpm = atoi(wurds[1]);
        if (bpm > 0)
            mixer_update_bpm(mixr, bpm);
        return true;
    }

    else if (strncmp("every", wurds[0], 4) == 0)
    {
        algorithm *a = new_algorithm(num_wurds, wurds);
        mixer_add_algorithm(mixr, a);
    }

    else if (strncmp("brak", wurds[0], 4) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            soundgenerator *sg =
                (soundgenerator *)mixr->sound_generators[sg_num];
            if (sg->is_valid_pattern(sg, sg_pattern_num))
            {
                midi_event *pattern = sg->get_pattern(sg, sg_pattern_num);
                brak(pattern);
            }
        }
        return true;
    }
    else if (strncmp("cp", wurds[0], 2) == 0)
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
    else if (strncmp("quantize", wurds[0], 8) == 0)
    {
        int quanta = atoi(wurds[1]);
        switch (quanta)
        {
        case (32):
            mixr->quantize = Q32;
        case (16):
            mixr->quantize = Q16;
        case (8):
            mixr->quantize = Q8;
        case (4):
            mixr->quantize = Q4;
        default:
            printf("nae danger, mate, quantize yer heid..\n");
        }
        return true;
    }

    else if (strncmp("compat", wurds[0], 6) == 0)
    {
        if (strncmp("keys", wurds[1], 4) == 0)
        {
            mixer_print_compat_keys(mixr);
        }
        return true;
    }

    else if (strncmp("keys", wurds[0], 4) == 0)
    {
        for (int i = 0; i < NUM_KEYS; i++)
        {
            char *key = key_names[i];
            printf("%d [%s] ", i, key);
        }
        printf("\n");
        return true;
    }
    else if (strncmp("key", wurds[0], 3) == 0)
    {
        int key = atoi(wurds[1]);
        if (key >= 0 && key < NUM_KEYS)
        {
            printf("Changing KEY!\n");
            mixr->key = key;
        }
        return true;
    }
    else if (strncmp("<~", wurds[0], 2) == 0 || strncmp("~>", wurds[0], 2) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            soundgenerator *sg =
                (soundgenerator *)mixr->sound_generators[sg_num];
            if (sg->is_valid_pattern(sg, sg_pattern_num))
            {
                int places_to_shift = atoi(wurds[2]);
                midi_event *pattern = sg->get_pattern(sg, sg_pattern_num);
                if (strcmp("<~", wurds[0]) == 0)
                    left_shift(pattern, places_to_shift);
                else
                    right_shift(pattern, places_to_shift);
            }
        }
        return true;
    }
    else if (strncmp("preview", wurds[0], 7) == 0)
    {
        if (is_valid_file(wurds[1]))
        {
            mixer_preview_track(mixr, wurds[1]);
        }
        return true;
    }

    else if (strncmp("debug", wurds[0], 5) == 0)
    {
        if (strncmp("on", wurds[1], 2) == 0 ||
            strncmp("true", wurds[1], 4) == 0)
        {
            printf("Enabling debug mode\n");
            mixr->debug_mode = true;
        }
        else if (strncmp("off", wurds[1], 2) == 0 ||
                 strncmp("false", wurds[1], 5) == 0)
        {
            printf("Disabling debug mode\n");
            mixr->debug_mode = false;
        }
        return true;
    }

    else if (strncmp("ps", wurds[0], 2) == 0)
    {
        printf("CMD PS\n");
        mixer_ps(mixr);
        return true;
    }

    else if (strncmp("quiet", wurds[0], 5) == 0 ||
             strncmp("hush", wurds[0], 4) == 0)
    {
        for (int i = 0; i < mixr->soundgen_num; i++)
            mixr->sound_generators[i]->setvol(mixr->sound_generators[i], 0.0);
        return true;
    }
    else if (strncmp("ls", wurds[0], 2) == 0)
    {
        list_sample_dir(wurds[1]);
        return true;
    }

    else if (strncmp("rm", wurds[0], 3) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            printf("Deleting SOUND GEN %d\n", soundgen_num);
            mixer_del_soundgen(mixr, soundgen_num);
        }
        return true;
    }
    else if (strncmp("start", wurds[0], 5) == 0)
    {
        if (strncmp(wurds[1], "all", 3) == 0)
        {
            for (int i = 0; i < mixr->soundgen_num; i++)
            {
                soundgenerator *sg = mixr->sound_generators[i];
                if (sg != NULL)
                    sg->start(sg);
            }
        }
        else
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                printf("Starting SOUND GEN %d\n", soundgen_num);
                soundgenerator *sg = mixr->sound_generators[soundgen_num];
                sg->start(sg);
            }
        }
        return true;
    }
    else if (strncmp("stop", wurds[0], 5) == 0)
    {
        if (strncmp(wurds[1], "all", 3) == 0)
        {
            for (int i = 0; i < mixr->soundgen_num; i++)
            {
                soundgenerator *sg = mixr->sound_generators[i];
                if (sg != NULL)
                    sg->stop(sg);
            }
        }
        else
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                printf("Stopping SOUND GEN %d\n", soundgen_num);
                soundgenerator *sg = mixr->sound_generators[soundgen_num];
                sg->stop(sg);
            }
        }
        return true;
    }
    else if (strncmp("down", wurds[0], 4) == 0 ||
             strncmp("up", wurds[0], 3) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            SBMSG *msg = new_sbmsg();
            msg->sound_gen_num = soundgen_num;
            if (strcmp("up", wurds[0]) == 0)
            {
                strncpy(msg->cmd, "fadeuprrr", 19);
            }
            else
            {
                strncpy(msg->cmd, "fadedownrrr", 19);
            }
            thrunner(msg);
        }
        return true;
    }

    else if (strncmp("vol", wurds[0], 3) == 0)
    {
        if (strncmp("mixer", wurds[1], 5) == 0)
        {
            double vol = atof(wurds[2]);
            mixer_vol_change(mixr, vol);
        }
        else
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                double vol = atof(wurds[2]);
                vol_change(mixr, soundgen_num, vol);
            }
        }
        return true;
    }

    else if (strncmp("scene", wurds[0], 5) == 0)
    {
        if (strncmp("new", wurds[1], 4) == 0)
        {
            int num_bars = atoi(wurds[2]);
            if (num_bars == 0)
                num_bars = 4; // default
            int new_scene_num = mixer_add_scene(mixr, num_bars);
            printf("New scene %d with %d bars!\n", new_scene_num, num_bars);
        }
        else if (strncmp("mode", wurds[1], 4) == 0)
        {
            if (strncmp("on", wurds[2], 2) == 0)
            {
                mixr->scene_mode = true;
                mixr->scene_start_pending = true;
            }
            else if (strncmp("off", wurds[2], 3) == 0)
            {
                mixr->scene_mode = false;
                mixr->scene_start_pending = false;
            }
            else
                mixr->scene_mode = 1 - mixr->scene_mode;
            printf("Mode scene! %s\n", mixr->scene_mode ? "true" : "false");
        }
        else
        {
            int scene_num = atoi(wurds[1]);
            if (mixer_is_valid_scene_num(mixr, scene_num))
            {
                printf("Changing scene %d\n", scene_num);
                if (strncmp("add", wurds[2], 3) == 0)
                {
                    int sg_num = 0;
                    int sg_track_num = 0;
                    sscanf(wurds[3], "%d:%d", &sg_num, &sg_track_num);
                    if (mixer_is_valid_soundgen_track_num(mixr, sg_num,
                                                          sg_track_num))
                    {
                        printf("Adding sg %d %d\n", sg_num, sg_track_num);
                        mixer_add_soundgen_track_to_scene(mixr, scene_num,
                                                          sg_num, sg_track_num);
                    }
                    else
                        printf("WHUT? INVALID?!\n");
                }
                else if (strncmp("cp", wurds[2], 2) == 0)
                {
                    int scene_num2 = atoi(wurds[3]);
                    if (mixer_is_valid_scene_num(mixr, scene_num2))
                    {
                        printf("Copying scene %d "
                               "to %d\n",
                               scene_num, scene_num2);
                        mixer_cp_scene(mixr, scene_num, scene_num2);
                    }
                    else
                    {
                        printf("Not copying scene %d "
                               "-- %d is not a valid "
                               "destination\n",
                               scene_num, scene_num2);
                    }
                }
                else if (strncmp("dupe", wurds[2], 4) == 0)
                {
                    printf("Duplicating scene %d\n", scene_num);
                    int default_num_bars = 4;
                    int new_scene_num = mixer_add_scene(mixr, default_num_bars);
                    mixer_cp_scene(mixr, scene_num, new_scene_num);
                }
                else if (strncmp("rm", wurds[2], 2) == 0)
                {
                    int sg_num = 0;
                    int sg_track_num = 0;
                    sscanf(wurds[3], "%d:%d", &sg_num, &sg_track_num);
                    if (mixer_is_valid_soundgen_track_num(mixr, sg_num,
                                                          sg_track_num))
                    {
                        printf("Removing sg %d %d\n", sg_num, sg_track_num);
                        mixer_rm_soundgen_track_from_scene(
                            mixr, scene_num, sg_num, sg_track_num);
                    }
                }
                else
                {
                    printf("Queueing scene %d\n", scene_num);
                    mixr->scene_start_pending = true;
                    mixr->current_scene = scene_num;
                }
            }
        }
        return true;
    }
    else if (strncmp("midi", wurds[0], 4) == 0)
    {
        printf("MIDI Time!\n");
        // int sgnum = add_minisynth(mixr);
        // mixr->midi_control_destination = SYNTH;
        // mixr->active_midi_soundgen_num = sgnum;
        if (strncmp("init", wurds[1], 4) == 0)
        {
            if (!mixr->have_midi_controller)
            {
                printf("Initializing MIDI\n");
                //// run the MIDI event looprrr...
                pthread_t midi_th;
                if (pthread_create(&midi_th, NULL, midiman, NULL))
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
    // PROGRAMMING CMDS
    else if (strncmp("var", wurds[0], 3) == 0 && strncmp("=", wurds[2], 1) == 0)
    {
        printf("Oooh! %s = %s\n", wurds[1], wurds[3]);
        update_environment(wurds[1], atoi(wurds[3]));
        return true;
    }

    // UTILS
    else if (strncmp("chord", wurds[0], 6) == 0)
    {
        chordie(wurds[1]);
        return true;
    }

    else if (strncmp("strategy", wurds[0], 8) == 0)
    {
        oblique_strategy();
        return true;
    }

    return false;
}
