#include <stdlib.h>
#include <string.h>

#include <cmdloop.h>
#include <cmdloop/sequence_engine_cmds.h>
#include <looper.h>
#include <mixer.h>
#include <utils.h>

extern mixer *mixr;
bool parse_looper_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("looper", wurds[0], 8) == 0 ||
        strncmp("gran", wurds[0], 4) == 0 || strncmp("loop", wurds[0], 4) == 0)
    {
        if (is_valid_file(wurds[1]) || strncmp(wurds[1], "none", 4) == 0)
        {
            printf("VALID!\n");
            int soundgen_num = add_looper(mixr, wurds[1]);
            printf("sound_generator %d\n", soundgen_num);
        }
        else
        {
            int soundgen_num = -1;
            int target_pattern_num = -1;
            sscanf(wurds[1], "%d:%d", &soundgen_num, &target_pattern_num);
            // int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                mixr->sound_generators[soundgen_num]->type == LOOPER_TYPE)
            {
                looper *g = (looper *)mixr->sound_generators[soundgen_num];

                if (parse_sequence_engine_cmd(soundgen_num, target_pattern_num,
                                              &wurds[2], num_wurds - 2))
                {
                    // no-op, we good
                }
                else
                {
                    if (strncmp("degrade", wurds[2], 7) == 0)
                    {
                        int degrade = atoi(wurds[3]);
                        looper_set_degrade_by(g, degrade);
                    }
                    else if (strncmp("gate_mode", wurds[2], 9) == 0)
                    {
                        bool b = atoi(wurds[3]);
                        looper_set_gate_mode(g, b);
                    }
                    else if (strncmp("grain_dur_ms", wurds[2], 14) == 0)
                    {
                        int dur = atoi(wurds[3]);
                        looper_set_grain_duration(g, dur);
                    }
                    else if (strncmp("grains_per_sec", wurds[2], 14) == 0)
                    {
                        int gps = atoi(wurds[3]);
                        looper_set_grain_density(g, gps);
                    }
                    else if (strncmp("density_dur_sync", wurds[2], 16) == 0)
                    {
                        bool b = atoi(wurds[3]);
                        looper_set_density_duration_sync(g, b);
                    }
                    else if (strncmp("idx", wurds[2], 3) == 0)
                    {
                        unsigned int pos_perc = atoi(wurds[3]);
                        if (pos_perc <= 100)
                        {
                            double pos = g->audio_buffer_len / 100 * pos_perc;
                            looper_set_audio_buffer_read_idx(g, pos);
                        }
                        else
                            printf("idx should be a percent val\n");
                    }
                    else if (strncmp("atk", wurds[2], 3) == 0)
                    {
                        unsigned int attack_ms = atoi(wurds[3]);
                        if (attack_ms > 0 && attack_ms <= 100)
                        {
                            looper_set_grain_env_attack_pct(g, attack_ms);
                        }
                        else
                            printf("atk should be a percent val\n");
                    }
                    else if (strncmp("rel", wurds[2], 3) == 0)
                    {
                        unsigned int release_ms = atoi(wurds[3]);
                        if (release_ms > 0 && release_ms <= 100)
                        {
                            looper_set_grain_env_release_pct(g, release_ms);
                        }
                        else
                            printf("rel should be a percent val\n");
                    }
                    else if (strncmp("fill_factor", wurds[2], 11) == 0)
                    {
                        double ff = atof(wurds[3]);
                        looper_set_fill_factor(g, ff);
                    }
                    else if (strncmp("grain_spray_ms", wurds[2], 14) == 0)
                    {
                        int spray = atoi(wurds[3]);
                        looper_set_granular_spray(g, spray);
                    }
                    else if (strncmp("quasi_grain_fudge", wurds[2], 17) == 0)
                    {
                        int fudge = atoi(wurds[3]);
                        looper_set_quasi_grain_fudge(g, fudge);
                    }
                    else if (strncmp("debug", wurds[2], 5) == 0)
                        looper_dump_buffer(g);
                    else if (strncmp("xsrc", wurds[2], 3) == 0)
                    {
                        int sg = atoi(wurds[3]);
                        if (mixer_is_valid_soundgen_num(mixr, sg))
                        {
                            looper_set_external_source(g, sg);
                        }
                    }
                    else if (strncmp("file", wurds[2], 4) == 0 ||
                             strncmp("open", wurds[2], 4) == 0 ||
                             strncmp("import", wurds[2], 6) == 0)
                    {
                        if (is_valid_file(wurds[3]))
                        {
                            looper_import_file(g, wurds[3]);
                        }
                    }
                    else if (strncmp("pitch", wurds[2], 10) == 0)
                    {
                        double pitch = atof(wurds[3]);
                        looper_set_grain_pitch(g, pitch);
                    }
                    else if (strncmp("selection_mode", wurds[2], 14) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        looper_set_selection_mode(g, mode);
                    }
                    else if (strncmp("env_mode", wurds[2], 8) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        looper_set_envelope_mode(g, mode);
                    }
                    else if (strncmp("mode", wurds[2], 4) == 0)
                    {
                        unsigned int mode = atoi(wurds[3]);
                        looper_set_loop_mode(g, mode);
                    }
                    else if (strncmp("len", wurds[2], 3) == 0)
                    {
                        double len = atof(wurds[3]);
                        looper_set_loop_len(g, len);
                    }
                    else if (strncmp("reverse", wurds[2], 8) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        looper_set_reverse_mode(g, mode);
                    }
                    else if (strncmp("scramble", wurds[2], 8) == 0)
                    {
                        looper_set_scramble_pending(g);
                    }
                    else if (strncmp("stutter", wurds[2], 8) == 0)
                    {
                        looper_set_stutter_pending(g);
                    }
                    else if (strncmp("step", wurds[2], 4) == 0)
                    {
                        bool b = atoi(wurds[3]);
                        looper_set_step_mode(g, b);
                    }
                    else if (strncmp("trace", wurds[2], 5) == 0)
                    {
                        looper_set_trace_envelope(g);
                    }
                    else if (strncmp("xmode", wurds[2], 5) == 0)
                    {
                        unsigned int mode = atoi(wurds[3]);
                        looper_set_grain_external_source_mode(g, mode);
                    }

                    // Env Gen
                    else if (strncmp("eg_attack_ms", wurds[2], 16) == 0)
                    {
                        int attack = atoi(wurds[3]);
                        eg_set_attack_time_msec(&g->m_eg1, attack);
                    }
                    else if (strncmp("eg_release_ms", wurds[2], 17) == 0)
                    {
                        int release = atoi(wurds[3]);
                        eg_set_release_time_msec(&g->m_eg1, release);
                    }
                }
            }
        }
        return true;
    }
    return false;
}
