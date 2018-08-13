#include <stdlib.h>
#include <string.h>

#include <cmdloop.h>
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
            printf("soundgenerator %d\n", soundgen_num);
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

                if (parse_step_sequencer_command(
                        soundgen_num, target_pattern_num, wurds, num_wurds))
                {
                    // no-op - command found
                }
                else
                {
                    if (strncmp("gate_mode", wurds[2], 9) == 0)
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
                    else if (strncmp("idx", wurds[2], 333) == 0)
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
                    else if (strncmp("extsource", wurds[2], 9) == 0)
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
                    else if (strncmp("scramble", wurds[2], 8) == 0)
                    {
                        looper_set_scramble_pending(g);
                    }
                    else if (strncmp("stutter", wurds[2], 8) == 0)
                    {
                        looper_set_stutter_pending(g);
                    }
                    else if (strncmp("sustain_ms", wurds[2], 10) == 0)
                    {
                        double sustain_ms = atoi(wurds[3]);
                        looper_set_sustain_ms(g, sustain_ms);
                    }
                    else if (strncmp("step", wurds[2], 4) == 0)
                    {
                        looper_set_step_pending(g);
                    }
                    else if (strncmp("reverse", wurds[2], 8) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        looper_set_reverse_mode(g, mode);
                    }

                    // Grain Duration LFO
                    else if (strncmp("gd_on", wurds[2], 14) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->graindur_lfo_on = b;
                    }
                    else if (strncmp("gd_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        looper_set_lfo_voice(g, 1, type);
                    }
                    else if (strncmp("gd_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        looper_set_lfo_amp(g, 1, amp);
                    }
                    else if (strncmp("gd_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        if (strncmp(wurds[3], "sync", 4) == 0)
                        {
                            rate = mixer_get_hz_per_bar(mixr);
                            int bars = atoi(wurds[4]);
                            if (bars > 0)
                                rate /= bars;
                        }
                        looper_set_lfo_rate(g, 1, rate);
                    }
                    else if (strncmp("gd_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        looper_set_lfo_sync(g, 1, loops);
                    }
                    else if (strncmp("gd_lo", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        looper_set_lfo_min(g, 1, min);
                    }
                    else if (strncmp("gd_hi", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        looper_set_lfo_max(g, 1, max);
                    }

                    // Grains Per Sec LFO
                    else if (strncmp("gps_on", wurds[2], 13) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->grainps_lfo_on = b;
                    }
                    else if (strncmp("gps_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", type);
                        looper_set_lfo_voice(g, 2, type);
                    }
                    else if (strncmp("gps_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        looper_set_lfo_amp(g, 2, amp);
                    }
                    else if (strncmp("gps_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        if (strncmp(wurds[3], "sync", 4) == 0)
                        {
                            rate = mixer_get_hz_per_bar(mixr);
                            int bars = atoi(wurds[4]);
                            if (bars > 0)
                                rate /= bars;
                        }
                        looper_set_lfo_rate(g, 2, rate);
                    }
                    else if (strncmp("gps_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        looper_set_lfo_sync(g, 2, loops);
                    }
                    else if (strncmp("gps_lo", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        looper_set_lfo_min(g, 2, min);
                    }
                    else if (strncmp("gps_hi", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        looper_set_lfo_max(g, 2, max);
                    }

                    // Grain Scan LFO
                    else if (strncmp("gs_on", wurds[2], 16) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->grainscanfile_lfo_on = b;
                    }
                    else if (strncmp("gs_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", type);
                        looper_set_lfo_voice(g, 3, type);
                    }
                    else if (strncmp("gs_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        looper_set_lfo_amp(g, 3, amp);
                    }
                    else if (strncmp("gs_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        if (strncmp(wurds[3], "sync", 4) == 0)
                        {
                            rate = mixer_get_hz_per_bar(mixr);
                            int bars = atoi(wurds[4]);
                            if (bars > 0)
                                rate /= bars;
                        }
                        looper_set_lfo_rate(g, 3, rate);
                    }
                    else if (strncmp("gs_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        looper_set_lfo_sync(g, 3, loops);
                    }
                    else if (strncmp("gs_lo", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        looper_set_lfo_min(g, 3, min);
                    }
                    else if (strncmp("gs_hi", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        looper_set_lfo_max(g, 3, max);
                    }

                    // GRAIN PITCH LFO
                    else if (strncmp("gp_on", wurds[2], 17) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->grainpitch_lfo_on = b;
                    }
                    else if (strncmp("gp_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", type);
                        looper_set_lfo_voice(g, 4, type);
                    }
                    else if (strncmp("gp_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        looper_set_lfo_amp(g, 4, amp);
                    }
                    else if (strncmp("gp_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        if (strncmp(wurds[3], "sync", 4) == 0)
                        {
                            rate = mixer_get_hz_per_bar(mixr);
                            int bars = atoi(wurds[4]);
                            if (bars > 0)
                                rate /= bars;
                        }
                        looper_set_lfo_rate(g, 4, rate);
                    }
                    else if (strncmp("gp_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        looper_set_lfo_sync(g, 4, loops);
                    }
                    else if (strncmp("gp_lo", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        looper_set_lfo_min(g, 4, min);
                    }
                    else if (strncmp("gp_hi", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        looper_set_lfo_max(g, 4, max);
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
