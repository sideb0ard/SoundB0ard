#include <stdlib.h>
#include <string.h>

#include <fx_cmds.h>
#include <looper_cmds.h>
#include <mixer.h>
#include <mixer_cmds.h>
#include <new_item_cmds.h>
#include <obliquestrategies.h>
#include <pattern_transformers.h>
#include <pattern_utils.h>
#include <stepper_cmds.h>
#include <synth_cmds.h>
#include <utils.h>

extern mixer *mixr;
extern char *key_names[NUM_KEYS];
extern char *chord_type_names[NUM_CHORD_TYPES];

extern wtable *wave_tables[5];

#define MAX_APPLY_TARGETS 5
#define MAX_MODIFIERS 5

bool parse_mixer_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{

    bool cmd_found = false;

    if (strncmp("bpm", wurds[0], 3) == 0)
    {
        int bpm = atoi(wurds[1]);
        if (bpm > 0)
            mixer_update_bpm(mixr, bpm);

        goto cmd_found;
    }
    // individual status types
    else if (strncmp("patz", wurds[0], 4) == 0)
    {
        mixer_status_patz(mixr);
    }
    else if (strncmp("sgz", wurds[0], 3) == 0)
    {
        mixer_status_sgz(mixr, true);
    }
    else if (strncmp("mixr", wurds[0], 4) == 0)
    {
        mixer_status_mixr(mixr);
    }
    else if (strncmp("move", wurds[0], 4) == 0)
    {
        bool b = atoi(wurds[1]);
        mixer_set_should_progress_chords(mixr, b);
    }
    else if (strncmp("bump", wurds[0], 4) == 0)
    {
        mixer_next_chord(mixr);
    }
    else if (strncmp("algoz", wurds[0], 5) == 0)
    {
        if (strncmp("off", wurds[1], 3) == 0)
            for (int i = 0; i < mixr->algorithm_num; i++)
            {
                if (mixer_is_valid_algo(mixr, i))
                {
                    algorithm *a = mixr->algorithms[i];
                    algorithm_stop(a);
                }
            }
        mixer_status_algoz(mixr, true);
    }

    else if (strncmp("bars_per_chord", wurds[0], 15) == 0)
    {
        int bars = atoi(wurds[1]);
        mixer_set_bars_per_chord(mixr, bars);

        goto cmd_found;
    }

    else if (strncmp("pl", wurds[0], 2) == 0 ||
             strncmp("preview", wurds[0], 7) == 0 ||
             strncmp("play", wurds[0], 4) == 0)
    {
        if (is_valid_file(wurds[1]))
            mixer_preview_audio(mixr, wurds[1]);
        goto cmd_found;
    }

    else if (strncmp("fuckup", wurds[0], 6) == 0)
    {
        char fwurds[6][SIZE_OF_WURD] = {"every", "4", "bar", "loop"};
        strcpy(fwurds[4], wurds[1]);
        strcpy(fwurds[5], "scramble");
        algorithm *a = new_algorithm(6, fwurds);
        if (a)
            mixer_add_algorithm(mixr, a);

        strcpy(fwurds[1], "3");
        strcpy(fwurds[5], "stutter");
        a = new_algorithm(6, fwurds);
        if (a)
            mixer_add_algorithm(mixr, a);

        goto cmd_found;
    }

    else if (strncmp("brak", wurds[0], 4) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            if (sequence_engine_is_valid_pattern(engine, sg_pattern_num))
            {
                midi_event *pattern =
                    sequence_engine_get_pattern(engine, sg_pattern_num);
                brak(pattern);
            }
        }
        goto cmd_found;
    }

    else if (strncmp("dense", wurds[0], 5) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            if (sequence_engine_is_valid_pattern(engine, sg_pattern_num))
            {
                int density = atoi(wurds[2]);
                if (density < 2)
                    density = 2;
                midi_event *pattern =
                    sequence_engine_get_pattern(engine, sg_pattern_num);
                dense(pattern, density);
            }
        }
        goto cmd_found;
    }
    else if (strncmp("double", wurds[0], 5) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            if (sequence_engine_is_valid_pattern(engine, sg_pattern_num))
            {
                sequence_engine_pattern_to_double_speed(engine, sg_pattern_num);
            }
        }
        goto cmd_found;
    }
    else if (strncmp("echo", wurds[0], 4) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            if (sequence_engine_is_valid_pattern(engine, sg_pattern_num))
            {
                midi_event *pattern =
                    sequence_engine_get_pattern(engine, sg_pattern_num);
                echo(pattern);
            }
        }
        goto cmd_found;
    }
    else if (strncmp("half", wurds[0], 5) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            if (sequence_engine_is_valid_pattern(engine, sg_pattern_num))
            {
                sequence_engine_pattern_to_half_speed(engine, sg_pattern_num);
            }
        }
        goto cmd_found;
    }
    else if (strncmp("reverse", wurds[0], 7) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            if (sequence_engine_is_valid_pattern(engine, sg_pattern_num))
            {
                midi_event *pattern =
                    sequence_engine_get_pattern(engine, sg_pattern_num);
                reverse(pattern);
            }
        }
        goto cmd_found;
    }
    else if (strncmp("prog", wurds[0], 4) == 0)
    {
        printf("PROG!\n");
        int prog_num = atoi(wurds[1]);
        mixer_set_chord_progression(mixr, prog_num);
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
        goto cmd_found;
    }

    else if (strncmp("randamp", wurds[0], 7) == 0)
    {
        int sg_num = -1;
        int sg_pattern_num = -1;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            sound_generator *sg = mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            if (sequence_engine_is_valid_pattern(engine, sg_pattern_num))
            {
                midi_event *pattern =
                    sequence_engine_get_pattern(engine, sg_pattern_num);
                midi_pattern_rand_amp(pattern);
            }
        }
        goto cmd_found;
    }
    else if (strncmp("sweep", wurds[0], 5) == 0)
    {
        int sg_num = -1;
        int sg_pattern_num = -1;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_fx(mixr, sg_num, sg_pattern_num))
        {
            char fwurds[9][SIZE_OF_WURD] = {"over",          "8", "bar", "osc",
                                            "\"180 18000\"", "fx"};
            strcpy(fwurds[6], wurds[1]);
            strcpy(fwurds[7], "freq");
            strcpy(fwurds[8], "\%s");
            algorithm *a = new_algorithm(9, fwurds);
            if (a)
                mixer_add_algorithm(mixr, a);
        }

        goto cmd_found;
    }
    else if (strncmp("apply", wurds[0], 6) == 0)
    {
        int pg_num = atoi(wurds[1]);
        if (!mixer_is_valid_pattern_gen_num(mixr, pg_num))
        {
            printf("bogey!\n");
            goto cmd_found;
        }
        typedef struct
        {
            int sg_num;
            int sg_pattern_num;
        } Dest;
        Dest dests[MAX_APPLY_TARGETS] = {0};
        int num_dests = 0;
        int dest_sg_num = -1;
        int dest_sg_pattern_num = -1;
        int wurd_idx = 2; // next wurds[i]
        while (sscanf(wurds[wurd_idx], "%d:%d", &dest_sg_num,
                      &dest_sg_pattern_num) == 2 &&
               wurd_idx++ < num_wurds && num_dests < MAX_APPLY_TARGETS)
        {
            dests[num_dests].sg_num = dest_sg_num;
            dests[num_dests].sg_pattern_num = dest_sg_pattern_num;
            num_dests++;
        }

        typedef struct
        {
            char name[20];
        } modifier;
        modifier mods[MAX_MODIFIERS] = {0};
        int num_modifiers = 0;
        int num_wurds_left = num_wurds - 1 - wurd_idx;
        bool num_wurds_left_is_odd = num_wurds_left % 2 == 1;
        if (num_wurds_left > 0 && num_wurds_left_is_odd)
        {
            for (int i = wurd_idx; strncmp("#", wurds[i], 1) == 0 &&
                                   num_modifiers < MAX_MODIFIERS;
                 i += 2)
            {
                strncpy(mods[num_modifiers++].name, wurds[i + 1], 19);
            }
        }

        pattern_generator *pg = mixr->pattern_generators[pg_num];
        midi_event midi_pattern[PPBAR] = {};
        pg->generate(pg, &midi_pattern);

        for (int i = 0; i < num_dests; i++)
        {
            if (mixer_is_valid_soundgen_num(mixr, dests[i].sg_num))
            {
                sound_generator *sg = mixr->sound_generators[dests[i].sg_num];
                sequence_engine *engine = get_sequence_engine(sg);
                pattern_change_info change_info = {.clear_previous = true,
                                                   .temporary = false};
                sequence_engine_set_pattern(engine, dests[i].sg_pattern_num,
                                            change_info,
                                            (midi_event *)&midi_pattern);

                for (int j = 0; j < num_modifiers; j++)
                {
                    char *mod = mods[j].name;
                    if (strncmp("once", mod, 4) == 0)
                        set_pattern_to_self_destruct(
                            sequence_engine_get_pattern(
                                engine, dests[i].sg_pattern_num));

                    else if (strncmp("hspeed", mod, 6) == 0)
                    {
                        sequence_engine_pattern_to_half_speed(
                            engine, dests[i].sg_pattern_num);
                    }
                    else if (strncmp("2speed", mod, 6) == 0)
                    {
                        sequence_engine_pattern_to_double_speed(
                            engine, dests[i].sg_pattern_num);
                    }
                    else if (strncmp("valz", mod, 4) == 0)
                    {
                        if (strlen(mod) != 5)
                        {
                            printf("nae val gen num!\n");
                        }
                        else
                        {
                            int val_gen_num = mod[4] - '0';
                            if (mixer_is_valid_value_gen_num(mixr, val_gen_num))
                            {
                                value_generator *vg =
                                    mixr->value_generators[val_gen_num];
                                pattern_apply_values(
                                    vg, sequence_engine_get_pattern(
                                            engine, dests[i].sg_pattern_num));
                            }
                        }
                    }
                }
            }
        }
    }

    else if (strncmp("keys", wurds[0], 4) == 0)
    {
        for (int i = 0; i < NUM_KEYS; i++)
        {
            char *key = key_names[i];
            printf("%d [%s] ", i, key);
        }
        printf("\n");
        goto cmd_found;
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
        goto cmd_found;
    }
    else if (strncmp("octave", wurds[0], 6) == 0)
    {
        int oct = atoi(wurds[1]);
        mixer_set_octave(mixr, oct);
    }
    else if (strncmp("notes", wurds[0], 5) == 0)
    {
        printf("NOTES in CHORD!\n");

        chord_midi_notes chnotes = {0};
        get_midi_notes_from_chord(mixr->chord, mixr->chord_type, mixr->octave,
                                  &chnotes);
        printf("%d %d %d\n", chnotes.root, chnotes.third, chnotes.fifth);
        goto cmd_found;
    }
    else if (strncmp("chords", wurds[0], 6) == 0)
    {
        printf("CHORDS in KEY!\n");

        for (int i = 0; i < 8; i++)
            printf("%s %s\n", key_names[mixr->notes[i]],
                   chord_type_names[get_chord_type(i)]);
        goto cmd_found;
    }
    else if (strncmp("chord", wurds[0], 5) == 0)
    {
        if (strncmp("gen", wurds[1], 3) == 0)
        {
            for (int i = 0; i < 4; i++)
            {
                int note_num = rand() % 8;
                int note = mixr->notes[note_num];
                unsigned int chord_type = get_chord_type(note_num);
                printf("%s %s\n", key_names[note],
                       chord_type_names[chord_type]);
                chord_midi_notes chnotes;
                get_midi_notes_from_chord(note, chord_type, mixr->octave,
                                          &chnotes);
            }
        }
    }
    else if (strncmp("<~", wurds[0], 2) == 0 || strncmp("~>", wurds[0], 2) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {
            sound_generator *sg =
                (sound_generator *)mixr->sound_generators[sg_num];
            sequence_engine *engine = get_sequence_engine(sg);
            if (sequence_engine_is_valid_pattern(engine, sg_pattern_num))
            {
                int places_to_shift = atoi(wurds[2]);
                if (!places_to_shift)
                    places_to_shift = 4;
                midi_event *pattern =
                    sequence_engine_get_pattern(engine, sg_pattern_num);
                if (strcmp("<~", wurds[0]) == 0)
                    left_shift(pattern, places_to_shift);
                else
                    right_shift(pattern, places_to_shift);
            }
        }
        goto cmd_found;
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
        goto cmd_found;
    }

    else if (strncmp("ps", wurds[0], 2) == 0)
    {
        if (strncmp("mixr", wurds[1], 4) == 0)
            mixer_status_mixr(mixr);
        else if (strncmp("all", wurds[1], 3) == 0)
            mixer_ps(mixr, true);
        else
            mixer_ps(mixr, false);

        goto cmd_found;
    }

    else if (strncmp("quiet", wurds[0], 5) == 0 ||
             strncmp("hush", wurds[0], 4) == 0)
    {
        for (int i = 0; i < mixr->soundgen_num; i++)
            mixr->sound_generators[i]->set_volume(mixr->sound_generators[i],
                                                  0.0);
        goto cmd_found;
    }
    else if (strncmp("ls", wurds[0], 2) == 0)
    {
        list_sample_dir(wurds[1]);
        goto cmd_found;
    }

    else if (strncmp("rm", wurds[0], 3) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            printf("Deleting SOUND GEN %d\n", soundgen_num);
            mixer_del_soundgen(mixr, soundgen_num);
        }
        goto cmd_found;
    }
    else if (strncmp("start", wurds[0], 5) == 0)
    {
        if (strncmp(wurds[1], "all", 3) == 0)
        {
            for (int i = 0; i < mixr->soundgen_num; i++)
            {
                sound_generator *sg = mixr->sound_generators[i];
                if (sg != NULL)
                    sg->start(sg);
            }
        }
        else
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                sound_generator *sg = mixr->sound_generators[soundgen_num];
                sg->start(sg);
            }
        }
        goto cmd_found;
    }
    else if (strncmp("invert", wurds[0], 6) == 0 ||
             strncmp("blend", wurds[0], 5) == 0)
    {
        // e.g. invert 0 0:0 1:0
        int pattern_gen_num = atoi(wurds[1]);

        int sg1_num = 0;
        int sg1_track_num = 0;
        sscanf(wurds[2], "%d:%d", &sg1_num, &sg1_track_num);

        int sg2_num = 0;
        int sg2_track_num = 0;
        sscanf(wurds[3], "%d:%d", &sg2_num, &sg2_track_num);
        if (mixer_is_valid_soundgen_track_num(mixr, sg1_num, sg1_track_num) &&
            mixer_is_valid_soundgen_track_num(mixr, sg2_num, sg2_track_num) &&
            mixer_is_valid_pattern_gen_num(mixr, pattern_gen_num))
        {
            pattern_generator *pg = mixr->pattern_generators[pattern_gen_num];

            sound_generator *s1 = mixr->sound_generators[sg1_num];
            sequence_engine *e1 = get_sequence_engine(s1);
            sound_generator *s2 = mixr->sound_generators[sg2_num];
            sequence_engine *e2 = get_sequence_engine(s2);

            midi_event midi_pattern[PPBAR] = {};
            pg->generate(pg, &midi_pattern);
            uint16_t bit_pattern =
                midi_pattern_to_short((midi_event *)&midi_pattern);

            uint16_t pattern_1, pattern_2 = 0;

            if (strncmp("invert", wurds[0], 6) == 0)
            {
                pattern_1 = bit_pattern;
                pattern_2 = ~bit_pattern;
            }
            else // blend
            {
                pattern_1 = bit_pattern & 0xFF00;
                pattern_2 = bit_pattern & 0x00FF;
            }

            pattern_change_info change_info = {.clear_previous = true,
                                               .temporary = false};

            midi_event pattern[PPBAR];
            apply_short_to_midi_pattern(pattern_1, pattern);
            sequence_engine_set_pattern(e1, sg1_track_num, change_info,
                                        pattern);

            apply_short_to_midi_pattern(pattern_2, pattern);
            sequence_engine_set_pattern(e2, sg1_track_num, change_info,
                                        pattern);
        }
        goto cmd_found;
    }
    else if (strncmp("stop", wurds[0], 5) == 0)
    {
        if (strncmp(wurds[1], "all", 3) == 0)
        {
            for (int i = 0; i < mixr->soundgen_num; i++)
            {
                sound_generator *sg = mixr->sound_generators[i];
                if (sg != NULL)
                    sg->stop(sg);
            }
        }
        else
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                sound_generator *sg = mixr->sound_generators[soundgen_num];
                sg->stop(sg);
            }
        }
        goto cmd_found;
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
        goto cmd_found;
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
        goto cmd_found;
    }
    else if (strncmp("pan", wurds[0], 3) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            double val = atof(wurds[2]);
            pan_change(mixr, soundgen_num, val);
        }
        goto cmd_found;
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
        goto cmd_found;
    }
    // PROGRAMMING CMDS
    else if (strncmp("var", wurds[0], 3) == 0 && strncmp("=", wurds[2], 1) == 0)
    {
        printf("Oooh! %s = %s\n", wurds[1], wurds[3]);
        update_environment(wurds[1], atoi(wurds[3]));
        goto cmd_found;
    }

    // UTILS
    else if (strncmp("chord", wurds[0], 6) == 0)
    {
        chordie(wurds[1]);
        goto cmd_found;
    }

    else if (strncmp("strategy", wurds[0], 8) == 0)
    {
        oblique_strategy();
        goto cmd_found;
    }

    return false;
cmd_found:
    return true;
}