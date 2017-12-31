#include <locale.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wchar.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "algorithm.h"
#include "basicfilterpass.h"
#include "beatrepeat.h"
#include "bitcrush.h"
#include "chaosmonkey.h"
#include "cmdloop.h"
#include "defjams.h"
#include "digisynth.h"
#include "distortion.h"
#include "dxsynth.h"
#include "dynamics_processor.h"
#include "envelope.h"
#include "envelope_follower.h"
#include "euclidean.h"
#include "granulator.h"
#include "help.h"
#include "keys.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "modfilter.h"
#include "modular_delay.h"
#include "obliquestrategies.h"
#include "oscillator.h"
#include "pattern_parser.h"
#include "reverb.h"
#include "sample_sequencer.h"
#include "sequencer_utils.h"
#include "sparkline.h"
#include "spork.h"
#include "synthbase.h"
#include "synthdrum_sequencer.h"
#include "table.h"
#include "utils.h"
#include "waveshaper.h"

extern mixer *mixr;
extern char *key_names[NUM_KEYS];

extern wtable *wave_tables[5];

//#define READLINE_SAFE_MAGENTA "\001\x1b[35m\002"
//#define READLINE_SAFE_RESET "\001\x1b[0m\002"
// char const *prompt = READLINE_SAFE_MAGENTA "SB#> " READLINE_SAFE_RESET;
// Turns out OSX uses NetBSD editline to implement readline and it has a
// bug that means color prompt is broke -
// https://stackoverflow.com/questions/31329952/colorized-readline-prompt-breaks-control-a
// TODO(find a fix)
char const *prompt = "SB#> ";

void loopy(void)
{
    read_history(NULL);

    setlocale(LC_ALL, "");

    char *line;
    while ((line = readline(prompt)) != NULL)
    {
        // if (line[0] != 0)
        if (line && *line)
        {
            add_history(line);
            interpret(line);
            free(line);
        }
    }
    write_history(NULL);
    printf("BYTE!\n");
}

void interpret(char *line)
{
    char wurds[NUM_WURDS][SIZE_OF_WURD];

    char *cmd, *last_s;
    char const *sep = ",";
    for (cmd = strtok_r(line, sep, &last_s); cmd;
         cmd = strtok_r(NULL, sep, &last_s))
    {

        char tmp[1024];
        strncpy((char *)tmp, cmd, 127);

        int num_wurds = parse_wurds_from_cmd(wurds, tmp);

        // (TODO) some kid of doxygen-like scheme to write help output here

        //////  MIXER COMMANDS  /////////////////////////
        if (strncmp("help", wurds[0], 4) == 0)
        {
            oblique_strategy();
        }

        else if (strncmp("quit", wurds[0], 4) == 0 ||
                 strncmp("exit", wurds[0], 4) == 0)
        {
            exxit();
        }

        else if (strncmp("bpm", wurds[0], 3) == 0)
        {
            int bpm = atoi(wurds[1]);
            if (bpm > 0)
                mixer_update_bpm(mixr, bpm);
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
        }

        else if (strncmp("compat", wurds[0], 6) == 0)
        {
            if (strncmp("keys", wurds[1], 4) == 0)
            {
                mixer_print_compat_keys(mixr);
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
        }
        else if (strncmp("key", wurds[0], 3) == 0)
        {
            int key = atoi(wurds[1]);
            if (key >= 0 && key < NUM_KEYS)
            {
                printf("Changing KEY!\n");
                mixr->key = key;
            }
        }
        else if (strncmp("new", wurds[0], 3) == 0)
        {
            if (strncmp("bitshift", wurds[1], 4) == 0)
            {
                printf("BITSHIFT! SEQUENCE GEN!\n");
                mixer_add_bitshift(mixr, num_wurds - 2, &wurds[2]);
            }
            else if (strncmp("euclid", wurds[1], 6) == 0)
            {
                if (num_wurds != 4)
                {
                    printf("get a life, mate - need to gimme number of hits "
                           "and number of steps\n");
                    continue;
                }
                int num_hits = atoi(wurds[2]);
                int num_steps = atoi(wurds[3]);
                printf("EUCLIDEAN! SEQUENCE GEN num_hits:%d num_steps:%d!\n",
                       num_hits, num_steps);
                mixer_add_euclidean(mixr, num_hits, num_steps);
            }
        }

        else if (strncmp("preview", wurds[0], 7) == 0)
        {
            if (is_valid_file(wurds[1]))
            {
                mixer_preview_track(mixr, wurds[1]);
            }
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
        }

        else if (strncmp("ps", wurds[0], 2) == 0)
        {
            mixer_ps(mixr);
        }

        else if (strncmp("quiet", wurds[0], 5) == 0)
        {
            for (int i = 0; i < mixr->soundgen_num; i++)
                mixr->sound_generators[i]->setvol(mixr->sound_generators[i],
                                                  0.0);
        }
        else if (strncmp("ls", wurds[0], 2) == 0)
        {
            list_sample_dir(wurds[1]);
        }

        else if (strncmp("rm", wurds[0], 3) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                printf("Deleting SOUND GEN %d\n", soundgen_num);
                mixer_del_soundgen(mixr, soundgen_num);
            }
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
                            mixer_add_soundgen_track_to_scene(
                                mixr, scene_num, sg_num, sg_track_num);
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
                        int new_scene_num =
                            mixer_add_scene(mixr, default_num_bars);
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
        }

        else if (strncmp("seq", wurds[0], 3) == 0)
        {
            int sgnum = atoi(wurds[1]);
            if (mixer_is_valid_seq_gen_num(mixr, sgnum))
            {
                sequence_generator *sg = mixr->sequence_generators[sgnum];
                if (strncmp("gen", wurds[2], 3) == 0)
                {
                    int num =
                        sg->generate(sg, (void *)&mixr->timing_info.cur_sample);
                    char binnum[17] = {0};
                    char_binary_version_of_int(num, binnum);
                    printf("NOM!: %d %s\n", num, binnum);
                }
                if (sg->type == EUCLIDEAN)
                {
                    euclidean *e = (euclidean *)sg;
                    if (strncmp(wurds[2], "mode", 4) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        euclidean_change_mode(e, mode);
                    }
                    if (strncmp(wurds[2], "hits", 4) == 0)
                    {
                        int val = atoi(wurds[3]);
                        euclidean_change_hits(e, val);
                    }
                    if (strncmp(wurds[2], "steps", 5) == 0)
                    {
                        int val = atoi(wurds[3]);
                        euclidean_change_steps(e, val);
                    }
                }
            }
        }
        else if (strncmp("synthdrum", wurds[0], 9) == 0 ||
                 strncmp("sd", wurds[0], 2) == 0)
        {
            char pattern[151] = {0};
            if (strncmp("new", wurds[1], 4) == 0)
            {
                synthdrum_sequencer *sds = new_synthdrum_seq();
                char_array_to_seq_string_pattern(&sds->m_seq, pattern, wurds, 2,
                                                 num_wurds);
                int sgnum = add_sound_generator(
                    mixr,
                    (soundgenerator *)sds); //  add_seq_char_pattern(mixr,
                                            //  wurds[1], pattern);
                printf("New Sound Generator (SD): %d\n", sgnum);
                pattern_char_to_pattern(
                    &sds->m_seq, pattern,
                    sds->m_seq.patterns[sds->m_seq.num_patterns++]);
            }
            if (strncmp("list", wurds[1], 4) == 0 ||
                strncmp("ls", wurds[1], 2) == 0)
            {
                printf("Listing SYNTHDRUM patches.. \n");
                synthdrum_list_patches();
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        SYNTHDRUM_TYPE)
                {

                    synthdrum_sequencer *sds =
                        (synthdrum_sequencer *)
                            mixr->sound_generators[soundgen_num];

                    if (strncmp("osc1_wav", wurds[2], 8) == 0)
                    {
                        int val = atoi(wurds[3]);
                        synthdrum_set_osc_wav(sds, 1, val);
                    }
                    else if (strncmp("osc1_fo", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_fo(sds, 1, val);
                    }
                    else if (strncmp("osc1_amp", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_amp(sds, 1, val);
                    }
                    else if (strncmp("eg1_attack", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_attack(sds, 1, val);
                    }
                    else if (strncmp("eg1_decay", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_decay(sds, 1, val);
                    }
                    else if (strncmp("eg1_sustain_ms", wurds[2], 14) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_sustain_ms(sds, 1, val);
                    }
                    else if (strncmp("eg1_sustain_level", wurds[2], 17) == 0)
                    {
                        printf("Sustain LEvel!\n");
                        double val = atof(wurds[3]);
                        eg_set_sustain_level(&sds->m_eg1, val);
                    }
                    else if (strncmp("eg1_release", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_release(sds, 1, val);
                    }
                    else if (strncmp("osc2_wav", wurds[2], 8) == 0)
                    {
                        int val = atoi(wurds[3]);
                        synthdrum_set_osc_wav(sds, 2, val);
                    }
                    else if (strncmp("osc2_fo", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_fo(sds, 2, val);
                    }
                    else if (strncmp("osc2_amp", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_amp(sds, 2, val);
                    }
                    else if (strncmp("osc3_wav", wurds[2], 8) == 0)
                    {
                        int val = atoi(wurds[3]);
                        synthdrum_set_osc_wav(sds, 3, val);
                    }
                    else if (strncmp("osc3_fo", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_fo(sds, 3, val);
                    }
                    else if (strncmp("eg2_attack", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_attack(sds, 2, val);
                    }
                    else if (strncmp("eg2_decay", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_decay(sds, 2, val);
                    }
                    else if (strncmp("eg2_sustain_ms", wurds[2], 14) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_sustain_ms(sds, 2, val);
                    }
                    else if (strncmp("eg2_sustain_level", wurds[2], 17) == 0)
                    {
                        printf("Sustain LEvel!\n");
                        double val = atof(wurds[3]);
                        eg_set_sustain_level(&sds->m_eg2, val);
                    }
                    else if (strncmp("eg2_release", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_release(sds, 2, val);
                    }
                    else if (strncmp("eg2_osc2_int", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg2_osc_intensity(sds, val);
                    }
                    else if (strncmp("eg3_attack", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_attack(sds, 3, val);
                    }
                    else if (strncmp("eg3_decay", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_decay(sds, 3, val);
                    }
                    else if (strncmp("eg3_sustain_ms", wurds[2], 14) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_sustain_ms(sds, 3, val);
                    }
                    else if (strncmp("eg3_sustain_level", wurds[2], 17) == 0)
                    {
                        printf("Sustain LEvel!\n");
                        double val = atof(wurds[3]);
                        eg_set_sustain_level(&sds->m_eg3, val);
                    }
                    else if (strncmp("eg3_release", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_release(sds, 3, val);
                    }
                    else if (strncmp("filter_type", wurds[2], 11) == 0)
                    {
                        printf("filter tyyyype!\n");
                        int val = atof(wurds[3]);
                        synthdrum_set_filter_type(sds, val);
                    }
                    else if (strncmp("freq", wurds[2], 4) == 0)
                    {
                        printf("Freq!\n");
                        double val = atof(wurds[3]);
                        synthdrum_set_filter_freq(sds, val);
                    }
                    else if (strncmp("q", wurds[2], 1) == 0)
                    {
                        printf("Q!\n");
                        double val = atof(wurds[3]);
                        synthdrum_set_filter_q(sds, val);
                    }
                    else if (strncmp("distortion_threshold", wurds[2], 20) == 0)
                    {
                        printf("DIZZTORION!\n");
                        double val = atof(wurds[3]);
                        synthdrum_set_distortion_threshold(sds, val);
                    }
                    else if (strncmp("mod_pitch_semitones", wurds[2], 19) == 0)
                    {
                        printf("modPITCH!!\n");
                        int val = atoi(wurds[3]);
                        synthdrum_set_mod_semitones_range(sds, val);
                    }
                    else if (strncmp("save", wurds[2], 4) == 0 ||
                             strncmp("export", wurds[2], 6) == 0)
                    {
                        printf("Saving SYNTHDRUM patch..\n");
                        synthdrum_save_patch(sds, wurds[3]);
                    }
                    else if (strncmp("open", wurds[2], 4) == 0)
                    {
                        printf("Opening SYNTHDRUM "
                               "patches.. \n");
                        synthdrum_open_patch(sds, wurds[3]);
                    }
                    else if (strncmp("rand", wurds[2], 4) == 0 ||
                             strncmp("randomize", wurds[2], 9) == 0)
                    {
                        printf("Ra-rarararrandom SYNTHDRUM patch!\n");
                        synthdrum_randomize(sds);
                    }
                    else
                    {
                        printf("SYNTHDRUM SEQ!\n");
                        parse_sample_sequencer_command(&sds->m_seq, wurds,
                                                       num_wurds, pattern);
                    }
                }
            }
        }

        //////  SAMP SEQUENCER COMMANDS  /////////////////////////
        else if (strncmp("samp", wurds[0], 3) == 0)
        {

            char *pattern = (char *)calloc(151, sizeof(char));

            if (strncmp(wurds[1], "kit", 3) == 0)
            {
                char kickfile[512] = {0};
                get_random_sample_from_dir("kicks", kickfile);
                printf("Opening %s\n", kickfile);
                sample_sequencer *bd = new_sample_seq(kickfile);
                int bdnum = add_sound_generator(mixr, (soundgenerator *)bd);
                pattern_char_to_pattern(
                    &bd->m_seq, pattern,
                    bd->m_seq.patterns[bd->m_seq.num_patterns++]);
                update_environment("bd", bdnum);

                char snarefile[512] = {0};
                get_random_sample_from_dir("snrs", snarefile);
                printf("Opening %s\n", snarefile);
                sample_sequencer *sd = new_sample_seq(snarefile);
                int sdnum = add_sound_generator(mixr, (soundgenerator *)sd);
                pattern_char_to_pattern(
                    &sd->m_seq, pattern,
                    sd->m_seq.patterns[sd->m_seq.num_patterns++]);
                update_environment("sd", sdnum);

                char highhat[512] = {0};
                get_random_sample_from_dir("hats", highhat);
                printf("Opening %s\n", highhat);
                sample_sequencer *hh = new_sample_seq(highhat);
                int hhnum = add_sound_generator(mixr, (soundgenerator *)hh);
                pattern_char_to_pattern(
                    &hh->m_seq, pattern,
                    hh->m_seq.patterns[hh->m_seq.num_patterns++]);
                update_environment("hh", hhnum);

                char perc[512] = {0};
                get_random_sample_from_dir("perc", perc);
                printf("Opening %s\n", perc);
                sample_sequencer *pc = new_sample_seq(perc);
                int pcnum = add_sound_generator(mixr, (soundgenerator *)pc);
                pattern_char_to_pattern(
                    &pc->m_seq, pattern,
                    pc->m_seq.patterns[pc->m_seq.num_patterns++]);
                update_environment("pc", pcnum);
            }
            else if (is_valid_file(wurds[1]))
            {
                sample_sequencer *s = new_sample_seq(wurds[1]);
                char_array_to_seq_string_pattern(&s->m_seq, pattern, wurds, 2,
                                                 num_wurds);
                int sgnum = add_sound_generator(
                    mixr,
                    (soundgenerator *)s); //  add_seq_char_pattern(mixr,
                                          //  wurds[1], pattern);
                pattern_char_to_pattern(
                    &s->m_seq, pattern,
                    s->m_seq.patterns[s->m_seq.num_patterns++]);

                printf("New SG at pos %d\n", sgnum);
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        SEQUENCER_TYPE)
                {

                    if (strncmp("morph", wurds[2], 8) == 0)
                    {
                        printf("M0RRRPH!\n");
                        sample_sequencer *s =
                            (sample_sequencer *)
                                mixr->sound_generators[soundgen_num];
                        s->morph = 1 - s->morph;
                    }
                    else if (strncmp("load", wurds[2], 4) == 0 ||
                             strncmp("import", wurds[2], 6) == 0)
                    {
                        if (is_valid_file(wurds[3]))
                        {
                            printf("Changing Loaded "
                                   "FILE!\n");
                            sample_sequencer *s =
                                (sample_sequencer *)
                                    mixr->sound_generators[soundgen_num];
                            sample_seq_import_file(s, wurds[3]);
                        }
                        else
                            printf("%s is not a valid file\n", wurds[3]);
                    }
                    else
                    {
                        sample_sequencer *s =
                            (sample_sequencer *)
                                mixr->sound_generators[soundgen_num];
                        sequencer *seq = &s->m_seq;
                        parse_sample_sequencer_command(seq, wurds, num_wurds,
                                                       pattern);
                    }
                }
            }
            free(pattern);
        }

        else if (strncmp("spork", wurds[0], 5) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                mixr->sound_generators[soundgen_num]->type == SPORK_TYPE)
            {
                spork *s = (spork *)mixr->sound_generators[soundgen_num];
                printf("Your Spork is my command!\n");
                double val = atof(wurds[3]);
                if (strncmp("freq", wurds[2], 4) == 0)
                    spork_set_freq(s, val);
                else if (strncmp("waveform", wurds[2], 8) == 0)
                    spork_set_waveform(s, val);
                else if (strncmp("mode", wurds[2], 4) == 0)
                    spork_set_mode(s, val);
                else if (strncmp("polarity", wurds[2], 4) == 0)
                    spork_set_polarity(s, val);
            }
        }
        // GRANULATOR COMMANDS
        else if (strncmp("granulator", wurds[0], 8) == 0 ||
                 strncmp("gran", wurds[0], 4) == 0)
        {
            if (is_valid_file(wurds[1]) || strncmp(wurds[1], "none", 4) == 0)
            {
                printf("VALID!\n");
                int soundgen_num = add_granulator(mixr, wurds[1]);
                printf("soundgenerator %d\n", soundgen_num);
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        GRANULATOR_TYPE)
                {
                    granulator *g =
                        (granulator *)mixr->sound_generators[soundgen_num];
                    if (strncmp("grain_duration_ms", wurds[2], 14) == 0)
                    {
                        int dur = atoi(wurds[3]);
                        granulator_set_grain_duration(g, dur);
                    }
                    else if (strncmp("grains_per_sec", wurds[2], 14) == 0)
                    {
                        int gps = atoi(wurds[3]);
                        granulator_set_grains_per_sec(g, gps);
                    }
                    else if (strncmp("audio_buffer_read_idx", wurds[2], 14) ==
                             0)
                    {
                        int pos = atoi(wurds[3]);
                        granulator_set_audio_buffer_read_idx(g, pos);
                    }
                    else if (strncmp("grain_spray_ms", wurds[2], 14) == 0)
                    {
                        int spray = atoi(wurds[3]);
                        granulator_set_granular_spray(g, spray);
                    }
                    else if (strncmp("quasi_grain_fudge", wurds[2], 17) == 0)
                    {
                        int fudge = atoi(wurds[3]);
                        granulator_set_quasi_grain_fudge(g, fudge);
                    }
                    else if (strncmp("extsource", wurds[2], 9) == 0)
                    {
                        printf("EXTSOURCE!\n");
                        int sg = atoi(wurds[3]);
                        if (mixer_is_valid_soundgen_num(mixr, sg))
                        {
                            printf("GRAN is following "
                                   "%d\n",
                                   sg);
                            granulator_set_external_source(g, sg);
                        }
                    }
                    else if (strncmp("file", wurds[2], 4) == 0 ||
                             strncmp("open", wurds[2], 4) == 0 ||
                             strncmp("import", wurds[2], 6) == 0)
                    {
                        if (is_valid_file(wurds[3]))
                        {
                            granulator_import_file(g, wurds[3]);
                        }
                    }
                    else if (strncmp("grain_pitch", wurds[2], 10) == 0)
                    {
                        double pitch = atof(wurds[3]);
                        granulator_set_grain_pitch(g, pitch);
                    }
                    else if (strncmp("selection_mode", wurds[2], 14) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        granulator_set_selection_mode(g, mode);
                    }
                    else if (strncmp("env_mode", wurds[2], 8) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        printf("ENV MODE is %d\n", mode);
                        granulator_set_envelope_mode(g, mode);
                    }
                    else if (strncmp("sequencer_mode", wurds[2], 13) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        printf("MODE is %d\n", mode);
                        granulator_set_sequencer_mode(g, mode);
                    }
                    else if (strncmp("movement", wurds[2], 8) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        printf("Movement MODE is %d\n", mode);
                        granulator_set_movement_mode(g, mode);
                    }
                    else if (strncmp("reverse", wurds[2], 8) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        printf("REverse MODE is %d\n", mode);
                        granulator_set_reverse_mode(g, mode);
                    }
                    else if (strncmp("lfo1_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", mode);
                        granulator_set_lfo_voice(g, 1, type);
                    }
                    else if (strncmp("lfo1_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        granulator_set_lfo_amp(g, 1, amp);
                    }
                    else if (strncmp("lfo1_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        granulator_set_lfo_rate(g, 1, rate);
                    }
                    else if (strncmp("lfo1_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        granulator_set_lfo_sync(g, 1, loops);
                    }
                    else if (strncmp("lfo1_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        granulator_set_lfo_min(g, 1, min);
                    }
                    else if (strncmp("lfo1_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        granulator_set_lfo_max(g, 1, max);
                    }
                    else if (strncmp("lfo2_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", mode);
                        granulator_set_lfo_voice(g, 2, type);
                    }
                    else if (strncmp("lfo2_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        granulator_set_lfo_amp(g, 2, amp);
                    }
                    else if (strncmp("lfo2_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        granulator_set_lfo_rate(g, 2, rate);
                    }
                    else if (strncmp("lfo2_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        granulator_set_lfo_sync(g, 2, loops);
                    }
                    else if (strncmp("lfo2_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        granulator_set_lfo_min(g, 2, min);
                    }
                    else if (strncmp("lfo2_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        granulator_set_lfo_max(g, 2, max);
                    }
                    else if (strncmp("lfo3_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", mode);
                        granulator_set_lfo_voice(g, 3, type);
                    }
                    else if (strncmp("lfo3_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        granulator_set_lfo_amp(g, 3, amp);
                    }
                    else if (strncmp("lfo3_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        granulator_set_lfo_rate(g, 3, rate);
                    }
                    else if (strncmp("lfo3_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        granulator_set_lfo_sync(g, 3, loops);
                    }
                    else if (strncmp("lfo3_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        granulator_set_lfo_min(g, 3, min);
                    }
                    else if (strncmp("lfo3_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        granulator_set_lfo_max(g, 3, max);
                    }
                    else if (strncmp("lfo4_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", mode);
                        granulator_set_lfo_voice(g, 4, type);
                    }
                    else if (strncmp("lfo4_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        granulator_set_lfo_amp(g, 4, amp);
                    }
                    else if (strncmp("lfo4_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        granulator_set_lfo_rate(g, 4, rate);
                    }
                    else if (strncmp("lfo4_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        granulator_set_lfo_sync(g, 4, loops);
                    }
                    else if (strncmp("lfo4_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        granulator_set_lfo_min(g, 4, min);
                    }
                    else if (strncmp("lfo4_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        granulator_set_lfo_max(g, 4, max);
                    }
                    else if (strncmp("graindur_lfo_on", wurds[2], 14) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->graindur_lfo_on = b;
                    }
                    else if (strncmp("grainpitch_lfo_on", wurds[2], 17) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->grainpitch_lfo_on = b;
                    }
                    else if (strncmp("grainps_lfo_on", wurds[2], 13) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->grainps_lfo_on = b;
                    }
                    else if (strncmp("grainscan_lfo_on", wurds[2], 16) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->grainscanfile_lfo_on = b;
                    }
                    else if (strncmp("eg_amp_attack_ms", wurds[2], 16) == 0)
                    {
                        int attack = atoi(wurds[3]);
                        eg_set_attack_time_msec(&g->m_eg1, attack);
                    }
                    else if (strncmp("eg_amp_release_ms", wurds[2], 17) == 0)
                    {
                        int release = atoi(wurds[3]);
                        eg_set_release_time_msec(&g->m_eg1, release);
                    }
                    else
                    {
                        printf("ELSEY SEQUENCE!\n");
                        char *pattern = (char *)calloc(151, sizeof(char));
                        sequencer *seq = &g->m_seq;
                        parse_sample_sequencer_command(seq, wurds, num_wurds,
                                                       pattern);
                        free(pattern);
                    }
                }
            }
        }

        // SAMPLE LOOPER COMMANDS
        else if (strncmp("loop", wurds[0], 4) == 0)
        {
            if (is_valid_file(wurds[1]) || strncmp(wurds[1], "none", 4) == 0)
            {
                int loop_len = atoi(wurds[2]);
                int soundgen_num = add_looper(mixr, wurds[1], loop_len);
                printf("soundgenerator %d\n", soundgen_num);
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type == LOOPER_TYPE)
                {

                    looper *s = (looper *)mixr->sound_generators[soundgen_num];

                    if (strncmp("add", wurds[2], 6) == 0)
                    {
                        if (is_valid_file(wurds[3]) ||
                            strncmp(wurds[3], "none", 4) == 0)
                        {
                            int loop_len = atoi(wurds[4]);
                            if (loop_len > 0)
                            {
                                looper_add_sample(s, wurds[3], loop_len);
                            }
                        }
                    }
                    else if (strncmp("change", wurds[2], 6) == 0)
                    {
                        int sample_num = atoi(wurds[3]);
                        if (is_valid_sample_num(s, sample_num))
                        {
                            if (strncmp("looplen", wurds[4], 8) == 0)
                            {
                                double looplen = atoi(wurds[5]);
                                s->pending_loop_num = sample_num;
                                s->pending_loop_size = looplen;
                                s->change_loopsize_pending = true;
                                printf("CHANGE PENDING\n");
                            }
                            else if (strncmp("numloops", wurds[4], 8) == 0)
                            {
                                int numloops = atoi(wurds[5]);
                                if (numloops != 0)
                                    looper_change_num_loops(s, sample_num,
                                                            numloops);
                            }
                        }
                    }
                    else if (strncmp("multi", wurds[2], 5) == 0)
                    {
                        if (strncmp("true", wurds[3], 4) == 0)
                        {
                            looper_set_multi_sample_mode(s, true);
                        }
                        else if (strncmp("false", wurds[3], 5) == 0)
                        {
                            looper_set_multi_sample_mode(s, false);
                        }
                        printf("Sampler multi mode : %s\n",
                               s->multi_sample_mode ? "true" : "false");
                    }
                    else if (strncmp("scramble", wurds[2], 8) == 0)
                    {
                        if (strncmp(wurds[3], "every", 5) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                printf("Scrambling "
                                       "every %d n "
                                       "loops!\n",
                                       num_gens);
                                looper_set_scramble_mode(s, true);
                                s->scramble_every_n_loops = num_gens;
                            }
                            else
                            {
                                printf("Need a "
                                       "number for "
                                       "every "
                                       "'n'\n");
                            }
                        }
                        else
                        {
                            int max_gen = atoi(wurds[3]);
                            if (max_gen > 0)
                            {
                                looper_set_max_generation(s, max_gen);
                                looper_set_scramble_mode(s, true);
                            }
                            else
                            {
                                printf("Toggling "
                                       "scramble.."
                                       "\n");
                                int new_mode = 1 - s->scramblrrr_mode;
                                looper_set_scramble_mode(s, new_mode);
                            }
                        }
                    }
                    else if (strncmp("stutter", wurds[2], 7) == 0)
                    {
                        if (strncmp(wurds[3], "every", 4) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                printf("Stuttering "
                                       "every %d n "
                                       "loops!\n",
                                       num_gens);
                                looper_set_stutter_mode(s, true);
                                s->stutter_every_n_loops = num_gens;
                            }
                            else
                            {
                                printf("Need a "
                                       "number for "
                                       "every "
                                       "'n'\n");
                            }
                        }
                        else if (strncmp(wurds[3], "for", 3) == 0)
                        {
                            int max_gen = atoi(wurds[3]);
                            if (max_gen > 0)
                            {
                                looper_set_max_generation(s, max_gen);
                                looper_set_stutter_mode(s, true);
                            }
                            else
                            {
                                printf("Need a "
                                       "number of "
                                       "loops for "
                                       "'for'\n");
                            }
                        }
                        else if (strncmp(wurds[3], "true", 4) == 0)
                            looper_set_stutter_mode(s, true);
                        else if (strncmp(wurds[3], "false", 5) == 0)
                            looper_set_stutter_mode(s, false);
                        else
                        {
                            int new_mode = 1 - s->stutter_mode;
                            printf("Toggling sTUTTER "
                                   "to %s..\n",
                                   new_mode ? "true" : "false");
                            looper_set_stutter_mode(s, new_mode);
                        }
                    }
                    else if (strncmp("switch", wurds[3], 6) == 0)
                    {
                        int sample_num = atoi(wurds[3]);
                        looper_switch_sample(s, sample_num);
                    }
                }
            }
        }

        else if (strncmp("midi", wurds[0], 4) == 0)
        {
            printf("MIDI Time!\n");
            if (num_wurds == 1)
                return;
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
        }

        // SINGLE SHOT SAMPLE PLAYER COMMANDS
        else if (strncmp("play", wurds[0], 4) == 0)
        {
            printf("Playing onetime sample...[TODO]\n");
        }

        // SYNTHESIZER COMMANDS
        else if (strncmp("syn", wurds[0], 3) == 0)
        {
            if (strncmp("mini", wurds[1], 4) == 0)
            {
                int sgnum = add_minisynth(mixr);
                mixr->midi_control_destination = SYNTH;
                mixr->active_midi_soundgen_num = sgnum;
                if (num_wurds > 2)
                {
                    minisynth *ms = (minisynth *)mixr->sound_generators[sgnum];
                    char_melody_to_midi_melody(&ms->base, 0, wurds, 2,
                                               num_wurds);
                }
            }
            else if (strncmp("dx", wurds[1], 2) == 0)
            {
                int sgnum = add_dxsynth(mixr);
                mixr->midi_control_destination = SYNTH;
                mixr->active_midi_soundgen_num = sgnum;
                if (num_wurds > 2)
                {
                    dxsynth *dx = (dxsynth *)mixr->sound_generators[sgnum];
                    char_melody_to_midi_melody(&dx->base, 0, wurds, 2,
                                               num_wurds);
                }
            }
            else if (strncmp("digi", wurds[1], 4) == 0)
            {
                if (strlen(wurds[2]) != 0)
                {
                    int sgnum = add_digisynth(mixr, wurds[2]);
                    mixr->midi_control_destination = SYNTH;
                    mixr->active_midi_soundgen_num = sgnum;
                    if (num_wurds > 2)
                    {
                        digisynth *ds =
                            (digisynth *)mixr->sound_generators[sgnum];
                        char_melody_to_midi_melody(&ds->base, 0, wurds, 2,
                                                   num_wurds);
                    }
                }
                else
                {
                    printf("Need to give me a sample name for a digisynth..\n");
                }
            }
            else if (strncmp("ls", wurds[1], 2) == 0)
            {
                minisynth_list_presets();
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    is_synth(mixr->sound_generators[soundgen_num]))
                {
                    // ALL THE SYNTHBASE COMMONALITY - BELOW DOES ACTIONS BASED
                    // ON WHICH KIND OF SYNTH

                    printf("SYNTHBASE STIFF?\n");

                    synthbase *base =
                        get_synthbase(mixr->sound_generators[soundgen_num]);

                    if (strncmp("add", wurds[2], 3) == 0)
                    {
                        if (strncmp("melody", wurds[3], 6) == 0 ||
                            strncmp("pattern", wurds[3], 7) == 0)
                        {
                            int new_melody_num = synthbase_add_melody(base);
                            if (num_wurds > 4)
                            {
                                char_melody_to_midi_melody(base, new_melody_num,
                                                           wurds, 4, num_wurds);
                            }
                        }
                    }
                    else if (strncmp("cp", wurds[2], 2) == 0)
                    {
                        printf("CP!\n");
                        int pattern_num = atoi(wurds[3]);
                        int sg2 = 0;
                        int pattern_num2 = 0;
                        sscanf(wurds[4], "%d:%d", &sg2, &pattern_num2);
                        printf("CP'ing %d:%d to %d:%d\n", soundgen_num,
                               pattern_num, sg2, pattern_num2);
                        if (mixer_is_valid_soundgen_num(mixr, sg2) &&
                            is_synth(mixr->sound_generators[sg2]))
                        {
                            printf("ALL VALID!\n");
                            synthbase *sb2 =
                                get_synthbase(mixr->sound_generators[sg2]);

                            if (is_valid_melody_num(base, pattern_num) &&
                                is_valid_melody_num(sb2, pattern_num2))
                            {

                                printf("Copying SYNTH "
                                       "pattern from "
                                       "%d:%d to "
                                       "%d:%d!\n",
                                       soundgen_num, pattern_num, sg2,
                                       pattern_num2);

                                midi_events_loop loop_copy;
                                synthbase_copy_midi_loop(base, pattern_num,
                                                         &loop_copy);

                                synthbase_replace_midi_loop(sb2, &loop_copy,
                                                            pattern_num2);
                            }
                        }
                    }
                    else if (strncmp("delete", wurds[2], 6) == 0)
                    {
                        if (strncmp("melody", wurds[3], 6) == 0)
                        {
                            // minisynth_delete_melody(ms);
                            // // TODO implement
                            printf("Imagine i deleted "
                                   "your melody..\n");
                        }
                        else
                        {
                            int melody = atoi(wurds[3]);
                            if (is_valid_melody_num(base, melody))
                            {
                                printf("MELODY "
                                       "DELETE "
                                       "EVENT!\n");
                            }
                            int tick = atoi(wurds[4]);
                            if (tick < PPNS)
                                synthbase_rm_micro_note(base, melody, tick);
                        }
                    }
                    else if (strncmp("dupe", wurds[2], 4) == 0)
                    {
                        int pattern_num = atoi(wurds[3]);
                        int new_pattern_num = synthbase_add_melody(base);
                        synthbase_dupe_melody(&base->melodies[pattern_num],
                                              &base->melodies[new_pattern_num]);
                    }
                    else if (strncmp("import", wurds[2], 6) == 0)
                    {
                        printf("Importing file %s\n", wurds[3]);
                        synthbase_import_midi_from_file(base, wurds[3]);
                    }
                    else if (strncmp("keys", wurds[2], 4) == 0)
                    {
                        keys(soundgen_num);
                    }
                    else if (strncmp("generate", wurds[2], 8) == 0)
                    {
                        if (strncmp("every", wurds[3], 5) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                synthbase_set_generate_mode(base, true);
                                base->generate_every_n_loops = num_gens;
                            }
                            else
                            {
                                printf("Need a number for every 'n'\n");
                            }
                        }
                        else if (strncmp("for", wurds[3], 3) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                synthbase_set_generate_mode(base, true);
                                base->max_generation = num_gens;
                            }
                            else
                            {
                                printf("Need a "
                                       "number for "
                                       "'for'\n");
                            }
                        }
                        else if (strncmp("false", wurds[3], 5) == 0)
                        {
                            printf("Disabling generate mode\n");
                            synthbase_set_generate_mode(base, false);
                        }
                        else
                        {
                            int melody_num = atoi(wurds[3]);
                            int max_notes = atoi(wurds[4]);
                            int max_steps = atoi(wurds[5]);
                            synthbase_generate_melody(base, melody_num,
                                                      max_notes, max_steps);
                        }
                    }
                    else if (strncmp("morph", wurds[2], 8) == 0)
                    {
                        if (strncmp("every", wurds[3], 5) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                synthbase_set_morph_mode(base, true);
                                base->morph_every_n_loops = num_gens;
                            }
                            else
                            {
                                printf("Need a number for every 'n'\n");
                            }
                        }
                        else if (strncmp("for", wurds[3], 3) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                synthbase_set_morph_mode(base, true);
                                base->max_generation = num_gens;
                            }
                            else
                                printf("Need a number for 'for'\n");
                        }
                        else if (strncmp("false", wurds[3], 5) == 0)
                        {
                            printf("Disabling morph mode\n");
                            synthbase_set_morph_mode(base, false);
                        }
                        else
                        {
                            printf("toggling morph mode\n");
                            bool cur_mod = base->morph_mode;
                            synthbase_set_morph_mode(base, 1 - cur_mod);
                        }
                    }
                    else if (strncmp("midi", wurds[2], 4) == 0)
                    {
                        mixr->midi_control_destination = SYNTH;
                        mixr->active_midi_soundgen_num = soundgen_num;
                    }
                    else if (strncmp("multi", wurds[2], 5) == 0)
                    {
                        if (strncmp("true", wurds[3], 4) == 0)
                        {
                            synthbase_set_multi_melody_mode(base, true);
                        }
                        else if (strncmp("false", wurds[3], 5) == 0)
                        {
                            synthbase_set_multi_melody_mode(base, false);
                        }
                        else // toggle
                        {
                            synthbase_set_multi_melody_mode(
                                base, 1 - base->multi_melody_mode);
                        }

                        printf("Synth multi mode : %s\n",
                               base->multi_melody_mode ? "true" : "false");
                    }
                    else if (strncmp("nudge", wurds[2], 5) == 0)
                    {
                        int sixteenth = atoi(wurds[3]);
                        if (sixteenth < 16)
                        {
                            printf("Nudging Melody "
                                   "along %d "
                                   "sixteenthzzzz!\n",
                                   sixteenth);
                            synthbase_nudge_melody(base, base->cur_melody,
                                                   sixteenth);
                        }
                    }
                    else if (strncmp("quantize", wurds[2], 8) == 0)
                    {
                        int melody_num = atoi(wurds[3]);
                        if (is_valid_melody_num(base, melody_num))
                        {
                            printf("QuantiZe!\n");
                            midi_melody_quantize(
                                &base->melodies[base->cur_melody]);
                        }
                    }
                    else if (strncmp("reset", wurds[2], 5) == 0)
                    {
                        if (strncmp("all", wurds[3], 3) == 0)
                        {
                            synthbase_reset_melody_all(base);
                        }
                        else
                        {
                            int melody_num = atoi(wurds[3]);
                            synthbase_reset_melody(base, melody_num);
                        }
                    }
                    else if (strncmp("switch", wurds[2], 6) == 0 ||
                             strncmp("CurMelody", wurds[2], 9) == 0)
                    {
                        int melody_num = atoi(wurds[3]);
                        synthbase_switch_melody(base, melody_num);
                    }
                    else // individual PATTERN/MELODIES manipulation
                    {
                        int melody_num = atoi(wurds[2]);
                        if (is_valid_melody_num(base, melody_num))
                        {
                            if (strncmp("numloops", wurds[3], 8) == 0)
                            {
                                int numloops = atoi(wurds[4]);
                                if (numloops != 0)
                                {
                                    synthbase_set_melody_loop_num(
                                        base, melody_num, numloops);
                                    printf("NUMLOO"
                                           "PS "
                                           "Now "
                                           "%d\n",
                                           numloops);
                                }
                            }
                            else if (strncmp("add", wurds[3], 3) == 0)
                            {
                                char_melody_to_midi_melody(base, melody_num,
                                                           wurds, 4, num_wurds);
                            }
                            else if (strncmp("mv", wurds[3], 2) == 0)
                            {
                                int fromtick = atoi(wurds[4]);
                                int totick = atoi(wurds[5]);
                                printf("MV'ing "
                                       "note\n");
                                synthbase_mv_note(base, melody_num, fromtick,
                                                  totick);
                            }
                            else if (strncmp("rm", wurds[3], 2) == 0)
                            {
                                int tick = atoi(wurds[4]);
                                printf("Rm'ing "
                                       "note\n");
                                synthbase_rm_note(base, melody_num, tick);
                            }
                            else if (strncmp("madd", wurds[3], 4) == 0)
                            { // TODO - give this support for chords - atm its
                                // just single midi note
                                int tick = 0;
                                int midi_note = 0;
                                sscanf(wurds[4], "%d:%d", &tick, &midi_note);
                                if (midi_note != 0)
                                {
                                    printf("MAddin"
                                           "g "
                                           "note"
                                           "\n");
                                    synthbase_add_micro_note(base, melody_num,
                                                             tick, midi_note);
                                }
                            }
                            else if (strncmp("melody", wurds[3], 6) == 0 ||
                                     strncmp("pattern", wurds[3], 7) == 0)
                            {
                                synthbase_reset_melody(base, melody_num);
                                char_melody_to_midi_melody(base, melody_num,
                                                           wurds, 4, num_wurds);
                            }
                            else if (strncmp("mmv", wurds[3], 2) == 0)
                            {
                                int fromtick = atoi(wurds[4]);
                                int totick = atoi(wurds[5]);
                                printf("MMV'ing "
                                       "note\n");
                                synthbase_mv_micro_note(base, melody_num,
                                                        fromtick, totick);
                            }
                            else if (strncmp("mrm", wurds[3], 3) == 0)
                            {
                                int tick = atoi(wurds[4]);
                                printf("Rm'ing "
                                       "note\n");
                                synthbase_rm_micro_note(base, melody_num, tick);
                            }
                            else if (strncmp("up", wurds[3], 2) == 0)
                            {
                                printf("UP OCTAVE!\n");
                                synthbase_change_octave_melody(base, melody_num,
                                                               1);
                            }
                            else if (strncmp("down", wurds[3], 4) == 0)
                            {
                                printf("DOWN OCTAVE!\n");
                                synthbase_change_octave_melody(base, melody_num,
                                                               0);
                            }
                        }

                        if (mixr->sound_generators[soundgen_num]->type ==
                            DXSYNTH_TYPE)
                        {
                            dxsynth *dx =
                                (dxsynth *)mixr->sound_generators[soundgen_num];
                            if (parse_dxsynth_settings_change(dx, wurds))
                            {
                                dxsynth_update(dx);
                                continue;
                            }
                        }
                        else if (mixr->sound_generators[soundgen_num]->type ==
                                 MINISYNTH_TYPE)
                        {
                            minisynth *ms =
                                (minisynth *)
                                    mixr->sound_generators[soundgen_num];
                            if (parse_minisynth_settings_change(ms, wurds))
                            {
                                minisynth_update(ms);
                                continue;
                            }
                            else if (strncmp("print", wurds[2], 5) == 0)
                            {
                                if (strncmp(wurds[3], "melodies", 8) == 0)
                                {
                                    minisynth_print_melodies(ms);
                                }
                                else if (strncmp(wurds[3], "mod", 3) == 0 ||
                                         strncmp(wurds[3], "routing", 7) == 0 ||
                                         strncmp(wurds[3], "route", 5) == 0)
                                {
                                    minisynth_print_modulation_routings(ms);
                                }
                                else
                                    minisynth_print(ms);
                            }
                            else if (strncmp("load", wurds[2], 4) == 0)
                            {
                                char preset_name[20];
                                strncpy(preset_name, wurds[3], 19);
                                minisynth_load_settings(ms, preset_name);
                            }
                            else if (strncmp("genrand", wurds[2], 4) == 0)
                            {
                                printf("GENRAND!\n");
                                minisynth_rand_settings(ms);
                                int melody_num = atoi(wurds[3]);
                                int max_notes = atoi(wurds[4]);
                                int max_steps = atoi(wurds[5]);
                                synthbase_generate_melody(&ms->base, melody_num,
                                                          max_notes, max_steps);
                            }
                            else if (strncmp("arp", wurds[2], 3) == 0)
                            {
                                ms->base.recording = false;
                                minisynth_set_arpeggiate(ms,
                                                         1 - ms->m_arp.active);
                            }
                            else if (strncmp("rand", wurds[2], 4) == 0)
                            {
                                minisynth_rand_settings(ms);
                            }
                            else if (strncmp("mono", wurds[2], 4) == 0)
                            {
                                int mono = atoi(wurds[3]);
                                minisynth_set_monophonic(ms, mono);
                            }
                            else if (strncmp("save", wurds[2], 4) == 0)
                            {
                                char preset_name[20];
                                strncpy(preset_name, wurds[3], 19);
                                minisynth_save_settings(ms, preset_name);
                            }

                            minisynth_update(ms);
                        }
                        else if (mixr->sound_generators[soundgen_num]->type ==
                                 DIGISYNTH_TYPE)
                        {
                            digisynth *ds =
                                (digisynth *)
                                    mixr->sound_generators[soundgen_num];
                            (void)ds;
                        }
                    }
                }
            }
        }

        // CHAOS COMMANDS
        else if (strncmp("chaos", wurds[0], 6) == 0)
        {

            if (strncmp("monkey", wurds[1], 6) == 0)
            {
                int soundgen_num = atoi(wurds[2]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
                {
                    add_chaosmonkey(soundgen_num);
                }
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        CHAOSMONKEY_TYPE)
                {

                    chaosmonkey *cm =
                        (chaosmonkey *)mixr->sound_generators[soundgen_num];

                    if (strncmp("wakeup", wurds[2], 7) == 0)
                    {
                        int freq_wakeup_secs = atoi(wurds[3]);
                        chaosmonkey_change_wakeup_freq(cm, freq_wakeup_secs);
                    }
                    else if (strncmp("chance", wurds[2], 7) == 0)
                    {
                        int percent_chance = atoi(wurds[3]);
                        chaosmonkey_change_chance_interrupt(cm, percent_chance);
                    }
                    else if (strncmp("suggest", wurds[2], 8) == 0)
                    {
                        if (strncmp("true", wurds[3], 5) == 0 ||
                            strncmp("false", wurds[3], 6) == 0)
                        {
                            bool val =
                                strcmp("true", wurds[4]) == 0 ? true : false;
                            chaosmonkey_suggest_mode(cm, val);
                        }
                    }
                    else if (strncmp("action", wurds[2], 7) == 0)
                    {
                        if (strncmp("true", wurds[3], 5) == 0 ||
                            strncmp("false", wurds[3], 6) == 0)
                        {
                            bool val =
                                strcmp("true", wurds[4]) == 0 ? true : false;
                            chaosmonkey_action_mode(cm, val);
                        }
                    }
                }
            }
        }
        // FX COMMANDS
        else if (strncmp("bitcrush", wurds[0], 8) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_bitcrush_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("compressor", wurds[0], 10) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_compressor_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("delay", wurds[0], 7) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                int delay_len_ms = atoi(wurds[2]);
                add_delay_soundgen(mixr->sound_generators[soundgen_num],
                                   delay_len_ms);
            }
        }
        else if (strncmp("filter", wurds[0], 6) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_basicfilter_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("follower", wurds[0], 8) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_follower_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        // else if (strncmp("granulator", wurds[0], 8) == 0 ||
        //         strncmp("gran", wurds[0], 4) == 0) {
        //    int soundgen_num = atoi(wurds[1]);
        //    if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
        //        add_granulator_soundgen(mixr->sound_generators[soundgen_num]);
        //    }
        //}
        else if (strncmp("moddelay", wurds[0], 7) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_moddelay_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("modfilter", wurds[0], 9) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_modfilter_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("reverb", wurds[0], 6) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_reverb_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("waveshape", wurds[0], 6) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_waveshape_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("distort", wurds[0], 7) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_distortion_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("decimate", wurds[0], 8) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_decimator_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("env", wurds[0], 3) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                int loop_len = atoi(wurds[2]);
                int env_type = atoi(wurds[3]);
                ENVSTREAM *e = new_envelope_stream(loop_len, env_type);
                if (e != NULL)
                {
                    add_envelope_soundgen(mixr->sound_generators[soundgen_num],
                                          e);
                }
            }
        }
        else if (strncmp("lowpass", wurds[0], 8) == 0 ||
                 strncmp("highpass", wurds[0], 9) == 0 ||
                 strncmp("bandpass", wurds[0], 9) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                int val = atoi(wurds[2]);
                if (strcmp("lowpass", wurds[0]) == 0)
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, LOWPASS);
                else if (strcmp("highpass", wurds[0]) == 0)
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, HIGHPASS);
                else
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, BANDPASS);
            }
        }

        else if (strncmp("repeat", wurds[0], 6) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                int nbeats = atoi(wurds[2]);
                int sixteenth = atoi(wurds[3]);
                add_beatrepeat_soundgen(mixr->sound_generators[soundgen_num],
                                        nbeats, sixteenth);
            }
        }

        else if (strncmp("fx", wurds[0], 2) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            int fx_num = atoi(wurds[2]);
            if (is_valid_fx_num(soundgen_num, fx_num))
            {
                fx *f = mixr->sound_generators[soundgen_num]->effects[fx_num];

                if (strncmp("on", wurds[3], 2) == 0)
                    f->enabled = true;
                else if (strncmp("off", wurds[3], 3) == 0)
                    f->enabled = false;
                else if (f->type == DELAY)
                {
                    // printf("Changing Dulay!\n");
                    stereodelay *sd = (stereodelay *)f;
                    double val = atof(wurds[4]);
                    // keep these strings in sync with status()
                    // output
                    if (strncmp("delayms", wurds[3], 7) == 0)
                        stereo_delay_set_delay_time_ms(sd, val);
                    else if (strncmp("fb", wurds[3], 2) == 0)
                        stereo_delay_set_feedback_percent(sd, val);
                    else if (strncmp("ratio", wurds[3], 5) == 0)
                        stereo_delay_set_delay_ratio(sd, val);
                    else if (strncmp("wetmx", wurds[3], 5) == 0)
                        stereo_delay_set_wet_mix(sd, val);
                    else if (strncmp("mode", wurds[3], 4) == 0)
                        stereo_delay_set_mode(sd, val);
                    else
                        printf("<bleurgh!>\n");
                }
                else if (f->type == COMPRESSOR)
                {
                    dynamics_processor *dp = (dynamics_processor *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("inputgain", wurds[3], 9) == 0)
                        dynamics_processor_set_inputgain_db(dp, val);
                    else if (strncmp("threshold", wurds[3], 9) == 0)
                        dynamics_processor_set_threshold(dp, val);
                    else if (strncmp("attackms", wurds[3], 8) == 0)
                        dynamics_processor_set_attack_ms(dp, val);
                    else if (strncmp("releasems", wurds[3], 9) == 0)
                        dynamics_processor_set_release_ms(dp, val);
                    else if (strncmp("ratio", wurds[3], 5) == 0)
                        dynamics_processor_set_ratio(dp, val);
                    else if (strncmp("outputgain", wurds[3], 10) == 0)
                        dynamics_processor_set_outputgain_db(dp, val);
                    else if (strncmp("kneewidth", wurds[3], 9) == 0)
                        dynamics_processor_set_knee_width(dp, val);
                    else if (strncmp("lookahead", wurds[3], 9) == 0)
                        dynamics_processor_set_lookahead_delay_ms(dp, val);
                    else if (strncmp("stereolink", wurds[3], 9) == 0)
                        dynamics_processor_set_stereo_link(dp, val);
                    else if (strncmp("type", wurds[3], 4) == 0)
                        dynamics_processor_set_processor_type(dp, val);
                    else if (strncmp("mode", wurds[3], 4) == 0)
                        dynamics_processor_set_time_constant(dp, val);
                    else if (strncmp("extsource", wurds[3], 9) == 0)
                        dynamics_processor_set_external_source(dp, val);
                    else if (strncmp("def", wurds[3], 3) == 0 ||
                             strncmp("default", wurds[3], 7) == 0)
                        dynamics_processor_set_default_sidechain_params(dp);
                }
                else if (f->type == REVERB)
                {
                    reverb *r = (reverb *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("predelayms", wurds[3], 10) == 0)
                        reverb_set_pre_delay_msec(r, val);
                    else if (strncmp("predelayattDb", wurds[3], 13) == 0)
                        reverb_set_pre_delay_atten_db(r, val);
                    else if (strncmp("inputLPFg", wurds[3], 9) == 0)
                        reverb_set_input_lpf_g(r, val);
                    else if (strncmp("lpf2g2", wurds[3], 6) == 0)
                        reverb_set_lpf2_g2(r, val);
                    else if (strncmp("reverbtime", wurds[3], 10) == 0)
                        reverb_set_rt60(r, val);
                    else if (strncmp("wetmx", wurds[3], 5) == 0)
                        reverb_set_wet_pct(r, val);
                    else if (strncmp("APF1delayms", wurds[3], 11) == 0)
                        reverb_set_apf_delay_msec(r, 1, val);
                    else if (strncmp("APF1g", wurds[3], 5) == 0)
                        reverb_set_apf_g(r, 1, val);
                    else if (strncmp("APF2delayms", wurds[3], 11) == 0)
                        reverb_set_apf_delay_msec(r, 2, val);
                    else if (strncmp("APF2g", wurds[3], 5) == 0)
                        reverb_set_apf_g(r, 2, val);
                    else if (strncmp("APF3delayms", wurds[3], 11) == 0)
                        reverb_set_apf_delay_msec(r, 3, val);
                    else if (strncmp("APF3g", wurds[3], 5) == 0)
                        reverb_set_apf_g(r, 3, val);
                    else if (strncmp("APF4delayms", wurds[3], 11) == 0)
                        reverb_set_apf_delay_msec(r, 4, val);
                    else if (strncmp("APF4g", wurds[3], 5) == 0)
                        reverb_set_apf_g(r, 4, val);
                    else if (strncmp("comb1delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 1, val);
                    else if (strncmp("comb2delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 2, val);
                    else if (strncmp("comb3delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 3, val);
                    else if (strncmp("comb4delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 4, val);
                    else if (strncmp("comb5delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 5, val);
                    else if (strncmp("comb6delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 6, val);
                    else if (strncmp("comb7delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 7, val);
                    else if (strncmp("comb8delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 8, val);
                }
                else if (f->type == WAVESHAPER)
                {
                    waveshaper *ws = (waveshaper *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("k_pos", wurds[3], 5) == 0)
                        waveshaper_set_arc_tan_k_pos(ws, val);
                    else if (strncmp("k_neg", wurds[3], 5) == 0)
                        waveshaper_set_arc_tan_k_neg(ws, val);
                    else if (strncmp("stages", wurds[3], 5) == 0)
                        waveshaper_set_stages(ws, val);
                    else if (strncmp("invert", wurds[3], 5) == 0)
                        waveshaper_set_invert_stages(ws, val);
                }
                else if (f->type == BITCRUSH)
                {
                    bitcrush *bc = (bitcrush *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("bitdepth", wurds[3], 8) == 0)
                        bitcrush_set_bitdepth(bc, val);
                    else if (strncmp("bitrate", wurds[3], 7) == 0)
                        bitcrush_set_bitrate(bc, val);
                    else if (strncmp("sample_hold_freq", wurds[3], 16) == 0)
                        bitcrush_set_sample_hold_freq(bc, val);
                }
                else if (f->type == BASICFILTER)
                {
                    filterpass *fp = (filterpass *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("freq", wurds[3], 4) == 0)
                        filter_set_fc_control(&fp->m_filter.f, val);
                    else if (strncmp("q", wurds[3], 4) == 0)
                        moog_set_qcontrol(&fp->m_filter.f, val);
                    else if (strncmp("type", wurds[3], 4) == 0)
                        filter_set_type(&fp->m_filter.f, val);
                    else if (strncmp("lfo1_active", wurds[3], 11) == 0)
                        filterpass_set_lfo_active(fp, 1, val);
                    else if (strncmp("lfo1_type", wurds[3], 9) == 0)
                        filterpass_set_lfo_type(fp, 1, val);
                    else if (strncmp("lfo1_amp", wurds[3], 9) == 0)
                        filterpass_set_lfo_amp(fp, 1, val);
                    else if (strncmp("lfo1_rate", wurds[3], 9) == 0)
                        filterpass_set_lfo_rate(fp, 1, val);
                    else if (strncmp("lfo2_active", wurds[3], 11) == 0)
                        filterpass_set_lfo_active(fp, 2, val);
                    else if (strncmp("lfo2_type", wurds[3], 9) == 0)
                        filterpass_set_lfo_type(fp, 2, val);
                    else if (strncmp("lfo2_amp", wurds[3], 9) == 0)
                        filterpass_set_lfo_amp(fp, 2, val);
                    else if (strncmp("lfo2_rate", wurds[3], 9) == 0)
                        filterpass_set_lfo_rate(fp, 2, val);
                }
                else if (f->type == BEATREPEAT)
                {
                    beatrepeat *br = (beatrepeat *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("numbeats", wurds[3], 8) == 0)
                        beatrepeat_change_num_beats_to_repeat(br, val);
                    else if (strncmp("sixteenth", wurds[3], 9) == 0)
                        beatrepeat_change_selected_sixteenth(br, val);
                }
                else if (f->type == DISTORTION)
                {
                    distortion *d = (distortion *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("threshold", wurds[3], 9) == 0)
                        distortion_set_threshold(d, val);
                }
                else if (f->type == ENVELOPEFOLLOWER)
                {
                    envelope_follower *ef = (envelope_follower *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("pregain", wurds[3], 4) == 0)
                        envelope_follower_set_pregain_db(ef, val);
                    else if (strncmp("threshold", wurds[3], 9) == 0)
                        envelope_follower_set_threshold(ef, val);
                    else if (strncmp("attackms", wurds[3], 8) == 0)
                        envelope_follower_set_attack_ms(ef, val);
                    else if (strncmp("releasems", wurds[3], 9) == 0)
                        envelope_follower_set_release_ms(ef, val);
                    else if (strncmp("q", wurds[3], 1) == 0)
                        envelope_follower_set_q(ef, val);
                    else if (strncmp("mode", wurds[3], 4) == 0)
                        envelope_follower_set_time_constant(ef, val);
                    else if (strncmp("dir", wurds[3], 4) == 0)
                        envelope_follower_set_direction(ef, val);
                }

                else if (f->type == MODDELAY)
                {
                    mod_delay *md = (mod_delay *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("depth", wurds[3], 5) == 0)
                    {
                        mod_delay_set_depth(md, val);
                    }
                    else if (strncmp("rate", wurds[3], 4) == 0)
                    {
                        mod_delay_set_rate(md, val);
                    }
                    else if (strncmp("fb", wurds[3], 8) == 0)
                    {
                        mod_delay_set_feedback_percent(md, val);
                    }
                    else if (strncmp("offset", wurds[3], 12) == 0)
                    {
                        mod_delay_set_chorus_offset(md, val);
                    }
                    else if (strncmp("type", wurds[3], 7) == 0)
                    {
                        mod_delay_set_mod_type(md, (unsigned int)val);
                    }
                    else if (strncmp("lfo", wurds[3], 7) == 0)
                    {
                        mod_delay_set_lfo_type(md, (unsigned int)val);
                    }
                }
                else if (f->type == MODFILTER)
                {
                    modfilter *mf = (modfilter *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("depthfc", wurds[3], 7) == 0)
                    {
                        modfilter_set_mod_depth_fc(mf, val);
                    }
                    else if (strncmp("ratefc", wurds[3], 6) == 0)
                    {
                        modfilter_set_mod_rate_fc(mf, val);
                    }
                    else if (strncmp("depthq", wurds[3], 6) == 0)
                    {
                        modfilter_set_mod_depth_q(mf, val);
                    }
                    else if (strncmp("rateq", wurds[3], 5) == 0)
                    {
                        modfilter_set_mod_rate_q(mf, val);
                    }
                    else if (strncmp("phase", wurds[3], 8) == 0)
                    {
                        modfilter_set_lfo_phase(mf, val);
                    }
                    else if (strncmp("lfo", wurds[3], 3) == 0)
                    {
                        modfilter_set_lfo_waveform(mf, val);
                    }
                }
            }
        }

        // PROGRAMMING CMDS
        else if (strncmp("var", wurds[0], 3) == 0 &&
                 strncmp("=", wurds[2], 1) == 0)
        {
            printf("Oooh! %s = %s\n", wurds[1], wurds[3]);
            update_environment(wurds[1], atoi(wurds[3]));
        }

        else if (strncmp("every", wurds[0], 5) == 0 &&
                 strncmp("loop", wurds[1], 4) == 0)
        {
            printf("Starting an algorithm - with %s!\n", cmd);
            add_algorithm(cmd);
        }

        else if (strncmp("beat", wurds[0], 4) == 0)
        {
            parse_pattern(num_wurds - 1, &wurds[1]);
        }

        // UTILS
        else if (strncmp("chord", wurds[0], 6) == 0)
        {
            chordie(wurds[1]);
        }

        else if (strncmp("strategy", wurds[0], 8) == 0)
        {
            oblique_strategy();
        }
    }
}

int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line)
{
    memset(wurds, 0, NUM_WURDS * SIZE_OF_WURD);
    int num_wurds = 0;
    char const *sep = " ";
    char *tok, *last_s;
    for (tok = strtok_r(line, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s))
    {
        strncpy(wurds[num_wurds++], tok, SIZE_OF_WURD);
        if (num_wurds == NUM_WURDS)
            break;
    }
    return num_wurds;
}

void char_array_to_seq_string_pattern(sequencer *seq, char *dest_pattern,
                                      char char_array[NUM_WURDS][SIZE_OF_WURD],
                                      int start, int end)
{
    if (strncmp("all24", char_array[start], 5) == 0)
    {
        strncat(dest_pattern,
                "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23",
                150);
    }
    else if (strncmp("all", char_array[start], 3) == 0)
    {
        if (seq->pattern_len == 24)
        {
            strncat(dest_pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 "
                                  "15 16 17 18 19 20 21 22 23",
                    150);
        }
        else
        {
            strncat(dest_pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", 127);
        }
    }
    else if (strncmp("none", char_array[start], 4) == 0)
    {
        // no-op
    }
    else
    {
        for (int i = start; i < end; i++)
        {
            strcat(dest_pattern, char_array[i]);
            if (i != (end - 1))
                strcat(dest_pattern, " ");
        }
    }
}

bool is_valid_sample_num(looper *s, int sample_num)
{
    if (sample_num < s->num_samples)
    {
        return true;
    }
    return false;
}

bool is_valid_fx_num(int soundgen_num, int fx_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
    {
        if (mixr->sound_generators[soundgen_num]->effects_num > 0 &&
            fx_num < mixr->sound_generators[soundgen_num]->effects_num)
        {
            return true;
        }
    }
    return false;
}

bool is_valid_file(char *filename)
{
    printf("IS valid? %s\n", filename);
    if (strlen(filename) == 0)
    {
        printf("That ain't no valid file where i come from..\n");
        return false;
    }
    char cwd[1024];
    getcwd(cwd, 1024);
    char *subdir = "/wavs/";
    char full_filename[strlen(cwd) + strlen(filename) + strlen(subdir)];
    strcpy(full_filename, cwd);
    strcat(full_filename, subdir);
    strcat(full_filename, filename);

    struct stat buffer;
    return (stat(full_filename, &buffer) == 0);
}

int exxit()
{
    printf(
        COOL_COLOR_GREEN
        "\nBeat it, ya val jerk...\n" ANSI_COLOR_RESET); // Thrashin' reference
    pa_teardown();
    exit(0);
}

void parse_sample_sequencer_command(sequencer *seq, char wurds[][SIZE_OF_WURD],
                                    int num_wurds, char *pattern)
{
    if (strncmp("add", wurds[2], 3) == 0)
    {
        printf("Adding\n");
        char_array_to_seq_string_pattern(seq, pattern, wurds, 3, num_wurds);
        add_char_pattern(seq, pattern);
    }
    else if (strncmp("multi", wurds[2], 5) == 0)
    {
        if (strncmp("true", wurds[3], 4) == 0)
        {
            seq_set_multi_pattern_mode(seq, true);
        }
        else if (strncmp("false", wurds[3], 5) == 0)
        {
            seq_set_multi_pattern_mode(seq, false);
        }
        printf("Sequencer multi mode : %s\n",
               seq->multi_pattern_mode ? "true" : "false");
    }
    else if (strncmp("print", wurds[2], 5) == 0)
    {
        int pattern_num = atoi(wurds[3]);
        if (seq_is_valid_pattern_num(seq, pattern_num))
        {
            printf("Printing pattern for %d\n", pattern_num);
            seq_print_pattern(seq, pattern_num);
        }
    }
    else if (strncmp("randamp", wurds[2], 6) == 0)
    {
        seq_set_randamp(seq, 1 - seq->randamp_on);
        printf("Toggling randamp to %s \n", seq->randamp_on ? "true" : "false");
    }
    else if (strncmp("sloppy", wurds[2], 6) == 0)
    {
        int sloppyjoe = atoi(wurds[3]);
        seq_set_sloppiness(seq, sloppyjoe);
    }
    else if (strncmp("generate", wurds[2], 8) == 0 ||
             strncmp("gen", wurds[2], 3) == 0)
    {
        if (strncmp("every", wurds[3], 5) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_generate_mode(seq, true);
                seq->generate_every_n_loops = num_gens;
            }
            else
            {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_generate_mode(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else
            {
                printf("Need a number for 'for'\n");
            }
        }
        else if (strncmp("source", wurds[3], 6) == 0 ||
                 strncmp("src", wurds[3], 3) == 0)
        {
            int generate_src = atoi(wurds[4]);
            if (mixer_is_valid_seq_gen_num(mixr, generate_src))
            {
                printf("Setting GENERATE SRC! %d\n", generate_src);
                seq_set_generate_src(seq, generate_src);
            }
            else
                printf("not a valid generate SRC: %d\n", generate_src);
        }
        else
        {
            seq_set_generate_mode(seq, 1 - seq->generate_mode);
        }
    }
    else if (strncmp("pattern_len", wurds[2], 10) == 0)
    {
        int len = atoi(wurds[3]);
        seq_set_pattern_len(seq, len);
    }
    else if (strncmp("visualize", wurds[2], 9) == 0)
    {
        bool b = atoi(wurds[3]);
        printf("Setting visualize to %s\n", b ? "true" : "false");
        seq->visualize = b;
    }
    else
    {
        int pattern_num = atoi(wurds[3]);
        if (seq_is_valid_pattern_num(seq, pattern_num))
        {
            if (strncmp("amp", wurds[4], 3) == 0)
            {
                int hit = atoi(wurds[5]);
                double amp = atof(wurds[6]);
                printf("Changing amp of %d:%d to %f\n", pattern_num, hit, amp);
                seq_set_sample_amp(seq, pattern_num, hit, amp);
            }
            if (strncmp("add", wurds[4], 3) == 0)
            {
                int hit = atoi(wurds[5]);
                printf("Adding a hit to %d\n", hit);
                seq_add_hit(seq, pattern_num, hit);
            }
            else if (strncmp("madd", wurds[4], 3) == 0)
            { // midi pulses
                int hit = atoi(wurds[5]);
                printf("Adding a hit to %d\n", hit);
                seq_add_micro_hit(seq, pattern_num, hit);
            }
            else if (strncmp("amp", wurds[4], 3) == 0)
            {
                char_array_to_seq_string_pattern(seq, pattern, wurds, 5,
                                                 num_wurds);
                printf("Setting pattern AMP to %s\n", pattern);
                seq_set_sample_amp_from_char_pattern(seq, pattern_num, pattern);
            }
            else if (strncmp("mv", wurds[4], 2) == 0)
            { // deals in 16th or 24th
                int hitfrom = atoi(wurds[5]);
                int hitto = atoi(wurds[6]);
                seq_mv_hit(seq, pattern_num, hitfrom, hitto);
            }
            else if (strncmp("mmv", wurds[4], 2) == 0)
            { // deals in midi pulses
                int hitfrom = atoi(wurds[5]);
                int hitto = atoi(wurds[6]);
                seq_mv_micro_hit(seq, pattern_num, hitfrom, hitto);
            }
            else if (strncmp("numloops", wurds[4], 8) == 0)
            {
                int numloops = atoi(wurds[5]);
                if (numloops != 0)
                {
                    seq_change_num_loops(seq, pattern_num, numloops);
                }
            }
            else if (strncmp("pattern", wurds[4], 7) == 0)
            {
                char_array_to_seq_string_pattern(seq, pattern, wurds, 5,
                                                 num_wurds);
                printf("Changing pattern to %s\n", pattern);
                change_char_pattern(seq, pattern_num, pattern);
            }
            else if (strncmp("rm", wurds[4], 2) == 0)
            {
                int hit = atoi(wurds[5]);
                printf("Rm'ing hit to %d\n", hit);
                seq_rm_hit(seq, pattern_num, hit);
            }
            else if (strncmp("mrm", wurds[4], 2) == 0)
            {
                int hit = atoi(wurds[5]);
                printf("Rm'ing hit to %d\n", hit);
                seq_rm_micro_hit(seq, pattern_num, hit);
            }
            else if (strncmp("swing", wurds[4], 5) == 0)
            {
                int swing_setting = atoi(wurds[5]);
                printf("changing swing to %d for pattern num %d\n",
                       swing_setting, pattern_num);
                seq_swing_pattern(seq, pattern_num, swing_setting);
            }
        }
    }
}

bool parse_dxsynth_settings_change(dxsynth *dx, char wurds[][SIZE_OF_WURD])
{
    printf("PARSLEYDX!\n");
    if (strncmp("algo", wurds[2], 4) == 0)
    {
        int algo = atoi(wurds[3]);
        dxsynth_set_voice_mode(dx, algo);
    }
    else if (strncmp("porta", wurds[2], 5) == 0)
    {
        double ms = atof(wurds[3]);
        printf("DXSynth change portamento time ms:%.2f!\n", ms);
        dxsynth_set_portamento_time_ms(dx, ms);
        return true;
    }
    else if (strncmp("pitchrange", wurds[2], 10) == 0)
    {
        int val = atoi(wurds[3]);
        printf("DXSynth change pitchrange:%d!\n", val);
        dxsynth_set_pitchbend_range(dx, val);
        return true;
    }
    else if (strncmp("vel2attack", wurds[2], 10) == 0)
    {
        bool val = atoi(wurds[3]);
        printf("DXSynth change velocity to attack!%s\n",
               val ? "true" : "false");
        dxsynth_set_velocity_to_attack_scaling(dx, val);
        return true;
    }
    else if (strncmp("note2decay", wurds[2], 10) == 0)
    {
        bool val = atoi(wurds[3]);
        printf("DXSynth change note to decay!?%s\n", val ? "true" : "false");
        dxsynth_set_note_number_to_decay_scaling(dx, val);
        return true;
    }
    else if (strncmp("dxreset2zero", wurds[2], 10) == 0)
    {
        bool val = atoi(wurds[3]);
        printf("DXSynth change reset to zero!?%s\n", val ? "true" : "false");
        dxsynth_set_reset_to_zero(dx, val);
        return true;
    }
    else if (strncmp("legato", wurds[2], 6) == 0)
    {
        bool val = atoi(wurds[3]);
        printf("DXSynth change legato!\n");
        dxsynth_set_legato_mode(dx, val);
        return true;
    }
    else if (strncmp("lfo1_intensity", wurds[2], 14) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change LFO1 intensity:%.2f!\n", val);
        dxsynth_set_lfo1_intensity(dx, val);
        return true;
    }
    else if (strncmp("lfo1_rate", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change LFO1 rate!%.2f\n", val);
        dxsynth_set_lfo1_rate(dx, val);
        return true;
    }
    else if (strncmp("lfo1_waveform", wurds[2], 13) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change LFO1 waveform:%d!\n", val);
        dxsynth_set_lfo1_waveform(dx, val);
        return true;
    }
    else if (strncmp("lfo1dest1", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        printf("DXSynth change LFO1 dest1:%d!\n", dest);
        dxsynth_set_lfo1_mod_dest(dx, 1, dest);
        return true;
    }
    else if (strncmp("lfo1dest2", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        printf("DXSynth change LFO1 dest2:%d!\n", dest);
        dxsynth_set_lfo1_mod_dest(dx, 2, dest);
        return true;
    }
    else if (strncmp("lfo1dest3", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        printf("DXSynth change LFO1 dest3:%d!\n", dest);
        dxsynth_set_lfo1_mod_dest(dx, 3, dest);
        return true;
    }
    else if (strncmp("lfo1dest4", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        printf("DXSynth change LFO1 dest4:%d!\n", dest);
        dxsynth_set_lfo1_mod_dest(dx, 4, dest);
        return true;
    }
    else if (strncmp("op1wave", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change OP1 wave:%d!\n", val);
        dxsynth_set_op_waveform(dx, 1, val);
        return true;
    }
    else if (strncmp("op1ratio", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP1 ratio:%.2f!\n", val);
        dxsynth_set_op_ratio(dx, 1, val);
        return true;
    }
    else if (strncmp("op1detune", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP1 detune:%.2f!\n", val);
        dxsynth_set_op_detune(dx, 1, val);
        return true;
    }
    else if (strncmp("eg1attackms", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG1 attack ms:%.2f!\n", val);
        dxsynth_set_eg_attack_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("eg1decayms", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG1 decay ms:%.2f!\n", val);
        dxsynth_set_eg_decay_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("eg1sustainlvl", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG1 sustain lvl:%.2f!\n", val);
        dxsynth_set_eg_sustain_lvl(dx, 1, val);
        return true;
    }
    else if (strncmp("eg1releasems", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG1 release ms:%.2f!\n", val);
        dxsynth_set_eg_release_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("op1output", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP1 output:%.2f!\n", val);
        dxsynth_set_op_output_lvl(dx, 1, val);
        return true;
    }

    else if (strncmp("op2wave", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change OP2 wave:%d!\n", val);
        dxsynth_set_op_waveform(dx, 2, val);
        return true;
    }
    else if (strncmp("op2ratio", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP2 ratio:%.2f!\n", val);
        dxsynth_set_op_ratio(dx, 2, val);
        return true;
    }
    else if (strncmp("op2detune", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP2 detune:%.2f!\n", val);
        dxsynth_set_op_detune(dx, 2, val);
        return true;
    }
    else if (strncmp("eg2attackms", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG2 attack ms:%.2f!\n", val);
        dxsynth_set_eg_attack_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("eg2decayms", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG2 decay ms:%.2f!\n", val);
        dxsynth_set_eg_decay_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("eg2sustainlvl", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG2 sustain lvl:%.2f!\n", val);
        dxsynth_set_eg_sustain_lvl(dx, 2, val);
        return true;
    }
    else if (strncmp("eg2releasems", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG2 release ms:%.2f!\n", val);
        dxsynth_set_eg_release_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("op2output", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP2 output:%.2f!\n", val);
        dxsynth_set_op_output_lvl(dx, 2, val);
        return true;
    }

    else if (strncmp("op3wave", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change OP3 wave:%d!\n", val);
        dxsynth_set_op_waveform(dx, 3, val);
        return true;
    }
    else if (strncmp("op3ratio", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP3 ratio:%.2f!\n", val);
        dxsynth_set_op_ratio(dx, 3, val);
        return true;
    }
    else if (strncmp("op3detune", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP3 detune:%.2f!\n", val);
        dxsynth_set_op_detune(dx, 3, val);
        return true;
    }
    else if (strncmp("eg3attackms", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG3 attack ms:%.2f!\n", val);
        dxsynth_set_eg_attack_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("eg3decayms", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG3 decay ms:%.2f!\n", val);
        dxsynth_set_eg_decay_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("eg3sustainlvl", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG3 sustain lvl:%.2f!\n", val);
        dxsynth_set_eg_sustain_lvl(dx, 3, val);
        return true;
    }
    else if (strncmp("eg3releasems", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG3 release ms:%.2f!\n", val);
        dxsynth_set_eg_release_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("op3output", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP3 output:%.2f!\n", val);
        dxsynth_set_op_output_lvl(dx, 3, val);
        return true;
    }

    else if (strncmp("op4wave", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change OP4 wave:%d!\n", val);
        dxsynth_set_op_waveform(dx, 4, val);
        return true;
    }
    else if (strncmp("op4ratio", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP4 ratio:%.2f!\n", val);
        dxsynth_set_op_ratio(dx, 4, val);
        return true;
    }
    else if (strncmp("op4detune", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP4 detune:%.2f!\n", val);
        dxsynth_set_op_detune(dx, 4, val);
        return true;
    }
    else if (strncmp("eg4attackms", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG4 attack ms:%.2f!\n", val);
        dxsynth_set_eg_attack_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("eg4decayms", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG4 decay ms:%.2f!\n", val);
        dxsynth_set_eg_decay_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("eg4sustainlvl", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG4 sustain lvl:%.2f!\n", val);
        dxsynth_set_eg_sustain_lvl(dx, 4, val);
        return true;
    }
    else if (strncmp("eg4releasems", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG4 release ms:%.2f!\n", val);
        dxsynth_set_eg_release_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("op4output", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP4 output:%.2f!\n", val);
        dxsynth_set_op_output_lvl(dx, 4, val);
        return true;
    }
    else if (strncmp("op4feedback", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP4 feedback:%.2f!\n", val);
        dxsynth_set_op4_feedback(dx, val);
        return true;
    }

    return false;
}

bool parse_minisynth_settings_change(minisynth *ms, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("attackms", wurds[2], 8) == 0)
    {
        printf("Minisynth change Attack Time Ms!\n");
        double val = atof(wurds[3]);
        minisynth_set_attack_time_ms(ms, val);
        return true;
    }
    else if (strncmp("decayms", wurds[2], 7) == 0)
    {
        printf("Minisynth change Decay Time MS!\n");
        double val = atof(wurds[3]);
        minisynth_set_decay_time_ms(ms, val);
        return true;
    }
    else if (strncmp("gen", wurds[2], 4) == 0)
    {
        if (strncmp(wurds[3], "source", 6) == 0 ||
            strncmp(wurds[3], "src", 3) == 0)
        {
            int src = atoi(wurds[4]);
            printf("Changing BITSHIFT SRC: %d\n", src);
            minisynth_set_bitshift_src(ms, src);
        }
        else
        {
            bool b = atof(wurds[3]);
            printf("Minisynth BITSHIFT %s!\n", b ? "true" : "false");
            minisynth_set_bitshift(ms, b);
        }
        return true;
    }
    else if (strncmp("releasems", wurds[2], 7) == 0)
    {
        printf("Minisynth change Release Time MS!\n");
        double val = atof(wurds[3]);
        minisynth_set_release_time_ms(ms, val);
        return true;
    }
    else if (strncmp("detune", wurds[2], 6) == 0)
    {
        printf("Minisynth change DETUNE!\n");
        double val = atof(wurds[3]);
        minisynth_set_detune(ms, val);
        return true;
    }
    else if (strncmp("eg1_osc_enabled", wurds[2], 9) == 0)
    {
        int val = atoi(wurds[3]);
        printf("Minisynth EG1 Osc Enable? %d!\n", val);
        minisynth_set_eg1_osc_enable(ms, val);
        return true;
    }
    else if (strncmp("eg1_osc_intensity", wurds[2], 9) == 0)
    {
        printf("Minisynth change EG1 Osc Int!\n");
        double val = atof(wurds[3]);
        minisynth_set_eg1_osc_int(ms, val);
        return true;
    }
    else if (strncmp("eg1_dca_enabled", wurds[2], 14) == 0)
    {
        int val = atoi(wurds[3]);
        printf("Minisynth EG1 DCA Enable? %d!\n", val);
        minisynth_set_eg1_dca_enable(ms, val);
        return true;
    }
    else if (strncmp("eg1_dca_intensity", wurds[2], 9) == 0)
    {
        printf("Minisynth change EG1 DCA Int!\n");
        double val = atof(wurds[3]);
        minisynth_set_eg1_dca_int(ms, val);
        return true;
    }
    else if (strncmp("eg1_filter_enabled", wurds[2], 14) == 0)
    {
        int val = atoi(wurds[3]);
        printf("Minisynth EG1 Filter Enable? %d!\n", val);
        minisynth_set_eg1_filter_enable(ms, val);
        return true;
    }
    else if (strncmp("eg1_filter_intensity", wurds[2], 12) == 0)
    {
        printf("Minisynth change EG1 Filter Int!\n");
        double val = atof(wurds[3]);
        minisynth_set_eg1_filter_int(ms, val);
        return true;
    }
    else if (strncmp("fc", wurds[2], 2) == 0)
    {
        printf("Minisynth change Filter Cutoff!\n");
        double val = atof(wurds[3]);
        minisynth_set_filter_fc(ms, val);
        return true;
    }
    else if (strncmp("fq", wurds[2], 2) == 0)
    {
        printf("Minisynth change Filter Qualivity!\n");
        double val = atof(wurds[3]);
        minisynth_set_filter_fq(ms, val);
        return true;
    }
    else if (strncmp("filtertype", wurds[2], 4) == 0)
    {
        printf("Minisynth change Filter TYPE!\n");
        int val = atoi(wurds[3]);
        minisynth_set_filter_type(ms, val);
        return true;
    }
    else if (strncmp("saturation", wurds[2], 10) == 0)
    {
        printf("Minisynth change Filter SATURATION!\n");
        double val = atof(wurds[3]);
        minisynth_set_filter_saturation(ms, val);
        return true;
    }
    else if (strncmp("nlp", wurds[2], 3) == 0)
    {
        printf("Minisynth change Filter NLP!\n");
        unsigned val = atoi(wurds[3]);
        minisynth_set_filter_nlp(ms, val);
        return true;
    }
    else if (strncmp("ktint", wurds[2], 5) == 0)
    {
        printf("Minisynth change Filter Keytrack Intensity!\n");
        double val = atof(wurds[3]);
        minisynth_set_keytrack_int(ms, val);
        return true;
    }
    else if (strncmp("kt", wurds[2], 2) == 0)
    {
        printf("Minisynth change Filter Keytrack!\n");
        int val = atoi(wurds[3]);
        minisynth_set_keytrack(ms, val);
        return true;
    }
    else if (strncmp("legato", wurds[2], 6) == 0)
    {
        printf("Minisynth change LEGATO!\n");
        int val = atoi(wurds[3]);
        minisynth_set_legato_mode(ms, val);
        return true;
    }
    else if (strncmp("lfo1wave", wurds[2], 7) == 0)
    {
        printf("Minisynth change LFO1 Wave!\n");
        int val = atoi(wurds[3]);
        minisynth_set_lfo_wave(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1mode", wurds[2], 7) == 0)
    {
        printf("Minisynth change LFO1 MODE!\n");
        int val = atoi(wurds[3]);
        minisynth_set_lfo_mode(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1rate", wurds[2], 8) == 0)
    {
        printf("Minisynth change LFO1 rate!\n");
        double val = atof(wurds[3]);
        minisynth_set_lfo_rate(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1amp", wurds[2], 7) == 0)
    {
        printf("Minisynth change LFO1 AMP!\n");
        double val = atof(wurds[3]);
        minisynth_set_lfo_amp(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1_osc_enabled", wurds[2], 16) == 0)
    {
        int val = atoi(wurds[3]);
        printf("LFO1->OSC routing enable? %d!\n", val);
        minisynth_set_lfo_osc_enable(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1_osc_intensity", wurds[2], 16) == 0)
    {
        double val = atof(wurds[3]);
        printf("LFO1->OSC intensity %f!\n", val);
        minisynth_set_lfo_osc_int(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1_filter_enabled", wurds[2], 16) == 0)
    {
        int val = atoi(wurds[3]);
        printf("LFO1->FILTER routing enable? %d!\n", val);
        minisynth_set_lfo_filter_enable(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1_filter_intensity", wurds[2], 21) == 0)
    {
        double val = atof(wurds[3]);
        printf("LFO1->FILTER intensity %f!\n", val);
        minisynth_set_lfo_filter_fc_int(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1_amp_enabled", wurds[2], 16) == 0)
    {
        int val = atoi(wurds[3]);
        printf("LFO1->AMP routing enable? %d!\n", val);
        minisynth_set_lfo_amp_enable(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1_amp_intensity", wurds[2], 16) == 0)
    {
        double val = atof(wurds[3]);
        printf("LFO1->AMP intensity %f!\n", val);
        minisynth_set_lfo_amp_int(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1_pan_enabled", wurds[2], 16) == 0)
    {
        int val = atoi(wurds[3]);
        printf("LFO1->PAN routing enable? %d!\n", val);
        minisynth_set_lfo_pan_enable(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1_pan_intensity", wurds[2], 10) == 0)
    {
        printf("Minisynth change LFO1 Pan Int!\n");
        double val = atof(wurds[3]);
        minisynth_set_lfo_pan_int(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo2wave", wurds[2], 7) == 0)
    {
        printf("Minisynth change lfo2 Wave!\n");
        int val = atoi(wurds[3]);
        minisynth_set_lfo_wave(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2mode", wurds[2], 7) == 0)
    {
        printf("Minisynth change lfo2 MODE!\n");
        int val = atoi(wurds[3]);
        minisynth_set_lfo_mode(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2rate", wurds[2], 8) == 0)
    {
        printf("Minisynth change lfo2 rate!\n");
        double val = atof(wurds[3]);
        minisynth_set_lfo_rate(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2amp", wurds[2], 7) == 0)
    {
        printf("Minisynth change lfo2 AMP!\n");
        double val = atof(wurds[3]);
        minisynth_set_lfo_amp(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2_osc_enabled", wurds[2], 16) == 0)
    {
        int val = atoi(wurds[3]);
        printf("lfo2->OSC routing enable? %d!\n", val);
        minisynth_set_lfo_osc_enable(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2_osc_intensity", wurds[2], 16) == 0)
    {
        double val = atof(wurds[3]);
        printf("lfo2->OSC intensity %f!\n", val);
        minisynth_set_lfo_osc_int(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2_filter_enabled", wurds[2], 16) == 0)
    {
        int val = atoi(wurds[3]);
        printf("lfo2->FILTER routing enable? %d!\n", val);
        minisynth_set_lfo_filter_enable(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2_filter_intensity", wurds[2], 21) == 0)
    {
        double val = atof(wurds[3]);
        printf("lfo2->FILTER intensity %f!\n", val);
        minisynth_set_lfo_filter_fc_int(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2_amp_enabled", wurds[2], 16) == 0)
    {
        int val = atoi(wurds[3]);
        printf("lfo2->AMP routing enable? %d!\n", val);
        minisynth_set_lfo_amp_enable(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2_amp_intensity", wurds[2], 16) == 0)
    {
        double val = atof(wurds[3]);
        printf("lfo2->AMP intensity %f!\n", val);
        minisynth_set_lfo_amp_int(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2_pan_enabled", wurds[2], 16) == 0)
    {
        int val = atoi(wurds[3]);
        printf("lfo2->PAN routing enable? %d!\n", val);
        minisynth_set_lfo_pan_enable(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2_pan_intensity", wurds[2], 10) == 0)
    {
        printf("Minisynth change lfo2 Pan Int!\n");
        double val = atof(wurds[3]);
        minisynth_set_lfo_pan_int(ms, 2, val);
        return true;
    }
    else if (strncmp("ndscale", wurds[2], 7) == 0)
    {
        printf("Minisynth change Note Number to Decay Scaling!\n");
        int val = atoi(wurds[3]);
        minisynth_set_note_to_decay_scaling(ms, val);
        return true;
    }
    else if (strncmp("noisedb", wurds[2], 7) == 0)
    {
        printf("Minisynth change Noise Osc DB!\n");
        double val = atof(wurds[3]);
        minisynth_set_noise_osc_db(ms, val);
        return true;
    }
    else if (strncmp("oct", wurds[2], 3) == 0)
    {
        printf("Minisynth change OCTAVE!\n");
        int val = atoi(wurds[3]);
        minisynth_set_octave(ms, val);
        return true;
    }
    else if (strncmp("pitchrange", wurds[2], 10) == 0)
    {
        printf("Minisynth change Pitchbend RANGE!\n");
        int val = atoi(wurds[3]);
        minisynth_set_pitchbend_range(ms, val);
        return true;
    }
    else if (strncmp("porta", wurds[2], 5) == 0)
    {
        printf("Minisynth change PORTAMENTO Time!\n");
        double val = atof(wurds[3]);
        minisynth_set_portamento_time_ms(ms, val);
        return true;
    }
    else if (strncmp("pw", wurds[2], 2) == 0)
    {
        printf("Minisynth change PULSEWIDTH Pct!\n");
        double val = atof(wurds[3]);
        minisynth_set_pulsewidth_pct(ms, val);
        return true;
    }
    else if (strncmp("subosc", wurds[2], 6) == 0)
    {
        printf("Minisynth change SubOSC DB!\n");
        double val = atof(wurds[3]);
        minisynth_set_sub_osc_db(ms, val);
        return true;
    }
    else if (strncmp("sustainlvl", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("Minisynth change Sustain Level! %.2f\n", val);
        minisynth_set_sustain(ms, val);
        return true;
    }
    else if (strncmp("sustainms", wurds[2], 9) == 0)
    {
        printf("Minisynth change Sustain Time ms!\n");
        double val = atof(wurds[3]);
        minisynth_set_sustain_time_ms(ms, val);
        return true;
    }
    else if (strncmp("sustain16th", wurds[2], 11) == 0)
    {
        printf("Minisynth change Sustain Time 16th!\n");
        double val = atof(wurds[3]);
        minisynth_set_sustain_time_sixteenth(ms, val);
        return true;
    }
    else if (strncmp("sustain", wurds[2], 7) == 0)
    {
        printf("Minisynth change SUSTAIN OVERRIDE!\n");
        int val = atoi(wurds[3]);
        minisynth_set_sustain_override(ms, val);
        return true;
    }
    else if (strncmp("vascale", wurds[2], 7) == 0)
    {
        printf("Minisynth change Velocity to Attack Scaling!\n");
        int val = atoi(wurds[3]);
        minisynth_set_velocity_to_attack_scaling(ms, val);
        return true;
    }
    else if (strncmp("voice", wurds[2], 5) == 0)
    {
        printf("Minisynth change VOICE!\n");
        int val = atoi(wurds[3]);
        minisynth_set_voice_mode(ms, val);
        return true;
    }
    else if (strncmp("vol", wurds[2], 3) == 0)
    {
        printf("Minisynth change VOLUME!\n");
        double val = atof(wurds[3]);
        minisynth_set_vol(ms, val);
        return true;
    }
    else if (strncmp("zero", wurds[2], 4) == 0)
    {
        printf("Minisynth change REST-To-ZERO!\n");
        int val = atoi(wurds[3]);
        minisynth_set_reset_to_zero(ms, val);
        return true;
    }
    else if (strncmp("arplatch", wurds[2], 8) == 0)
    {
        printf("Minisynth change ARP Latch!\n");
        int val = atoi(wurds[3]);
        if (val == 0 || val == 1)
            minisynth_set_arpeggiate_latch(ms, val);
        else
            printf("Gimme a 0 or 1\n");
        return true;
    }
    else if (strncmp("arprepeat", wurds[2], 9) == 0)
    {
        printf("Minisynth change ARP Repeat!\n");
        int val = atoi(wurds[3]);
        if (val == 0 || val == 1)
            minisynth_set_arpeggiate_single_note_repeat(ms, val);
        else
            printf("Gimme a 0 or 1\n");
        return true;
    }
    else if (strncmp("arpoctrange", wurds[2], 11) == 0)
    {
        printf("Minisynth change ARP Oct Range!\n");
        int val = atoi(wurds[3]);
        minisynth_set_arpeggiate_octave_range(ms, val);
        return true;
    }
    else if (strncmp("arpmode", wurds[2], 7) == 0)
    {
        printf("Minisynth change ARP Mode!\n");
        int val = atoi(wurds[3]);
        minisynth_set_arpeggiate_mode(ms, val);
        return true;
    }
    else if (strncmp("arprate", wurds[2], 7) == 0)
    {
        printf("Minisynth change ARP Rate!\n");
        int val = atoi(wurds[3]);
        minisynth_set_arpeggiate_rate(ms, val);
        return true;
    }
    else if (strncmp("arpcurstep", wurds[2], 11) == 0)
    {
        printf("you don't change Minisynth curstep - it increments by "
               "itself!\n");
        return true;
    }
    else if (strncmp("arp", wurds[2], 3) == 0)
    {
        printf("Minisynth change ARP!\n");
        int val = atoi(wurds[3]);
        if (val == 0 || val == 1)
            minisynth_set_arpeggiate(ms, val);
        else
            printf("Gimme a 1 or 0\n");
        return true;
    }
    return false;
}

void char_melody_to_midi_melody(synthbase *base, int dest_melody,
                                char char_array[NUM_WURDS][SIZE_OF_WURD],
                                int start, int end)
{
    for (int i = start; i < end; i++)
    {
        int tick = 0;
        int midi_note = 0;
        chord_midi_notes chnotes = {0, 0, 0};
        if (extract_chord_from_char_notation(char_array[i], &tick, &chnotes))
        {
            synthbase_add_note(base, dest_melody, tick, chnotes.root);
            synthbase_add_note(base, dest_melody, tick, chnotes.third);
            synthbase_add_note(base, dest_melody, tick, chnotes.fifth);
        }
        else
        {
            sscanf(char_array[i], "%d:%d", &tick, &midi_note);
            if (midi_note != 0)
            {
                printf("Adding %d:%d\n", tick, midi_note);
                synthbase_add_note(base, dest_melody, tick, midi_note);
            }
        }
    }
}

bool extract_chord_from_char_notation(char *wurd, int *tick,
                                      chord_midi_notes *chnotes)
{
    char char_note[3] = {0};
    sscanf(wurd, "%d:%s", tick, char_note);

    if (strncasecmp(char_note, "c", 1) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("C_MAJOR");
        return true;
    }
    else if (strncasecmp(char_note, "dm", 2) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("D_MINOR");
        return true;
    }
    else if (strncasecmp(char_note, "d", 1) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("D_MAJOR");
        return true;
    }
    else if (strncasecmp(char_note, "em", 2) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("E_MINOR");
        return true;
    }
    else if (strncasecmp(char_note, "e", 1) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("E_MAJOR");
        return true;
    }
    else if (strncasecmp(char_note, "f", 1) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("F_MAJOR");
        return true;
    }
    else if (strncasecmp(char_note, "gm", 2) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("G_MINOR");
        return true;
    }
    else if (strncasecmp(char_note, "g", 1) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("G_MAJOR");
        return true;
    }
    else if (strncasecmp(char_note, "am", 2) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("A_MINOR");
        return true;
    }
    else if (strncasecmp(char_note, "a", 1) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("A_MAJOR");
        return true;
    }
    else if (strncasecmp(char_note, "bm", 2) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("B_MINOR");
        return true;
    }
    else if (strncasecmp(char_note, "b", 1) == 0)
    {
        *chnotes = get_midi_notes_from_char_chord("B_MAJOR");
        return true;
    }

    return false;
}
