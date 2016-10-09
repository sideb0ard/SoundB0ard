#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "audioutils.h"
#include "cmdloop.h"
#include "defjams.h"
#include "envelope.h"
#include "help.h"
#include "keys.h"
#include "midimaaan.h"
#include "mixer.h"
#include "nanosynth.h"
#include "sampler.h"
#include "table.h"
#include "utils.h"

extern mixer *mixr;

extern wtable *wave_tables[5];

void loopy(void)
{
    char *line;
    while ((line = readline(ANSI_COLOR_MAGENTA "SB#> " ANSI_COLOR_RESET)) !=
           NULL) {
        if (line[0] != 0) {
            add_history(line);
            interpret(line);
        }
    }
}

void ps() { mixer_ps(mixr); }

int exxit()
{
    printf(COOL_COLOR_GREEN "\nBeat it, ya val jerk...\n" ANSI_COLOR_RESET);
    pa_teardown();
    exit(0);
}

void interpret(char *line)
{
    char *tok, *last_s;
    char *sep = ",";
    for (tok = strtok_r(line, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s)) {
        char *trim_tok = calloc(strlen(tok), sizeof(char));
        strim(tok, trim_tok);

        if (strcmp(trim_tok, "ps") == 0) {
            ps();
            return;
        }
        else if (strcmp(trim_tok, "nanosynth") == 0 ||
                 strcmp(trim_tok, "ns") == 0) {
            add_nanosynth(mixr);
        }
        else if (strcmp(trim_tok, "midi") == 0) {
            //// run the MIDI event looprrr...
            pthread_t midi_th;
            if (pthread_create(&midi_th, NULL, midiman, NULL)) {
                fprintf(stderr, "Errrr, wit tha midi..\n");
                return;
            }
            pthread_detach(midi_th);
        }
        else if (strcmp(trim_tok, "ls") == 0) {
            list_sample_dir();
        }
        else if (strcmp(trim_tok, "delay") == 0) {
            delay_toggle(mixr);
        }
        else if (strcmp(trim_tok, "exit") == 0) {
            exxit();
        }
        else if (strcmp(trim_tok, "help") == 0) {
            print_help();
        }

        // SIGNAL START
        regmatch_t ns_match[3];
        regex_t ns_cmdtype_rx;
        regcomp(&ns_cmdtype_rx,
                "^(bitwize|sine|sawd|sawu|tri|square) ([[:digit:].]+)$",
                REG_EXTENDED | REG_ICASE);
        if (regexec(&ns_cmdtype_rx, trim_tok, 3, ns_match, 0) == 0) {

            float val = 0;
            char cmd[20];
            SBMSG *msg = new_sbmsg();
            sscanf(trim_tok, "%s %f", cmd, &val);
            msg->freq = val;

            strncpy(msg->cmd, "timed_sig_start", 19);
            printf("PARAMZZZ %s", cmd);
            strncpy(msg->params, cmd, 10);
            thrunner(msg);
        }

        // TODO: move regex outside function to compile once
        regmatch_t pmatch[3];
        regex_t cmdtype_rx;
        regcomp(&cmdtype_rx,
                "^(bpm|duck|keys|midi|solo|stop|up|vol) ([[:digit:].]+)$",
                REG_EXTENDED | REG_ICASE);

        if (regexec(&cmdtype_rx, trim_tok, 3, pmatch, 0) == 0) {

            float val = 0;
            char cmd[20];
            SBMSG *msg = new_sbmsg();

            sscanf(trim_tok, "%s %f", cmd, &val);
            msg->freq = val;

            int is_val_a_valid_sig_num =
                (val >= 0 && val < mixr->soundgen_num) ? 1 : 0;

            if (strcmp(cmd, "vol") == 0) {
                if (val >= 0 && val <= 1) {
                    mixr->volume = val;
                }
            }

            if (strcmp(cmd, "bpm") == 0) {
                mixer_update_bpm(mixr, val);
                for (int i = 0; i < mixr->soundgen_num; i++) {
                    for (int j = 0;
                         j < mixr->sound_generators[i]->envelopes_num; j++) {
                        update_envelope_stream_bpm(
                            mixr->sound_generators[i]->envelopes[j]);
                    }
                    if (mixr->sound_generators[i]->type == SAMPLER_TYPE) {
                        sampler_set_incr(mixr->sound_generators[i]);
                    }
                }
            }
            else if (is_val_a_valid_sig_num) {

                if (strcmp(cmd, "keys") == 0) {
                    if (mixr->sound_generators[(int)val]->type ==
                        NANOSYNTH_TYPE) {
                        printf("KEYS!\n");
                        keys(val);
                    }
                    else {
                        printf("Can only run keys() on an nanosynth Type, ya "
                               "dafty\n");
                    }
                }
                else if (strcmp(cmd, "midi") == 0) {
                    if (mixr->sound_generators[(int)val]->type ==
                        NANOSYNTH_TYPE) {
                        printf("MIDI fer %d\n", (int)val);
                        mixr->has_active_nanosynth = 1;
                        mixr->active_nanosynth_soundgen_num = val;
                    }
                    else {
                        printf("Can only run keys() on an nanosynth Type, ya "
                               "dafty\n");
                    }
                }
                else if (strcmp(cmd, "stop") == 0) {
                    msg->sound_gen_num = val;
                    strncpy(msg->cmd, "fadedownrrr", 19);
                    thrunner(msg);
                }
                else if (strcmp(cmd, "up") == 0) {
                    msg->sound_gen_num = val;
                    strncpy(msg->cmd, "fadeuprrr", 19);
                    thrunner(msg);
                }
                else if (strcmp(cmd, "duck") == 0) {
                    msg->sound_gen_num = val;
                    strncpy(msg->cmd, "duckrrr", 19);
                    thrunner(msg);
                }
                else if (strcmp(cmd, "solo") == 0) {
                    for (int i = 0; i < mixr->soundgen_num; i++) {
                        if (i != val) {
                            SBMSG *msg = new_sbmsg();
                            msg->sound_gen_num = i;
                            printf("Duckin' %d\n", i);
                            strncpy(msg->cmd, "duckrrr", 19);
                            thrunner(msg);
                        }
                    }
                }
            }
        }

        regmatch_t sxmatch[3];
        regex_t sx_rx;
        regcomp(&sx_rx, "^(distort|decimate|life|rec|env) ([[:digit:].]+)$",
                REG_EXTENDED | REG_ICASE);
        if (regexec(&sx_rx, trim_tok, 2, sxmatch, 0) == 0) {
            double val = 0;
            char cmd_type[10];
            sscanf(trim_tok, "%s %lf", cmd_type, &val);
            int is_val_a_valid_sig_num =
                (val >= 0 && val < mixr->soundgen_num) ? 1 : 0;
            if (is_val_a_valid_sig_num) {
                if (strncmp(cmd_type, "distort", 10) == 0) {
                    printf("DISTORTTTTTTT!\n");
                    add_distortion_soundgen(mixr->sound_generators[(int)val]);
                }
                else if ((strncmp(cmd_type, "rec", 4) == 0)) {
                    if (mixr->sound_generators[(int)val]->type !=
                        NANOSYNTH_TYPE) {
                        printf("Beat it, ya chancer\n");
                    }
                    else {
                        printf("Toggling REC for %d\n", (int)val);
                        nanosynth *ns =
                            (nanosynth *)mixr->sound_generators[(int)val];
                        ns->recording = 1 - ns->recording;
                    }
                }
                else if ((strncmp(cmd_type, "life", 5) == 0)) {
                    if (mixr->sound_generators[(int)val]->type != DRUM_TYPE) {
                        printf("Beat it, ya chancer\n");
                    }
                    else {
                        printf("Toggling LIFE for %d\n", (int)val);
                        DRUM *d = (DRUM *)mixr->sound_generators[(int)val];
                        d->game_of_life_on = 1 - d->game_of_life_on;
                        // printf("LIFE now %d\n", d->game_of_life_on);
                        // ps();
                    }
                }
                else if ((strncmp(cmd_type, "env", 4) == 0)) {
                    printf("Toggling ENV for %d\n", (int)val);
                    mixr->sound_generators[(int)val]->envelopes_enabled =
                        1 - mixr->sound_generators[(int)val]->envelopes_enabled;
                }
                else {
                    printf("DECIMATE!\n");
                    add_decimator_soundgen(mixr->sound_generators[(int)val]);
                }
            }
            else {
                printf("Cmonbuddy, playdagem, eh? Only valid signal nums "
                       "allowed\n");
            }
        }

        regmatch_t sc_match[5];
        regex_t sc_rx;
        regcomp(&sc_rx,
                "^(sidechain) ([[:digit:]]) ([[:digit:]]) ([[:digit:]]+)$",
                REG_EXTENDED | REG_ICASE);
        if (regexec(&sc_rx, trim_tok, 4, sc_match, 0) == 0) {
            int val1, val2, val3;
            char cmd_type[10];
            sscanf(trim_tok, "%s %d %d %d", cmd_type, &val1, &val2, &val3);
            printf("SIDECHAIN! %d %d %d\n", val1, val2, val3);
            if (mixr->sound_generators[val2]->type == DRUM_TYPE) {
                DRUM *d = (DRUM *)mixr->sound_generators[val2];
                int pat_array[16];
                int_pattern_to_array(d->patterns[d->cur_pattern_num],
                                     pat_array);
                for (int i = 0; i < 16; i++) {
                    printf("%d ", pat_array[i]);
                }
                ENVSTREAM *e = new_sidechain_stream(pat_array, val3);
                add_envelope_soundgen(mixr->sound_generators[(int)val1], e);
            }
        }

        regmatch_t tpmatch[4];
        regex_t tsigtype_rx;
        regcomp(&tsigtype_rx,
                "^(vol|freq|delay1|delay2|reverb|res|randd|allpass|"
                "lowpass|highpass|bandpass|nanosynth|sustain|swing) "
                "([[:digit:].]+) ([[:digit:].]+)$",
                REG_EXTENDED | REG_ICASE);
        if (regexec(&tsigtype_rx, trim_tok, 3, tpmatch, 0) == 0) {
            double val1 = 0;
            double val2 = 0;
            char cmd_type[10];
            sscanf(trim_tok, "%s %lf %lf", cmd_type, &val1, &val2);

            int is_val_a_valid_sig_num =
                (val1 >= 0 && val1 < mixr->soundgen_num) ? 1 : 0;

            if (is_val_a_valid_sig_num) {

                if (strcmp(cmd_type, "vol") == 0) {
                    vol_change(mixr, val1, val2);
                    printf("VOL! %s %f %lf\n", cmd_type, val1, val2);
                }
                if (strcmp(cmd_type, "sustain") == 0) {
                    printf("SUSTAIN! %s %lf %lf\n", cmd_type, val1, val2);
                    if (mixr->sound_generators[(int)val1]->type ==
                        NANOSYNTH_TYPE) {
                        nanosynth *ns =
                            (nanosynth *)mixr->sound_generators[(int)val1];
                        nanosynth_set_sustain(ns, val2);
                    }
                }
                if (strcmp(cmd_type, "delay1") == 0) {
                    printf("DELAY1 CALLED FOR! %s %.lf %.lf\n", cmd_type, val1,
                           val2);
                    add_delay_soundgen(mixr->sound_generators[(int)val1], val2,
                                       DELAY1);
                }
                if (strcmp(cmd_type, "delay2") == 0) {
                    printf("DELAY2 CALLED FOR! %s %.lf %.lf\n", cmd_type, val1,
                           val2);
                    add_delay_soundgen(mixr->sound_generators[(int)val1], val2,
                                       DELAY2);
                }
                if (strcmp(cmd_type, "res") == 0) {
                    printf("RESONATOR CALLED FOR! %s %.lf %.lf\n", cmd_type,
                           val1, val2);
                    add_delay_soundgen(mixr->sound_generators[(int)val1], val2,
                                       RES);
                }
                if (strcmp(cmd_type, "reverb") == 0) {
                    printf("REVERB CALLED FOR! %s %.lf %.lf\n", cmd_type, val1,
                           val2);
                    add_delay_soundgen(mixr->sound_generators[(int)val1], val2,
                                       REVERB);
                }
                if (strcmp(cmd_type, "swing") == 0) {
                    if (mixr->sound_generators[(int)val1]->type != DRUM_TYPE) {
                        printf("Beat it, ya chancer\n");
                    }
                    else {
                        printf("SWING CALLED FOR! %s %.lf %.lf\n", cmd_type,
                               val1, val2);
                        swingrrr(mixr->sound_generators[(int)val1], val2);
                    }
                }
                if (strcmp(cmd_type, "allpass") == 0) {
                    printf("ALLPASS CALLED FOR! %s %.lf %.lf\n", cmd_type, val1,
                           val2);
                    add_delay_soundgen(mixr->sound_generators[(int)val1], val2,
                                       ALLPASS);
                }
                if (strcmp(cmd_type, "lowpass") == 0) {
                    printf("LOWPASS CALLED FOR! %s %.lf %.lf\n", cmd_type, val1,
                           val2);
                    add_freq_pass_soundgen(mixr->sound_generators[(int)val1],
                                           val2, LOWPASS);
                }
                if (strcmp(cmd_type, "highpass") == 0) {
                    printf("HIGHPASS CALLED FOR! %s %.lf %.lf\n", cmd_type,
                           val1, val2);
                    add_freq_pass_soundgen(mixr->sound_generators[(int)val1],
                                           val2, HIGHPASS);
                }
                if (strcmp(cmd_type, "bandpass") == 0) {
                    printf("BANDPASS CALLED FOR! %s %.lf %.lf\n", cmd_type,
                           val1, val2);
                    add_freq_pass_soundgen(mixr->sound_generators[(int)val1],
                                           val2, BANDPASS);
                }
                if (strcmp(cmd_type, "randd") == 0) {
                    printf("RANDY!\n");
                    SBMSG *msg = new_sbmsg();
                    strncpy(msg->cmd, "randdrum", 9);
                    msg->sound_gen_num = val1;
                    msg->looplen = val2;
                    thrunner(msg);
                }
            }
            else {
                printf("Oofft mate, you don't have enough sound_gens for "
                       "that..\n");
            }
        }

        // chord info // TODO - simplify - don't need a regex here but whatevs
        regmatch_t chmatch[2];
        regex_t chord_rx;
        regcomp(&chord_rx, "^chord ([[:alpha:]]+)$", REG_EXTENDED | REG_ICASE);
        if (regexec(&chord_rx, trim_tok, 2, chmatch, 0) == 0) {
            int note_len = chmatch[1].rm_eo - chmatch[1].rm_so;
            char note[note_len + 1];
            strncpy(note, trim_tok + chmatch[1].rm_so, note_len);
            note[note_len] = '\0';
            chordie(note);
        }

        // drum sample play
        regmatch_t fmatch[4];
        regex_t file_rx;
        regcomp(
            &file_rx,
            "^(play|addd) ([.[:alnum:]]+) ((all|none|[[:digit:][:space:]]+))$",
            REG_EXTENDED | REG_ICASE);
        if (regexec(&file_rx, trim_tok, 4, fmatch, 0) == 0) {

            char cmd_type[10];
            sscanf(trim_tok, "%s", cmd_type);

            int pattern_len = fmatch[3].rm_eo - fmatch[3].rm_so;
            char tmp_pattern[pattern_len + 1];
            strncpy(tmp_pattern, trim_tok + fmatch[3].rm_so, pattern_len);
            tmp_pattern[pattern_len] = '\0';

            char pattern[38];
            if (strcmp(tmp_pattern, "all") == 0) {
                strncpy(pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", 38);
            }
            else if (strcmp(tmp_pattern, "none") == 0) {
                strncpy(pattern, "", 38);
            }
            else {
                strncpy(pattern, tmp_pattern, 38);
            }

            int filename_len = fmatch[2].rm_eo - fmatch[2].rm_so;
            char filename[filename_len + 1];
            strncpy(filename, trim_tok + fmatch[2].rm_so, filename_len);
            filename[filename_len] = '\0';

            if (strcmp(cmd_type, "play") == 0) {
                add_drum(mixr, filename, pattern);
            }
            else {
                int val = atoi(filename);
                int is_val_a_valid_sig_num =
                    (val >= 0 && val < mixr->soundgen_num) ? 1 : 0;
                if (is_val_a_valid_sig_num &&
                    mixr->sound_generators[val]->type == DRUM_TYPE) {
                    add_pattern(mixr->sound_generators[val], pattern);
                }
            }
        }

        // nanosynth melody loop
        regmatch_t fmm_match[4];
        regex_t fmm_rx;
        regcomp(&fmm_rx,
                "^(melody|maddd) ([[:digit:]]+) ([[:alnum:][:space:]:#]+)$",
                REG_EXTENDED | REG_ICASE);
        if (regexec(&fmm_rx, trim_tok, 4, fmm_match, 0) == 0) {
            printf("MELODY MATCH\n");

            char cmd_type[10];
            int sig_num;
            sscanf(trim_tok, "%s %d", cmd_type, &sig_num);

            int pattern_len = fmm_match[3].rm_eo - fmm_match[3].rm_so;
            char pattern[pattern_len + 1];
            strncpy(pattern, trim_tok + fmm_match[3].rm_so, pattern_len);
            pattern[pattern_len] = '\0';
            printf("MELODY PATTERN %s\n", pattern);

            int is_val_a_valid_sig_num =
                (sig_num >= 0 && sig_num < mixr->soundgen_num) ? 1 : 0;
            if (is_val_a_valid_sig_num) {
                if (strncmp(cmd_type, "melody", 7) == 0) {
                    printf("First melody!\n");
                    // keys_start_melody_player(sig_num, pattern);
                }
                else {
                    printf("Maaaaaddd for it!\n");
                    // melody_loop *mloop = mloop_from_pattern(pattern);
                    // nanosynth_add_melody_loop(mixr->sound_generators[sig_num],
                    //                           mloop);
                }
            }
        }

        // loop sample play
        regmatch_t sfmatch[4];
        regex_t sfile_rx;
        regcomp(&sfile_rx, "^(sloop) ([.[:alnum:]]+) ([.[:digit:]]+)$",
                REG_EXTENDED | REG_ICASE);
        if (regexec(&sfile_rx, trim_tok, 4, sfmatch, 0) == 0) {

            printf("SLOOPY CMON!\n");
            int filename_len = sfmatch[2].rm_eo - sfmatch[2].rm_so;
            char filename[filename_len + 1];
            strncpy(filename, trim_tok + sfmatch[2].rm_so, filename_len);
            filename[filename_len] = '\0';

            printf("SLOOPY FILE DONE - DOING LEN!\n");
            int looplen_len = sfmatch[3].rm_eo - sfmatch[3].rm_so;
            char looplen_char[looplen_len + 1];
            strncpy(looplen_char, trim_tok + sfmatch[3].rm_so, looplen_len);
            looplen_char[looplen_len] = '\0';
            double looplen = atof(looplen_char);

            printf("LOOPYZZ %s %f\n", filename, looplen);
            SBMSG *msg = new_sbmsg();
            strncpy(msg->cmd, "timed_sig_start", 19);
            strncpy(msg->params, "sloop", 10);
            strncpy(msg->filename, filename, 99);
            msg->looplen = looplen;
            thrunner(msg);
        }

        // envelope pattern
        regmatch_t ematch[5];
        regex_t env_rx;
        regcomp(&env_rx, "^(env) ([[:digit:]]+) ([[:digit:]]+) ([[:digit:]]+)$",
                REG_EXTENDED | REG_ICASE);
        if (regexec(&env_rx, trim_tok, 5, ematch, 0) == 0) {
            printf("ENVELOPeeee!\n");

            double val1 = 0;
            double val2 = 0;
            double val3 = 0;
            char cmd_type[10];
            sscanf(trim_tok, "%s %lf %lf %lf", cmd_type, &val1, &val2, &val3);

            if (mixr->soundgen_num > val1) {
                // printf("ENV CALLED FOR! %s %.lf %.lf\n", cmd_type, val1,
                // val2);
                if (val3 >= 0 && val3 < 4) {
                    printf("Calling env with %lf %lf %lf\n", val1, val2, val3);
                    ENVSTREAM *e = new_envelope_stream(val2, val3);
                    add_envelope_soundgen(mixr->sound_generators[(int)val1], e);
                }
                else {
                    printf("Sorry, envelope type has to be between 0 and 3");
                }
            }
            else {
                printf("Oofft mate, you don't have enough sound_gens for "
                       "that..\n");
            }
        }
    }
}
