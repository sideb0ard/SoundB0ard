#include <stdlib.h>
#include <string.h>

#include <cmdloop.h>
#include <cmdloop/sequence_engine_cmds.h>
#include <drumsampler.h>
#include <drumsynth.h>
#include <looper.h>
#include <mixer.h>
#include <pattern_parser.h>
#include <stepper_cmds.h>
#include <utils.h>

extern mixer *mixr;

bool parse_stepper_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("step", wurds[0], 4) == 0)
    {

        if (strncmp("ls", wurds[1], 2) == 0)
        {
            drumsynth_list_patches();
            return true;
        }

        int soundgen_num = -1;
        int target_pattern_num = -1;
        sscanf(wurds[1], "%d:%d", &soundgen_num, &target_pattern_num);

        if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
            is_stepper(mixr->SoundGenerators[soundgen_num]))
        {
            if (parse_sequence_engine_cmd(soundgen_num, target_pattern_num,
                                          &wurds[2], num_wurds - 2))
            {
                // no-op, we good
            }
            else if (mixr->SoundGenerators[soundgen_num]->type ==
                     DRUMSAMPLER_TYPE)
            {
                drumsampler *ds =
                    (drumsampler *)mixr->SoundGenerators[soundgen_num];

                parse_drumsampler_cmd(ds, &wurds[2], num_wurds - 2);
            }
            else if (mixr->SoundGenerators[soundgen_num]->type ==
                     DRUMSYNTH_TYPE)
            {
                drumsynth *ds =
                    (drumsynth *)mixr->SoundGenerators[soundgen_num];
                parse_drumsynth_cmd(ds, &wurds[2], num_wurds - 2);
            }
        }
        return true;
    }
    return false;
}

void parse_drumsampler_cmd(drumsampler *ds, char wurds[][SIZE_OF_WURD],
                           int num_wurds)
{
    if (strncmp("attack_ms", wurds[0], 9) == 0)
    {
        float val = atof(wurds[1]);
        drumsampler_set_attack_time(ds, val);
    }
    else if (strncmp("decay_ms", wurds[0], 8) == 0)
    {
        float val = atof(wurds[1]);
        drumsampler_set_decay_time(ds, val);
    }
    else if (strncmp("eg", wurds[0], 2) == 0)
    {
        bool b = atoi(wurds[1]);
        drumsampler_enable_envelope_generator(ds, b);
    }
    else if (strncmp("end_pos", wurds[0], 7) == 0)
    {
        int pct = atoi(wurds[1]);
        drumsampler_set_cutoff_percent(ds, pct);
    }
    else if (strncmp("glitch", wurds[0], 6) == 0)
    {
        bool b = atoi(wurds[1]);
        drumsampler_set_glitch_mode(ds, b);
    }
    else if (strncmp("gpct", wurds[0], 4) == 0)
    {
        int pct = atoi(wurds[1]);
        drumsampler_set_glitch_rand_factor(ds, pct);
    }
    else if (strncmp("load", wurds[0], 4) == 0 ||
             strncmp("import", wurds[0], 6) == 0)
    {
        if (is_valid_file(wurds[1]))
        {
            printf("Changing Loaded "
                   "FILE!\n");
            drumsampler_import_file(ds, wurds[1]);
        }
        else
            printf("%s is not a valid file\n", wurds[1]);
    }
    else if (strncmp("pitch", wurds[0], 5) == 0)
    {
        double v = atof(wurds[1]);
        drumsampler_set_pitch(ds, v);
    }
    else if (strncmp("release_ms", wurds[0], 9) == 0)
    {
        float val = atof(wurds[1]);
        drumsampler_set_release_time(ds, val);
    }
    else if (strncmp("sustain", wurds[0], 7) == 0)
    {
        float val = atof(wurds[1]);
        drumsampler_set_sustain_lvl(ds, val);
    }
}

void parse_drumsynth_cmd(drumsynth *ds, char wurds[][SIZE_OF_WURD],
                         int num_wurds)
{
    (void)num_wurds;
    double val = atof(wurds[1]);
    if (strncmp("debug", wurds[0], 5) == 0)
    {
        drumsynth_set_debug(ds, val);
    }
    if (strncmp("save", wurds[0], 4) == 0)
    {
        drumsynth_save_patch(ds, wurds[1]);
    }
    if (strncmp("open", wurds[0], 4) == 0 ||
        strncmp("load", wurds[0], 4) == 0 ||
        strncmp("import", wurds[0], 6) == 0)
    {
        drumsynth_open_patch(ds, wurds[1]);
    }
    else if (strncmp("distortion_threshold", wurds[0], 20) == 0)
    {
        drumsynth_set_distortion_threshold(ds, val);
    }
    else if (strncmp("o1_wav", wurds[0], 6) == 0)
    {
        drumsynth_set_osc_wav(ds, 1, val);
    }
    else if (strncmp("o1_fo", wurds[0], 6) == 0)
    {
        drumsynth_set_osc_fo(ds, 1, val);
    }
    else if (strncmp("o1_amp", wurds[0], 6) == 0)
    {
        drumsynth_set_osc_amp(ds, 1, val);
    }
    else if (strncmp("e2_o2_int", wurds[0], 8) == 0)
    {
        drumsynth_set_eg_osc_intensity(ds, 2, 2, val);
    }
    else if (strncmp("e1_att", wurds[0], 6) == 0)
    {
        drumsynth_set_eg_attack(ds, 1, val);
    }
    else if (strncmp("e1_dec", wurds[0], 6) == 0)
    {
        drumsynth_set_eg_decay(ds, 1, val);
    }
    else if (strncmp("e1_sus_lvl", wurds[0], 10) == 0)
    {
        drumsynth_set_eg_sustain_lvl(ds, 1, val);
    }
    else if (strncmp("e1_rel", wurds[0], 6) == 0)
    {
        drumsynth_set_eg_release(ds, 1, val);
    }
    else if (strncmp("o2_wav", wurds[0], 6) == 0)
    {
        drumsynth_set_osc_wav(ds, 2, val);
    }
    else if (strncmp("o2_fo", wurds[0], 6) == 0)
    {
        drumsynth_set_osc_fo(ds, 2, val);
    }
    else if (strncmp("o2_amp", wurds[0], 6) == 0)
    {
        drumsynth_set_osc_amp(ds, 2, val);
    }
    else if (strncmp("mod_pitch_semitones", wurds[0], 19) == 0)
    {
        drumsynth_set_mod_semitones_range(ds, val);
    }
    else if (strncmp("e2_att", wurds[0], 6) == 0)
    {
        drumsynth_set_eg_attack(ds, 2, val);
    }
    else if (strncmp("e2_dec", wurds[0], 6) == 0)
    {
        drumsynth_set_eg_decay(ds, 2, val);
    }
    else if (strncmp("e2_sus_lvl", wurds[0], 10) == 0)
    {
        drumsynth_set_eg_sustain_lvl(ds, 2, val);
    }
    else if (strncmp("e2_rel", wurds[0], 6) == 0)
    {
        drumsynth_set_eg_release(ds, 2, val);
    }
    else if (strncmp("filter_type", wurds[0], 11) == 0)
    {
        drumsynth_set_filter_type(ds, val);
    }
    else if (strncmp("freq", wurds[0], 4) == 0)
    {
        drumsynth_set_filter_freq(ds, val);
    }
    else if (strncmp("rand", wurds[0], 4) == 0)
    {
        drumsynth_randomize(ds);
    }
    else if (strncmp("reset", wurds[0], 5) == 0)
    {
        drumsynth_set_reset_osc(ds, val);
    }
    else if (strncmp("q", wurds[0], 1) == 0)
    {
        drumsynth_set_filter_q(ds, val);
    }
}
