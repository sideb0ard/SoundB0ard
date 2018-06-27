#include <stdlib.h>
#include <string.h>

#include <cmdloop.h>
#include <digisynth.h>
#include <dxsynth.h>
#include <midimaaan.h>
#include <minisynth.h>
#include <mixer.h>
#include <pattern_parser.h>
#include <synth_cmds.h>

extern mixer *mixr;
bool parse_synth_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("syn", wurds[0], 3) == 0 || strncmp("moog", wurds[0], 4) == 0 ||
        strncmp("digi", wurds[0], 4) == 0 || strncmp("dx", wurds[0], 2) == 0 ||
        strncmp("fm", wurds[0], 2) == 0)
    {
        if (strncmp(wurds[1], "ls", 2) == 0)
        {
            if (strncmp("moog", wurds[0], 4) == 0)
            {
                synthbase_list_presets(MINISYNTH_TYPE);
            }
            else if (strncmp("dx", wurds[0], 2) == 0)
            {
                synthbase_list_presets(DXSYNTH_TYPE);
            }
            return true;
        }

        int soundgen_num = -1;
        int target_pattern_num = -1;
        sscanf(wurds[1], "%d:%d", &soundgen_num, &target_pattern_num);

        if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
            is_synth(mixr->sound_generators[soundgen_num]))
        {

            if (parse_synthbase_cmd(soundgen_num, target_pattern_num, &wurds[2],
                                    num_wurds - 2))
            {
                // no-op, we good
            }
            else if (mixr->sound_generators[soundgen_num]->type == DXSYNTH_TYPE)
            {
                dxsynth *dx = (dxsynth *)mixr->sound_generators[soundgen_num];
                if (parse_dxsynth_settings_change(dx, wurds))
                {
                    dxsynth_update(dx);
                }
            }
            else if (mixr->sound_generators[soundgen_num]->type ==
                     MINISYNTH_TYPE)
            {
                minisynth *ms =
                    (minisynth *)mixr->sound_generators[soundgen_num];
                if (parse_minisynth_settings_change(ms, wurds))
                {
                    minisynth_update(ms);
                }
            }
            else if (mixr->sound_generators[soundgen_num]->type ==
                     DIGISYNTH_TYPE)
            {
                digisynth *ds =
                    (digisynth *)mixr->sound_generators[soundgen_num];
                (void)ds;
            }
        }
        return true;
    }
    return false;
}

bool parse_dxsynth_settings_change(dxsynth *dx, char wurds[][SIZE_OF_WURD])
{
    printf("PARSLEYDX!\n");
    if (strncmp("algo", wurds[2], 4) == 0)
    {
        int algo = atoi(wurds[3]);
        dxsynth_set_voice_mode(dx, algo);
    }
    else if (strncmp("open", wurds[2], 4) == 0 ||
             strncmp("load", wurds[2], 4) == 0)
    {
        dxsynth_load_settings(dx, wurds[3]);
    }
    else if (strncmp("save", wurds[2], 4) == 0)
    {
        dxsynth_save_settings(dx, wurds[3]);
    }
    else if (strncmp("porta", wurds[2], 5) == 0)
    {
        double ms = atof(wurds[3]);
        printf("DXSynth change portamento time ms:%.2f!\n", ms);
        dxsynth_set_portamento_time_ms(dx, ms);
        return true;
    }
    else if (strncmp("rand", wurds[2], 4) == 0)
    {
        printf("RAND DX!\n");
        dxsynth_rand_settings(dx);
    }
    else if (strncmp("pitchrange", wurds[2], 10) == 0)
    {
        int val = atoi(wurds[3]);
        printf("DXSynth change pitchrange:%d!\n", val);
        dxsynth_set_pitchbend_range(dx, val);
        return true;
    }
    else if (strncmp("vel2att", wurds[2], 10) == 0)
    {
        bool val = atoi(wurds[3]);
        printf("DXSynth change velocity to attack!%s\n",
               val ? "true" : "false");
        dxsynth_set_velocity_to_attack_scaling(dx, val);
        return true;
    }
    else if (strncmp("note2dec", wurds[2], 10) == 0)
    {
        bool val = atoi(wurds[3]);
        printf("DXSynth change note to decay!?%s\n", val ? "true" : "false");
        dxsynth_set_note_number_to_decay_scaling(dx, val);
        return true;
    }
    else if (strncmp("reset2zero", wurds[2], 10) == 0)
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
    else if (strncmp("l1_int", wurds[2], 14) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change LFO1 intensity:%.2f!\n", val);
        dxsynth_set_lfo1_intensity(dx, val);
        return true;
    }
    else if (strncmp("l1_rate", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        if (strncmp("sync", wurds[3], 4) == 0)
            val = mixer_get_khz_per_bar(mixr);
        printf("DXSynth change LFO1 rate!%.2f\n", val);
        dxsynth_set_lfo1_rate(dx, val);
        return true;
    }
    else if (strncmp("l1_wav", wurds[2], 13) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change LFO1 waveform:%d!\n", val);
        dxsynth_set_lfo1_waveform(dx, val);
        return true;
    }
    else if (strncmp("l1_dest1", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        printf("DXSynth change LFO1 dest1:%d!\n", dest);
        dxsynth_set_lfo1_mod_dest(dx, 1, dest);
        return true;
    }
    else if (strncmp("l1_dest2", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        printf("DXSynth change LFO1 dest2:%d!\n", dest);
        dxsynth_set_lfo1_mod_dest(dx, 2, dest);
        return true;
    }
    else if (strncmp("l1_dest3", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        printf("DXSynth change LFO1 dest3:%d!\n", dest);
        dxsynth_set_lfo1_mod_dest(dx, 3, dest);
        return true;
    }
    else if (strncmp("l1_dest4", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        printf("DXSynth change LFO1 dest4:%d!\n", dest);
        dxsynth_set_lfo1_mod_dest(dx, 4, dest);
        return true;
    }
    else if (strncmp("o1wav", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change OP1 wave:%d!\n", val);
        dxsynth_set_op_waveform(dx, 1, val);
        return true;
    }
    else if (strncmp("o1rat", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP1 ratio:%.2f!\n", val);
        dxsynth_set_op_ratio(dx, 1, val);
        return true;
    }
    else if (strncmp("o1det", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP1 detune:%.2f!\n", val);
        dxsynth_set_op_detune(dx, 1, val);
        return true;
    }
    else if (strncmp("e1att", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG1 attack ms:%.2f!\n", val);
        dxsynth_set_eg_attack_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("e1dec", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG1 decay ms:%.2f!\n", val);
        dxsynth_set_eg_decay_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("e1sus", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG1 sustain lvl:%.2f!\n", val);
        dxsynth_set_eg_sustain_lvl(dx, 1, val);
        return true;
    }
    else if (strncmp("e1rel", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG1 release ms:%.2f!\n", val);
        dxsynth_set_eg_release_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("op1out", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP1 output:%.2f!\n", val);
        dxsynth_set_op_output_lvl(dx, 1, val);
        return true;
    }

    else if (strncmp("o2wav", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change OP2 wave:%d!\n", val);
        dxsynth_set_op_waveform(dx, 2, val);
        return true;
    }
    else if (strncmp("o2rat", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP2 ratio:%.2f!\n", val);
        dxsynth_set_op_ratio(dx, 2, val);
        return true;
    }
    else if (strncmp("o2det", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP2 detune:%.2f!\n", val);
        dxsynth_set_op_detune(dx, 2, val);
        return true;
    }
    else if (strncmp("e2att", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG2 attack ms:%.2f!\n", val);
        dxsynth_set_eg_attack_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("e2dec", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG2 decay ms:%.2f!\n", val);
        dxsynth_set_eg_decay_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("e2sus", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG2 sustain lvl:%.2f!\n", val);
        dxsynth_set_eg_sustain_lvl(dx, 2, val);
        return true;
    }
    else if (strncmp("e2rel", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG2 release ms:%.2f!\n", val);
        dxsynth_set_eg_release_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("op2out", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP2 output:%.2f!\n", val);
        dxsynth_set_op_output_lvl(dx, 2, val);
        return true;
    }

    else if (strncmp("o3wav", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change OP3 wave:%d!\n", val);
        dxsynth_set_op_waveform(dx, 3, val);
        return true;
    }
    else if (strncmp("o3rat", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP3 ratio:%.2f!\n", val);
        dxsynth_set_op_ratio(dx, 3, val);
        return true;
    }
    else if (strncmp("o3det", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP3 detune:%.2f!\n", val);
        dxsynth_set_op_detune(dx, 3, val);
        return true;
    }
    else if (strncmp("e3att", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG3 attack ms:%.2f!\n", val);
        dxsynth_set_eg_attack_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("e3dec", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG3 decay ms:%.2f!\n", val);
        dxsynth_set_eg_decay_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("e3sus", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG3 sustain lvl:%.2f!\n", val);
        dxsynth_set_eg_sustain_lvl(dx, 3, val);
        return true;
    }
    else if (strncmp("e3rel", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG3 release ms:%.2f!\n", val);
        dxsynth_set_eg_release_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("op3out", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP3 output:%.2f!\n", val);
        dxsynth_set_op_output_lvl(dx, 3, val);
        return true;
    }

    else if (strncmp("o4wav", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        printf("DXSynth change OP4 wave:%d!\n", val);
        dxsynth_set_op_waveform(dx, 4, val);
        return true;
    }
    else if (strncmp("o4rat", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP4 ratio:%.2f!\n", val);
        dxsynth_set_op_ratio(dx, 4, val);
        return true;
    }
    else if (strncmp("o4det", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP4 detune:%.2f!\n", val);
        dxsynth_set_op_detune(dx, 4, val);
        return true;
    }
    else if (strncmp("e4att", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG4 attack ms:%.2f!\n", val);
        dxsynth_set_eg_attack_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("e4dec", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG4 decay ms:%.2f!\n", val);
        dxsynth_set_eg_decay_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("e4sus", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG4 sustain lvl:%.2f!\n", val);
        dxsynth_set_eg_sustain_lvl(dx, 4, val);
        return true;
    }
    else if (strncmp("e4rel", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change EG4 release ms:%.2f!\n", val);
        dxsynth_set_eg_release_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("op4out", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        printf("DXSynth change OP4 output:%.2f!\n", val);
        dxsynth_set_op_output_lvl(dx, 4, val);
        return true;
    }
    else if (strncmp("op4fb", wurds[2], 11) == 0)
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
    bool to_update = true;
    double val = atof(wurds[3]);
    if (strncmp("attackms", wurds[2], 8) == 0)
        minisynth_set_attack_time_ms(ms, val);
    else if (strncmp("decayms", wurds[2], 7) == 0)
        minisynth_set_decay_time_ms(ms, val);
    else if (strncmp("detune", wurds[2], 6) == 0)
        minisynth_set_detune(ms, val);
    else if (strncmp("eg1_osc_en", wurds[2], 9) == 0)
        minisynth_set_eg1_osc_enable(ms, val);
    else if (strncmp("eg1_osc_int", wurds[2], 9) == 0)
        minisynth_set_eg1_osc_int(ms, val);
    else if (strncmp("eg1_dca_en", wurds[2], 14) == 0)
        minisynth_set_eg1_dca_enable(ms, val);
    else if (strncmp("eg1_dca_int", wurds[2], 9) == 0)
        minisynth_set_eg1_dca_int(ms, val);
    else if (strncmp("eg1_filter_en", wurds[2], 14) == 0)
        minisynth_set_eg1_filter_enable(ms, val);
    else if (strncmp("eg1_filter_int", wurds[2], 12) == 0)
        minisynth_set_eg1_filter_int(ms, val);
    else if (strncmp("fc", wurds[2], 2) == 0)
        minisynth_set_filter_fc(ms, val);
    else if (strncmp("fq", wurds[2], 2) == 0)
        minisynth_set_filter_fq(ms, val);
    else if (strncmp("filter", wurds[2], 4) == 0)
        minisynth_set_filter_type(ms, val);
    else if (strncmp("ktint", wurds[2], 5) == 0)
        minisynth_set_keytrack_int(ms, val);
    else if (strncmp("kt", wurds[2], 2) == 0)
        minisynth_set_keytrack(ms, val);
    else if (strncmp("legato", wurds[2], 6) == 0)
        minisynth_set_legato_mode(ms, val);
    else if (strncmp("l1wave", wurds[2], 7) == 0)
        minisynth_set_lfo_wave(ms, 1, val);
    else if (strncmp("l1mode", wurds[2], 7) == 0)
        minisynth_set_lfo_mode(ms, 1, val);
    else if (strncmp("l1rate", wurds[2], 8) == 0)
    {
        if (strncmp("sync", wurds[3], 4) == 0)
            val = mixer_get_khz_per_bar(mixr);
        minisynth_set_lfo_rate(ms, 1, val);
    }
    else if (strncmp("l1amp", wurds[2], 7) == 0)
        minisynth_set_lfo_amp(ms, 1, val);
    else if (strncmp("l1_osc_en", wurds[2], 16) == 0)
        minisynth_set_lfo_osc_enable(ms, 1, val);
    else if (strncmp("l1_osc_int", wurds[2], 16) == 0)
        minisynth_set_lfo_osc_int(ms, 1, val);
    else if (strncmp("l1_filter_en", wurds[2], 16) == 0)
        minisynth_set_lfo_filter_enable(ms, 1, val);
    else if (strncmp("l1_filter_int", wurds[2], 21) == 0)
        minisynth_set_lfo_filter_fc_int(ms, 1, val);
    else if (strncmp("l1_amp_en", wurds[2], 16) == 0)
        minisynth_set_lfo_amp_enable(ms, 1, val);
    else if (strncmp("l1_amp_int", wurds[2], 16) == 0)
        minisynth_set_lfo_amp_int(ms, 1, val);
    else if (strncmp("l1_pan_en", wurds[2], 16) == 0)
        minisynth_set_lfo_pan_enable(ms, 1, val);
    else if (strncmp("l1_pan_int", wurds[2], 10) == 0)
        minisynth_set_lfo_pan_int(ms, 1, val);
    else if (strncmp("l1_pw_en", wurds[2], 16) == 0)
        minisynth_set_lfo_pulsewidth_enable(ms, 1, val);
    else if (strncmp("l1_pw_int", wurds[2], 10) == 0)
        minisynth_set_lfo_pulsewidth_int(ms, 1, val);
    else if (strncmp("l2wave", wurds[2], 7) == 0)
        minisynth_set_lfo_wave(ms, 2, val);
    else if (strncmp("l2mode", wurds[2], 7) == 0)
        minisynth_set_lfo_mode(ms, 2, val);
    else if (strncmp("l2rate", wurds[2], 8) == 0)
    {
        if (strncmp("sync", wurds[3], 4) == 0)
            val = mixer_get_khz_per_bar(mixr);
        minisynth_set_lfo_rate(ms, 2, val);
    }
    else if (strncmp("l2amp", wurds[2], 7) == 0)
        minisynth_set_lfo_amp(ms, 2, val);
    else if (strncmp("l2_osc_en", wurds[2], 16) == 0)
        minisynth_set_lfo_osc_enable(ms, 2, val);
    else if (strncmp("l2_osc_int", wurds[2], 16) == 0)
        minisynth_set_lfo_osc_int(ms, 2, val);
    else if (strncmp("l2_filter_en", wurds[2], 16) == 0)
        minisynth_set_lfo_filter_enable(ms, 2, val);
    else if (strncmp("l2_filter_int", wurds[2], 21) == 0)
        minisynth_set_lfo_filter_fc_int(ms, 2, val);
    else if (strncmp("l2_amp_en", wurds[2], 16) == 0)
        minisynth_set_lfo_amp_enable(ms, 2, val);
    else if (strncmp("l2_amp_int", wurds[2], 16) == 0)
        minisynth_set_lfo_amp_int(ms, 2, val);
    else if (strncmp("l2_pan_en", wurds[2], 16) == 0)
        minisynth_set_lfo_pan_enable(ms, 2, val);
    else if (strncmp("l2_pan_int", wurds[2], 10) == 0)
        minisynth_set_lfo_pan_int(ms, 2, val);
    else if (strncmp("l2_pw_en", wurds[2], 16) == 0)
        minisynth_set_lfo_pulsewidth_enable(ms, 2, val);
    else if (strncmp("l2_pw_int", wurds[2], 10) == 0)
        minisynth_set_lfo_pulsewidth_int(ms, 2, val);
    else if (strncmp("load", wurds[2], 4) == 0)
    {
        char preset_name[20];
        strncpy(preset_name, wurds[3], 19);
        minisynth_load_settings(ms, preset_name);
    }
    else if (strncmp("mono", wurds[2], 4) == 0)
        minisynth_set_monophonic(ms, val);
    else if (strncmp("nlp", wurds[2], 3) == 0)
        minisynth_set_filter_nlp(ms, val);
    else if (strncmp("ndscale", wurds[2], 7) == 0)
        minisynth_set_note_to_decay_scaling(ms, val);
    else if (strncmp("noisedb", wurds[2], 7) == 0)
        minisynth_set_noise_osc_db(ms, val);
    else if (strncmp("oct", wurds[2], 3) == 0)
        minisynth_set_octave(ms, val);
    else if (strncmp("pitchrange", wurds[2], 10) == 0)
        minisynth_set_pitchbend_range(ms, val);
    else if (strncmp("porta", wurds[2], 5) == 0)
        minisynth_set_portamento_time_ms(ms, val);
    else if (strncmp("pw", wurds[2], 2) == 0)
        minisynth_set_pulsewidth_pct(ms, val);
    else if (strncmp("print", wurds[2], 5) == 0)
    {
        if (strncmp(wurds[3], "patterns", 7) == 0)
        {
            minisynth_print_patterns(ms);
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
    else if (strncmp("rand", wurds[2], 4) == 0)
    {
        minisynth_rand_settings(ms);
    }
    else if (strncmp("releasems", wurds[2], 7) == 0)
        minisynth_set_release_time_ms(ms, val);
    else if (strncmp("save", wurds[2], 4) == 0)
    {
        char preset_name[20];
        strncpy(preset_name, wurds[3], 19);
        minisynth_save_settings(ms, preset_name);
    }
    else if (strncmp("saturation", wurds[2], 10) == 0)
        minisynth_set_filter_saturation(ms, val);
    else if (strncmp("subosc", wurds[2], 6) == 0)
        minisynth_set_sub_osc_db(ms, val);
    else if (strncmp("sustainlvl", wurds[2], 10) == 0)
        minisynth_set_sustain(ms, val);
    else if (strncmp("sustainms", wurds[2], 9) == 0)
        minisynth_set_sustain_time_ms(ms, val);
    else if (strncmp("sustain16th", wurds[2], 11) == 0)
        minisynth_set_sustain_time_sixteenth(ms, val);
    else if (strncmp("sustain", wurds[2], 7) == 0)
        minisynth_set_sustain_override(ms, val);
    else if (strncmp("vascale", wurds[2], 7) == 0)
        minisynth_set_velocity_to_attack_scaling(ms, val);
    else if (strncmp("voice", wurds[2], 5) == 0)
        minisynth_set_voice_mode(ms, val);
    else if (strncmp("vol", wurds[2], 3) == 0)
        minisynth_set_vol(ms, val);
    else if (strncmp("zero", wurds[2], 4) == 0)
        minisynth_set_reset_to_zero(ms, val);
    else
        to_update = false;

    return to_update;
}

// void char_pattern_to_midi_pattern(synthbase *base, int dest_pattern,
//                                  char char_array[NUM_WURDS][SIZE_OF_WURD],
//                                  int start, int end)
//{
//    for (int i = start; i < end; i++)
//    {
//        int tick = 0;
//        int midi_note = 0;
//        chord_midi_notes chnotes = {0, 0, 0};
//        if (extract_chord_from_char_notation(char_array[i], &tick, &chnotes))
//        {
//            synthbase_add_note(base, dest_pattern, tick, chnotes.root, true);
//            synthbase_add_note(base, dest_pattern, tick, chnotes.third, true);
//            synthbase_add_note(base, dest_pattern, tick, chnotes.fifth, true);
//        }
//        else
//        {
//            sscanf(char_array[i], "%d:%d", &tick, &midi_note);
//            if (midi_note != 0)
//            {
//                printf("Adding %d:%d\n", tick, midi_note);
//                synthbase_add_note(base, dest_pattern, tick, midi_note, true);
//            }
//        }
//    }
//}
//
// bool extract_chord_from_char_notation(char *wurd, int *tick,
//                                      chord_midi_notes *chnotes)
//{
//    char char_note[3] = {0};
//    sscanf(wurd, "%d:%s", tick, char_note);
//
//    if (strncasecmp(char_note, "c", 1) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("C_MAJOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "dm", 2) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("D_MINOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "d", 1) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("D_MAJOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "em", 2) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("E_MINOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "e", 1) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("E_MAJOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "f", 1) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("F_MAJOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "gm", 2) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("G_MINOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "g", 1) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("G_MAJOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "am", 2) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("A_MINOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "a", 1) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("A_MAJOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "bm", 2) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("B_MINOR");
//        return true;
//    }
//    else if (strncasecmp(char_note, "b", 1) == 0)
//    {
//        *chnotes = get_midi_notes_from_char_chord("B_MAJOR");
//        return true;
//    }
//
//    return false;
//}

bool parse_synthbase_cmd(int soundgen_num, int pattern_num,
                         char wurds[][SIZE_OF_WURD], int num_wurds)
{
    synthbase *base = get_synthbase(mixr->sound_generators[soundgen_num]);

    bool cmd_found = true;
    // synthbase specific first, then patterns below
    if (pattern_num == -1)
    {
        if (strncmp("arp_mode", wurds[0], 8) == 0)
        {
            unsigned int mode = atoi(wurds[1]);
            synthbase_set_arp_mode(base, mode);
        }
        else if (strncmp("arp_speed", wurds[0], 9) == 0)
        {
            unsigned int speed = atoi(wurds[1]);
            switch (speed)
            {
            case (32):
                synthbase_set_arp_speed(base, ARP_32);
                break;
            case (16):
                synthbase_set_arp_speed(base, ARP_16);
                break;
            case (8):
                synthbase_set_arp_speed(base, ARP_8);
                break;
            case (4):
                synthbase_set_arp_speed(base, ARP_4);
                break;
            default:
                printf("Speed has to be one of 32, 16, 8 or 4\n");
            }
        }
        else if (strncmp("arp", wurds[0], 3) == 0)
        {
            bool enable = atoi(wurds[1]);
            synthbase_enable_arp(base, enable);
        }
        else if (strncmp("chord_mode", wurds[0], 10) == 0)
        {
            bool b = atoi(wurds[1]);
            synthbase_set_chord_mode(base, b);
        }
        else if (strncmp("note_mode", wurds[0], 10) == 0)
        {
            bool b = atoi(wurds[1]);
            synthbase_set_note_mode(base, b);
        }
        else if (strncmp("note_on", wurds[0], 7) == 0)
        {
            for (int i = 3; i < num_wurds; i++)
            {
                int six16th = atoi(wurds[i]) % 16;
                synthbase_add_note(base, base->cur_pattern, six16th,
                                   base->midi_note, true);
            }
        }
        else if (strncmp("num_patterns", wurds[0], 12) == 0)
        {
            int val = atoi(wurds[1]);
            synthbase_set_num_patterns(base, val);
        }
        else if (strncmp("sustain_note_ms", wurds[0], 15) == 0)
        {
            int sustain_note_ms = atoi(wurds[1]);
            if (sustain_note_ms > 0)
                synthbase_set_sustain_note_ms(base, sustain_note_ms);
        }
        else if (strncmp("midi_note", wurds[0], 8) == 0)
        {
            int midi_note = atoi(wurds[1]);
            synthbase_set_midi_note(base, midi_note);
        }
        else if (strncmp("oct", wurds[0], 3) == 0 ||
                 strncmp("octave", wurds[0], 6) == 0)
        {
            int oct = atoi(wurds[1]);
            synthbase_set_octave(base, oct);
        }
        else if (strncmp("import", wurds[0], 6) == 0)
        {
            printf("Importing file %s\n", wurds[1]);
            synthbase_import_midi_from_file(base, wurds[3]);
        }
        else if (strncmp("keys", wurds[0], 4) == 0)
        {
            keys(soundgen_num);
        }
        else if (strncmp("genone", wurds[0], 6) == 0 ||
                 strncmp("genonce", wurds[0], 7) == 0)
        {
            int src = atoi(wurds[1]);
            synthbase_generate_pattern(base, src, false, false);
        }
        else if (strncmp("gensave", wurds[0], 7) == 0)
        {
            int src = atoi(wurds[1]);
            synthbase_generate_pattern(base, src, false, true);
        }
        else if (strncmp("generate", wurds[0], 8) == 0 ||
                 strncmp("genkeep", wurds[0], 7) == 0 ||
                 strncmp("gen", wurds[0], 3) == 0)
        {
            int src = atoi(wurds[1]);
            synthbase_generate_pattern(base, src, true, false);
        }

        else if (strncmp("midi", wurds[0], 4) == 0)
        {
            mixr->midi_control_destination = SYNTH;
            mixr->active_midi_soundgen_num = soundgen_num;
        }

        else if (strncmp("multi", wurds[0], 5) == 0)
        {
            bool b = atoi(wurds[1]);
            synthbase_set_multi_pattern_mode(base, b);
            printf("Synth multi mode : %s\n",
                   base->multi_pattern_mode ? "true" : "false");
        }

        else if (strncmp("reset", wurds[0], 5) == 0)
        {
            if (strncmp("all", wurds[1], 3) == 0)
            {
                synthbase_reset_pattern_all(base);
            }
            else
            {
                int pattern_num = atoi(wurds[1]);
                synthbase_reset_pattern(base, pattern_num);
            }
        }
        else if (strncmp("sample_rate", wurds[0], 11) == 0)
        {
            int sample_rate = atoi(wurds[1]);
            synthbase_set_sample_rate(base, sample_rate);
        }
        else if (strncmp("switch", wurds[0], 6) == 0 ||
                 strncmp("cur_pattern", wurds[2], 9) == 0)
        {
            int pattern_num = atoi(wurds[1]);
            synthbase_switch_pattern(base, pattern_num);
        }
        else if (strncmp("up", wurds[0], 2) == 0)
        {
            for (int i = 0; i < base->num_patterns; i++)
                synthbase_change_octave_pattern(base, i, 1);
        }
        else if (strncmp("down", wurds[0], 4) == 0)
        {
            for (int i = 0; i < base->num_patterns; i++)
                synthbase_change_octave_pattern(base, i, 0);
        }
        else
            cmd_found = false;
    }
    else
    {
        if (is_valid_pattern_num(base, pattern_num))
        {
            if (strncmp("cp", wurds[0], 2) == 0)
            {
                int sg2 = 0;
                int pattern_num2 = 0;
                sscanf(wurds[1], "%d:%d", &sg2, &pattern_num2);
                printf("CP'ing %d:%d to %d:%d\n", soundgen_num, pattern_num,
                       sg2, pattern_num2);
                if (mixer_is_valid_soundgen_num(mixr, sg2) &&
                    is_synth(mixr->sound_generators[sg2]))
                {
                    synthbase *sb2 = get_synthbase(mixr->sound_generators[sg2]);

                    if (is_valid_pattern_num(base, pattern_num) &&
                        is_valid_pattern_num(sb2, pattern_num2))
                    {

                        midi_event *loop_copy =
                            synthbase_get_pattern(base, pattern_num);
                        synthbase_set_pattern(sb2, pattern_num2, loop_copy);
                    }
                }
            }
            else if (strncmp("dupe", wurds[0], 4) == 0)
            {
                int new_pattern_num = synthbase_add_pattern(base);
                synthbase_dupe_pattern(&base->patterns[pattern_num],
                                       &base->patterns[new_pattern_num]);
            }
            else if (strncmp("numloops", wurds[0], 8) == 0)
            {
                int numloops = atoi(wurds[1]);
                if (numloops != 0)
                {
                    synthbase_set_pattern_loop_num(base, pattern_num, numloops);
                    printf("NUMLOOPS Now %d\n", numloops);
                }
            }
            else if (strncmp("up", wurds[0], 2) == 0)
            {
                synthbase_change_octave_pattern(base, pattern_num, 1);
            }
            else if (strncmp("down", wurds[0], 4) == 0)
            {
                synthbase_change_octave_pattern(base, pattern_num, 0);
            }
            else if (strncmp("quantize", wurds[0], 8) == 0)
            {
                midi_pattern_quantize(&base->patterns[pattern_num]);
            }
            else
            {
                soundgenerator *sg = mixr->sound_generators[soundgen_num];
                check_and_set_pattern(sg, pattern_num, NOTE_PATTERN, &wurds[0],
                                      num_wurds);
            }
        }
    }
    return cmd_found;
}
