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

                // if (target_pattern_num != -1)
                //{
                //    looper_set_step_pending(g);
                //    parse_step_sequencer_command(
                //        soundgen_num, target_pattern_num, wurds, num_wurds);
                //}
                if (parse_step_sequencer_command(
                        soundgen_num, target_pattern_num, wurds, num_wurds))
                {
                    // no-op - command found
                }
                else
                {
                    if (strncmp("grain_dur_ms", wurds[2], 14) == 0)
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
                    else if (strncmp("audio_buffer_read_idx", wurds[2], 14) ==
                             0)
                    {
                        int pos = atoi(wurds[3]);
                        looper_set_audio_buffer_read_idx(g, pos);
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
                    else if (strncmp("extsource", wurds[2], 9) == 0)
                    {
                        printf("EXTSOURCE!\n");
                        int sg = atoi(wurds[3]);
                        if (mixer_is_valid_soundgen_num(mixr, sg))
                        {
                            printf("GRAN is following "
                                   "%d\n",
                                   sg);
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
                    else if (strncmp("loop_mode", wurds[2], 9) == 0)
                    {
                        unsigned int mode = atoi(wurds[3]);
                        looper_set_loop_mode(g, mode);
                    }
                    else if (strncmp("loop_len", wurds[2], 8) == 0)
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
                    else if (strncmp("step", wurds[2], 4) == 0)
                    {
                        looper_set_step_pending(g);
                    }
                    else if (strncmp("reverse", wurds[2], 8) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        looper_set_reverse_mode(g, mode);
                    }
                    else if (strncmp("l1_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        looper_set_lfo_voice(g, 1, type);
                    }
                    else if (strncmp("l1_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        looper_set_lfo_amp(g, 1, amp);
                    }
                    else if (strncmp("l1_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        looper_set_lfo_rate(g, 1, rate);
                    }
                    else if (strncmp("l1_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        looper_set_lfo_sync(g, 1, loops);
                    }
                    else if (strncmp("l1_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        looper_set_lfo_min(g, 1, min);
                    }
                    else if (strncmp("l1_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        looper_set_lfo_max(g, 1, max);
                    }
                    else if (strncmp("l2_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", type);
                        looper_set_lfo_voice(g, 2, type);
                    }
                    else if (strncmp("l2_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        looper_set_lfo_amp(g, 2, amp);
                    }
                    else if (strncmp("l2_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        looper_set_lfo_rate(g, 2, rate);
                    }
                    else if (strncmp("l2_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        looper_set_lfo_sync(g, 2, loops);
                    }
                    else if (strncmp("l2_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        looper_set_lfo_min(g, 2, min);
                    }
                    else if (strncmp("l2_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        looper_set_lfo_max(g, 2, max);
                    }
                    else if (strncmp("l3_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", type);
                        looper_set_lfo_voice(g, 3, type);
                    }
                    else if (strncmp("l3_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        looper_set_lfo_amp(g, 3, amp);
                    }
                    else if (strncmp("l3_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        looper_set_lfo_rate(g, 3, rate);
                    }
                    else if (strncmp("l3_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        looper_set_lfo_sync(g, 3, loops);
                    }
                    else if (strncmp("l3_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        looper_set_lfo_min(g, 3, min);
                    }
                    else if (strncmp("l3_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        looper_set_lfo_max(g, 3, max);
                    }
                    else if (strncmp("l4_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", type);
                        looper_set_lfo_voice(g, 4, type);
                    }
                    else if (strncmp("l4_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        looper_set_lfo_amp(g, 4, amp);
                    }
                    else if (strncmp("l4_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        looper_set_lfo_rate(g, 4, rate);
                    }
                    else if (strncmp("l4_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        looper_set_lfo_sync(g, 4, loops);
                    }
                    else if (strncmp("l4_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        looper_set_lfo_min(g, 4, min);
                    }
                    else if (strncmp("l4_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        looper_set_lfo_max(g, 4, max);
                    }
                    else if (strncmp("graindur_lfo_on", wurds[2], 14) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->graindur_lfo_on = b;
                    }
                    else if (strncmp("gp_lfo_on", wurds[2], 17) == 0)
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
