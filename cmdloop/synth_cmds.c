#include <stdlib.h>
#include <string.h>

#include <cmdloop.h>
#include <cmdloop/sequence_engine_cmds.h>
#include <digisynth.h>
#include <dxsynth.h>
#include <midimaaan.h>
#include <minisynth.h>
#include <mixer.h>
#include <pattern_parser.h>
#include <synth_cmds.h>
#include <utils.h>

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
                sequence_engine_list_presets(MINISYNTH_TYPE);
            }
             else if (strncmp("dx", wurds[0], 2) == 0)
            {
                sequence_engine_list_presets(DXSYNTH_TYPE);
            }
            return true;
        }

        int soundgen_num = -1;
        int target_pattern_num = -1;
        sscanf(wurds[1], "%d:%d", &soundgen_num, &target_pattern_num);

        if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
            is_synth(mixr->sound_generators[soundgen_num]))
        {
            if (parse_sequence_engine_cmd(soundgen_num, target_pattern_num,
                                          &wurds[2], num_wurds - 2))
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
                if (parse_digisynth_settings_change(ds, wurds))
                {
                    digisynth_update(ds);
                }
            }
        }
        return true;
    }
    return false;
}

bool parse_dxsynth_settings_change(dxsynth *dx, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("algo", wurds[2], 4) == 0)
    {
        int algo = atoi(wurds[3]);
        dxsynth_set_voice_mode(dx, algo);
        return true;
    }
    else if (strncmp("open", wurds[2], 4) == 0 ||
             strncmp("load", wurds[2], 4) == 0)
    {
        dxsynth_load_settings(dx, wurds[3]);
        return true;
    }
    else if (strncmp("save", wurds[2], 4) == 0)
    {
        dxsynth_save_settings(dx, wurds[3]);
        return true;
    }
    else if (strncmp("porta", wurds[2], 5) == 0)
    {
        double ms = atof(wurds[3]);
        dxsynth_set_portamento_time_ms(dx, ms);
        return true;
    }
    else if (strncmp("rand", wurds[2], 4) == 0)
    {
        dxsynth_rand_settings(dx);
        return true;
    }
    else if (strncmp("pitchrange", wurds[2], 10) == 0)
    {
        int val = atoi(wurds[3]);
        dxsynth_set_pitchbend_range(dx, val);
        return true;
    }
    else if (strncmp("vel2att", wurds[2], 10) == 0)
    {
        bool val = atoi(wurds[3]);
        dxsynth_set_velocity_to_attack_scaling(dx, val);
        return true;
    }
    else if (strncmp("note2dec", wurds[2], 10) == 0)
    {
        bool val = atoi(wurds[3]);
        dxsynth_set_note_number_to_decay_scaling(dx, val);
        return true;
    }
    else if (strncmp("reset2zero", wurds[2], 10) == 0)
    {
        bool val = atoi(wurds[3]);
        dxsynth_set_reset_to_zero(dx, val);
        return true;
    }
    else if (strncmp("legato", wurds[2], 6) == 0)
    {
        bool val = atoi(wurds[3]);
        dxsynth_set_legato_mode(dx, val);
        return true;
    }
    else if (strncmp("l1_int", wurds[2], 14) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_lfo1_intensity(dx, val);
        return true;
    }
    else if (strncmp("l1_rate", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        if (strncmp("sync", wurds[3], 4) == 0)
            val = mixer_get_hz_per_bar(mixr);
        int division = atoi(wurds[4]);
        if (division > 0)
            val *= division;
        dxsynth_set_lfo1_rate(dx, val);
        return true;
    }
    else if (strncmp("l1_wav", wurds[2], 13) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        dxsynth_set_lfo1_waveform(dx, val);
        return true;
    }
    else if (strncmp("l1_dest1", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
        dxsynth_set_lfo1_mod_dest(dx, 1, dest);
        return true;
    }
    else if (strncmp("l1_dest2", wurds[2], 9) == 0)
    {
        unsigned int dest = atoi(wurds[3]);
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
        dxsynth_set_lfo1_mod_dest(dx, 4, dest);
        return true;
    }
    else if (strncmp("o1wav", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        dxsynth_set_op_waveform(dx, 1, val);
        return true;
    }
    else if (strncmp("o1rat", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_ratio(dx, 1, val);
        return true;
    }
    else if (strncmp("o1det", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_detune(dx, 1, val);
        return true;
    }
    else if (strncmp("e1att", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_attack_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("e1dec", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_decay_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("e1sus", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_sustain_lvl(dx, 1, val);
        return true;
    }
    else if (strncmp("e1rel", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_release_ms(dx, 1, val);
        return true;
    }
    else if (strncmp("op1out", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_output_lvl(dx, 1, val);
        return true;
    }

    else if (strncmp("o2wav", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        dxsynth_set_op_waveform(dx, 2, val);
        return true;
    }
    else if (strncmp("o2rat", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_ratio(dx, 2, val);
        return true;
    }
    else if (strncmp("o2det", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_detune(dx, 2, val);
        return true;
    }
    else if (strncmp("e2att", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_attack_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("e2dec", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_decay_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("e2sus", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_sustain_lvl(dx, 2, val);
        return true;
    }
    else if (strncmp("e2rel", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_release_ms(dx, 2, val);
        return true;
    }
    else if (strncmp("op2out", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_output_lvl(dx, 2, val);
        return true;
    }

    else if (strncmp("o3wav", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        dxsynth_set_op_waveform(dx, 3, val);
        return true;
    }
    else if (strncmp("o3rat", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_ratio(dx, 3, val);
        return true;
    }
    else if (strncmp("o3det", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_detune(dx, 3, val);
        return true;
    }
    else if (strncmp("e3att", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_attack_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("e3dec", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_decay_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("e3sus", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_sustain_lvl(dx, 3, val);
        return true;
    }
    else if (strncmp("e3rel", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_release_ms(dx, 3, val);
        return true;
    }
    else if (strncmp("op3out", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_output_lvl(dx, 3, val);
        return true;
    }

    else if (strncmp("o4wav", wurds[2], 7) == 0)
    {
        unsigned int val = atoi(wurds[3]);
        dxsynth_set_op_waveform(dx, 4, val);
        return true;
    }
    else if (strncmp("o4rat", wurds[2], 8) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_ratio(dx, 4, val);
        return true;
    }
    else if (strncmp("o4det", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_detune(dx, 4, val);
        return true;
    }
    else if (strncmp("e4att", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_attack_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("e4dec", wurds[2], 10) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_decay_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("e4sus", wurds[2], 13) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_sustain_lvl(dx, 4, val);
        return true;
    }
    else if (strncmp("e4rel", wurds[2], 12) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_eg_release_ms(dx, 4, val);
        return true;
    }
    else if (strncmp("op4out", wurds[2], 9) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op_output_lvl(dx, 4, val);
        return true;
    }
    else if (strncmp("op4fb", wurds[2], 11) == 0)
    {
        double val = atof(wurds[3]);
        dxsynth_set_op4_feedback(dx, val);
        return true;
    }

    return false;
}

bool parse_digisynth_settings_change(digisynth *ds, char wurds[][SIZE_OF_WURD])
{
    bool to_update = true;
    if (strncmp("load", wurds[2], 4) == 0 || strncmp("open", wurds[2], 4) == 0)
    {
        printf("Loading new file! %s\n", wurds[3]);
        digisynth_load_wav(ds, wurds[3]);
    }
    else
        to_update = false;
    return to_update;
}

bool parse_minisynth_settings_change(minisynth *ms, char wurds[][SIZE_OF_WURD])
{
    bool to_update = true;
    double val = atof(wurds[3]);

    if (strncmp("eg1_attack", wurds[2], 11) == 0)
        minisynth_set_eg_attack_time_ms(ms, 1, val);
    else if (strncmp("eg1_decay", wurds[2], 10) == 0)
        minisynth_set_eg_decay_time_ms(ms, 1, val);
    else if (strncmp("eg1_osc_en", wurds[2], 10) == 0)
        minisynth_set_eg_osc_enable(ms, 1, val);
    else if (strncmp("eg1_osc_int", wurds[2], 10) == 0)
        minisynth_set_eg_osc_int(ms, 1, val);
    else if (strncmp("eg1_dca_en", wurds[2], 10) == 0)
        minisynth_set_eg_dca_enable(ms, 1, val);
    else if (strncmp("eg1_dca_int", wurds[2], 11) == 0)
        minisynth_set_eg_dca_int(ms, 1, val);
    else if (strncmp("eg1_filter_en", wurds[2], 12) == 0)
        minisynth_set_eg_filter_enable(ms, 1, val);
    else if (strncmp("eg1_filter_int", wurds[2], 14) == 0)
        minisynth_set_eg_filter_int(ms, 1, val);
    else if (strncmp("eg1_release", wurds[2], 13) == 0)
        minisynth_set_eg_release_time_ms(ms, 1, val);
    else if (strncmp("eg1_sustainlvl", wurds[2], 13) == 0)
        minisynth_set_eg_sustain(ms, 1, val);
    else if (strncmp("eg1_sustain", wurds[2], 11) == 0)
        minisynth_set_eg_sustain_override(ms, 1, val);

    else if (strncmp("eg2_attackms", wurds[2], 11) == 0)
        minisynth_set_eg_attack_time_ms(ms, 2, val);
    else if (strncmp("eg2_decayms", wurds[2], 10) == 0)
        minisynth_set_eg_decay_time_ms(ms, 2, val);
    else if (strncmp("eg2_osc_en", wurds[2], 10) == 0)
        minisynth_set_eg_osc_enable(ms, 2, val);
    else if (strncmp("eg2_osc_int", wurds[2], 10) == 0)
        minisynth_set_eg_osc_int(ms, 2, val);
    else if (strncmp("eg2_dca_en", wurds[2], 10) == 0)
        minisynth_set_eg_dca_enable(ms, 2, val);
    else if (strncmp("eg2_dca_int", wurds[2], 11) == 0)
        minisynth_set_eg_dca_int(ms, 2, val);
    else if (strncmp("eg2_filter_en", wurds[2], 12) == 0)
        minisynth_set_eg_filter_enable(ms, 2, val);
    else if (strncmp("eg2_filter_int", wurds[2], 14) == 0)
        minisynth_set_eg_filter_int(ms, 2, val);
    else if (strncmp("eg2_releasems", wurds[2], 13) == 0)
        minisynth_set_eg_release_time_ms(ms, 2, val);
    else if (strncmp("eg2_sustainlvl", wurds[2], 13) == 0)
        minisynth_set_eg_sustain(ms, 2, val);
    else if (strncmp("eg2_sustain", wurds[2], 11) == 0)
        minisynth_set_eg_sustain_override(ms, 2, val);

    else if (strncmp("detune", wurds[2], 6) == 0)
        minisynth_set_detune(ms, val);
    else if (strncmp("fc", wurds[2], 2) == 0)
        minisynth_set_filter_fc(ms, val);
    else if (strncmp("fq", wurds[2], 2) == 0)
        minisynth_set_filter_fq(ms, val);
    else if (strncmp("filter", wurds[2], 4) == 0)
        minisynth_set_filter_type(ms, val);
    else if (strncmp("hard_sync", wurds[2], 9) == 0)
        minisynth_set_hard_sync(ms, val);
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
            val = mixer_get_hz_per_bar(mixr);
        int division = atoi(wurds[4]);
        if (division > 0)
            val *= division;
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
            val = mixer_get_hz_per_bar(mixr);
        int division = atoi(wurds[4]);
        if (division > 0)
            val *= division;
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
    else if (strncmp("osc1", wurds[2], 4) == 0)
        minisynth_set_osc_type(ms, 1, val);
    else if (strncmp("o1amp", wurds[2], 5) == 0)
        minisynth_set_osc_amp(ms, 1, val);
    else if (strncmp("o1cents", wurds[2], 7) == 0)
        minisynth_set_osc_cents(ms, 1, val);
    else if (strncmp("osc2", wurds[2], 4) == 0)
        minisynth_set_osc_type(ms, 2, val);
    else if (strncmp("o2amp", wurds[2], 5) == 0)
        minisynth_set_osc_amp(ms, 2, val);
    else if (strncmp("o2cents", wurds[2], 7) == 0)
        minisynth_set_osc_cents(ms, 2, val);
    else if (strncmp("osc3", wurds[2], 4) == 0)
        minisynth_set_osc_type(ms, 3, val);
    else if (strncmp("osc4", wurds[2], 4) == 0)
        minisynth_set_osc_type(ms, 4, val);
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
    else if (strncmp("default", wurds[2], 7) == 0)
    {
        printf("Loading defaults\n");
        minisynth_load_defaults(ms);
    }
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
