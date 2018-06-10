#include <stdlib.h>
#include <string.h>

#include <fx_cmds.h>
#include <looper_cmds.h>
#include <mixer.h>
#include <mixer_cmds.h>
#include <new_item_cmds.h>
#include <obliquestrategies.h>
#include <pattern_transformers.h>
#include <sequencer_utils.h>
#include <stepper_cmds.h>
#include <synth_cmds.h>
#include <utils.h>

extern mixer *mixr;
extern char *key_names[NUM_KEYS];

extern wtable *wave_tables[5];

bool parse_mixer_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{

    bool cmd_found = false;

    if (strncmp("bpm", wurds[0], 3) == 0)
    {
        int bpm = atoi(wurds[1]);
        if (bpm > 0)
            mixer_update_bpm(mixr, bpm);

        cmd_found = true;
    }

    else if (strncmp("every", wurds[0], 4) == 0)
    {
        algorithm *a = new_algorithm(num_wurds, wurds);
        if (a)
            mixer_add_algorithm(mixr, a);

        cmd_found = true;
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
        cmd_found = true;
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
        cmd_found = true;
    }

    else if (strncmp("randamp", wurds[0], 7) == 0)
    {
        int sg_num = -1;
        int sg_pattern_num = -1;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            soundgenerator *sg = mixr->sound_generators[sg_num];
            if (sg->is_valid_pattern(sg, sg_pattern_num))
            {
                midi_event *pattern = sg->get_pattern(sg, sg_pattern_num);
                midi_pattern_rand_amp(pattern);
            }
        }
        cmd_found = true;
    }
    else if (strncmp("gen", wurds[0], 3) == 0)
    {
        if (strncmp("melody", wurds[1], 6) == 0)
        {
            int generator = atoi(wurds[2]);
            int dest_sg_num = -1;
            int dest_sg_pattern_num = -1;
            sscanf(wurds[3], "%d:%d", &dest_sg_num, &dest_sg_pattern_num);

            int num_bars = atoi(wurds[4]);
            if (!num_bars)
                num_bars = 1;
            if (num_bars > 4)
            {
                printf("Num bars has be 1, 2, 3 or 4\n");
                return true;
            }

            if (mixer_is_valid_soundgen_num(mixr, dest_sg_num) &&
                mixer_is_valid_seq_gen_num(mixr, generator) &&
                dest_sg_pattern_num != -1 && num_bars >= 0 && num_bars <= 4)
            {
                sequence_generator *seqg = mixr->sequence_generators[generator];
                short int num =
                    seqg->generate(seqg, (void *)&mixr->timing_info.cur_sample);

                soundgenerator *sg =
                    (soundgenerator *)mixr->sound_generators[dest_sg_num];
                synthbase *base = get_synthbase(sg);
                if (!base)
                {
                    printf("Can't do nuttin' for ya, man!\n");
                    return true;
                }

                int multiplier = 1;
                switch (num_bars)
                {
                case (0):
                case (1):
                    multiplier = PPSIXTEENTH;
                    break;
                case (2):
                    multiplier = PPSIXTEENTH * 2;
                    break;
                case (3):
                    multiplier = PPSIXTEENTH * 3;
                    break;
                case (4):
                    multiplier = PPSIXTEENTH * 4;
                    break;
                }

                for (int i = 0; i < num_bars; i++)
                {
                    int note = 0;
                    int midi_note = 0;
                    int midi_tick = 0;
                    int octave = 0;

                    midi_event pattern[PPBAR] = {0};

                    int len = (sizeof(short int) * 8) / num_bars;
                    for (int j = 0; j < len; j++)
                    {
                        if (num & (1 << (15 - j)))
                        {
                            if ((j == 0 && i == 0) ||
                                (j == (len - 1) && i == num_bars - 1))
                                note = mixr->key;
                            else
                            {
                                int randy = rand() % 8;
                                note = mixr->notes[randy];
                            }
                            midi_tick = multiplier * j;
                            octave = synthbase_get_octave(base);
                            if (rand() % 100 > 90)
                                octave++;
                            midi_note =
                                get_midi_note_from_mixer_key(note, octave);
                            midi_event ev = {.event_type = MIDI_ON,
                                             .data1 = midi_note,
                                             .data2 = DEFAULT_VELOCITY};
                            pattern[midi_tick] = ev;
                        }
                    }
                    // midi_pattern_print(pattern);
                    synthbase_set_pattern(base, dest_sg_pattern_num, pattern);
                    dest_sg_pattern_num++;
                }
            }
            else
                printf("Usage: gen melody <gen_num> "
                       "<dest_sg_num>:<dest_sg_pattern_num> <num_bars:default "
                       "1>\n");
        }
        cmd_found = true;
    }
    else if (strncmp("keys", wurds[0], 4) == 0)
    {
        for (int i = 0; i < NUM_KEYS; i++)
        {
            char *key = key_names[i];
            printf("%d [%s] ", i, key);
        }
        printf("\n");
        cmd_found = true;
    }
    else if (strncmp("key", wurds[0], 3) == 0)
    {
        printf("Changing KEY %s!\n", wurds[1]);
        if (strncasecmp(wurds[1], "c#", 2) == 0 ||
            strncasecmp(wurds[1], "db", 2) == 0 ||
            strncasecmp(wurds[1], "dm", 2) == 0)
            mixr->key = C_SHARP;
        else if (strncasecmp(wurds[1], "d#", 2) == 0 ||
                 strncasecmp(wurds[1], "eb", 2) == 0 ||
                 strncasecmp(wurds[1], "em", 2) == 0)
            mixr->key = D_SHARP;
        else if (strncasecmp(wurds[1], "f#", 2) == 0 ||
                 strncasecmp(wurds[1], "gb", 2) == 0 ||
                 strncasecmp(wurds[1], "gm", 2) == 0)
            mixr->key = F_SHARP;
        else if (strncasecmp(wurds[1], "g#", 2) == 0 ||
                 strncasecmp(wurds[1], "ab", 2) == 0 ||
                 strncasecmp(wurds[1], "am", 2) == 0)
            mixr->key = G_SHARP;
        else if (strncasecmp(wurds[1], "a#", 2) == 0 ||
                 strncasecmp(wurds[1], "bb", 2) == 0 ||
                 strncasecmp(wurds[1], "bm", 2) == 0)
            mixr->key = A_SHARP;
        else if (strncasecmp(wurds[1], "c", 1) == 0)
            mixr->key = C;
        else if (strncasecmp(wurds[1], "d", 1) == 0)
            mixr->key = D;
        else if (strncasecmp(wurds[1], "e", 1) == 0)
            mixr->key = E;
        else if (strncasecmp(wurds[1], "f", 1) == 0)
            mixr->key = F;
        else if (strncasecmp(wurds[1], "g", 1) == 0)
            mixr->key = G;
        else if (strncasecmp(wurds[1], "a", 1) == 0)
            mixr->key = A;
        else if (strncasecmp(wurds[1], "b", 1) == 0)
            mixr->key = B;

        mixer_set_notes(mixr);
        cmd_found = true;
    }
    else if (strncmp("notes", wurds[0], 5) == 0)
    {
        printf("NOTES in KEY!\n");

        for (int i = 0; i < 8; i++)
            printf("%s\n", key_names[mixr->notes[i]]);
        // int key = atoi(wurds[1]);
        // if (key >= 0 && key < NUM_KEYS)
        //{
        //    printf("Changing KEY!\n");
        //    mixr->key = key;
        //}
        cmd_found = true;
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
        cmd_found = true;
    }
    else if (strncmp("preview", wurds[0], 7) == 0)
    {
        if (is_valid_file(wurds[1]))
        {
            mixer_preview_track(mixr, wurds[1]);
        }
        cmd_found = true;
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
        cmd_found = true;
    }

    else if (strncmp("ps", wurds[0], 2) == 0)
    {
        mixer_ps(mixr);
        cmd_found = true;
    }

    else if (strncmp("quiet", wurds[0], 5) == 0 ||
             strncmp("hush", wurds[0], 4) == 0)
    {
        for (int i = 0; i < mixr->soundgen_num; i++)
            mixr->sound_generators[i]->setvol(mixr->sound_generators[i], 0.0);
        cmd_found = true;
    }
    else if (strncmp("ls", wurds[0], 2) == 0)
    {
        list_sample_dir(wurds[1]);
        cmd_found = true;
    }

    else if (strncmp("rm", wurds[0], 3) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            printf("Deleting SOUND GEN %d\n", soundgen_num);
            mixer_del_soundgen(mixr, soundgen_num);
        }
        cmd_found = true;
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
        cmd_found = true;
    }
    else if (strncmp("invert", wurds[0], 6) == 0)
    {
        // e.g. invert 0 0:0 1:0
        int sequence_gen_num = atoi(wurds[1]);

        int sg1_num = 0;
        int sg1_track_num = 0;
        sscanf(wurds[2], "%d:%d", &sg1_num, &sg1_track_num);

        int sg2_num = 0;
        int sg2_track_num = 0;
        sscanf(wurds[3], "%d:%d", &sg2_num, &sg2_track_num);
        if (mixer_is_valid_soundgen_track_num(mixr, sg1_num, sg1_track_num) &&
            mixer_is_valid_soundgen_track_num(mixr, sg2_num, sg2_track_num) &&
            mixer_is_valid_seq_gen_num(mixr, sequence_gen_num))
        {
            sequence_generator *sg =
                mixr->sequence_generators[sequence_gen_num];
            uint16_t bit_pattern = sg->generate(sg, NULL);
            uint16_t inverted_bit_pattern = ~bit_pattern;

            midi_event pattern[PPBAR];
            convert_bit_pattern_to_midi_pattern(bit_pattern, 16, pattern, 1, 0);
            soundgenerator *s1 = mixr->sound_generators[sg1_num];
            s1->set_pattern(s1, sg1_track_num, pattern);

            convert_bit_pattern_to_midi_pattern(inverted_bit_pattern, 16,
                                                pattern, 1, 0);
            soundgenerator *s2 = mixr->sound_generators[sg2_num];
            s2->set_pattern(s2, sg1_track_num, pattern);
        }
        cmd_found = true;
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
                soundgenerator *sg = mixr->sound_generators[soundgen_num];
                sg->stop(sg);
            }
        }
        cmd_found = true;
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
        cmd_found = true;
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
        cmd_found = true;
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
        cmd_found = true;
    }
    // PROGRAMMING CMDS
    else if (strncmp("var", wurds[0], 3) == 0 && strncmp("=", wurds[2], 1) == 0)
    {
        printf("Oooh! %s = %s\n", wurds[1], wurds[3]);
        update_environment(wurds[1], atoi(wurds[3]));
        cmd_found = true;
    }

    // UTILS
    else if (strncmp("chord", wurds[0], 6) == 0)
    {
        chordie(wurds[1]);
        cmd_found = true;
    }

    else if (strncmp("strategy", wurds[0], 8) == 0)
    {
        oblique_strategy();
        cmd_found = true;
    }

    return cmd_found;
}
