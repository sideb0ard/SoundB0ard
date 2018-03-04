#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <basicfilterpass.h>
#include <beatrepeat.h>
#include <bitcrush.h>
#include <distortion.h>
#include <dynamics_processor.h>
#include <envelope_follower.h>
#include <fx_cmds.h>
#include <mixer.h>
#include <modfilter.h>
#include <modular_delay.h>
#include <reverb.h>
#include <stereodelay.h>
#include <waveshaper.h>

extern mixer *mixr;

bool parse_fx_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("bitcrush", wurds[0], 8) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            add_bitcrush_soundgen(mixr->sound_generators[soundgen_num]);
        }
        return true;
    }
    else if (strncmp("sidechain", wurds[0], 9) == 0)
    {
        int dest_soundgen_num = atoi(wurds[1]);
        int src_soundgen_num = atoi(wurds[2]);
        if (mixer_is_valid_soundgen_num(mixr, dest_soundgen_num) &&
            mixer_is_valid_soundgen_num(mixr, src_soundgen_num))
        {
            int fxnum = add_compressor_soundgen(
                mixr->sound_generators[dest_soundgen_num]);
            dynamics_processor *dp =
                (dynamics_processor *)mixr->sound_generators[dest_soundgen_num]
                    ->effects[fxnum];
            dynamics_processor_set_external_source(dp, src_soundgen_num);
            dynamics_processor_set_default_sidechain_params(dp);
        }
        return true;
    }
    else if (strncmp("compressor", wurds[0], 10) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            add_compressor_soundgen(mixr->sound_generators[soundgen_num]);
        }
        return true;
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
        return true;
    }
    else if (strncmp("filter", wurds[0], 6) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        int freq = atoi(wurds[2]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            int f_num = add_basicfilter_soundgen(mixr->sound_generators[soundgen_num]);
            if (freq != 0)
            {
                soundgenerator *sg = mixr->sound_generators[soundgen_num];
                filterpass *fp = (filterpass *) sg->effects[f_num];
                filter_set_fc_control(&fp->m_filter.f, freq);
            }
        }
        return true;
    }
    else if (strncmp("follower", wurds[0], 8) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            add_follower_soundgen(mixr->sound_generators[soundgen_num]);
        }
        return true;
    }
    else if (strncmp("moddelay", wurds[0], 7) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            add_moddelay_soundgen(mixr->sound_generators[soundgen_num]);
        }
        return true;
    }
    else if (strncmp("modfilter", wurds[0], 9) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            add_modfilter_soundgen(mixr->sound_generators[soundgen_num]);
        }
        return true;
    }
    else if (strncmp("reverb", wurds[0], 6) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            add_reverb_soundgen(mixr->sound_generators[soundgen_num]);
        }
        return true;
    }
    else if (strncmp("waveshape", wurds[0], 6) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        int val = atoi(wurds[2]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            int fxnum =
                add_waveshape_soundgen(mixr->sound_generators[soundgen_num]);
            waveshaper *w = (waveshaper *)mixr->sound_generators[soundgen_num]
                                ->effects[fxnum];
            if (val != 0)
                waveshaper_set_stages(w, val);
        }
        return true;
    }
    else if (strncmp("distort", wurds[0], 7) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            add_distortion_soundgen(mixr->sound_generators[soundgen_num]);
        }
        return true;
    }
    else if (strncmp("decimate", wurds[0], 8) == 0)
    {
        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
        {
            add_decimator_soundgen(mixr->sound_generators[soundgen_num]);
        }
        return true;
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
                add_envelope_soundgen(mixr->sound_generators[soundgen_num], e);
            }
        }
        return true;
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
        return true;
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
        return true;
    }

    else if (strncmp("fx", wurds[0], 2) == 0)
    {
        int soundgen_num = -1;
        int fx_num = -1;
        sscanf(wurds[1], "%d:%d", &soundgen_num, &fx_num);
        if (is_valid_fx_num(soundgen_num, fx_num))
        {
            fx *f = mixr->sound_generators[soundgen_num]->effects[fx_num];

            if (strncmp("on", wurds[2], 2) == 0)
                f->enabled = true;
            else if (strncmp("off", wurds[2], 3) == 0)
                f->enabled = false;
            else if (strncmp("toggle", wurds[2], 6) == 0)
                f->enabled = 1 - f->enabled;
            else if (f->type == DELAY)
            {
                // printf("Changing Dulay!\n");
                stereodelay *sd = (stereodelay *)f;
                double val = atof(wurds[3]);
                // keep these strings in sync with status()
                // output
                if (strncmp("delayms", wurds[2], 7) == 0)
                    stereo_delay_set_delay_time_ms(sd, val);
                else if (strncmp("fb", wurds[2], 2) == 0)
                    stereo_delay_set_feedback_percent(sd, val);
                else if (strncmp("ratio", wurds[2], 5) == 0)
                    stereo_delay_set_delay_ratio(sd, val);
                else if (strncmp("wetmx", wurds[2], 5) == 0)
                    stereo_delay_set_wet_mix(sd, val);
                else if (strncmp("mode", wurds[2], 4) == 0)
                    stereo_delay_set_mode(sd, val);
                else
                    printf("<bleurgh!>\n");
            }
            else if (f->type == COMPRESSOR)
            {
                dynamics_processor *dp = (dynamics_processor *)f;
                double val = atof(wurds[3]);
                if (strncmp("inputgain", wurds[2], 9) == 0)
                    dynamics_processor_set_inputgain_db(dp, val);
                else if (strncmp("threshold", wurds[2], 9) == 0)
                    dynamics_processor_set_threshold(dp, val);
                else if (strncmp("attackms", wurds[2], 8) == 0)
                    dynamics_processor_set_attack_ms(dp, val);
                else if (strncmp("releasems", wurds[2], 9) == 0)
                    dynamics_processor_set_release_ms(dp, val);
                else if (strncmp("ratio", wurds[2], 5) == 0)
                    dynamics_processor_set_ratio(dp, val);
                else if (strncmp("outputgain", wurds[2], 10) == 0)
                    dynamics_processor_set_outputgain_db(dp, val);
                else if (strncmp("kneewidth", wurds[2], 9) == 0)
                    dynamics_processor_set_knee_width(dp, val);
                else if (strncmp("lookahead", wurds[2], 9) == 0)
                    dynamics_processor_set_lookahead_delay_ms(dp, val);
                else if (strncmp("stereolink", wurds[2], 9) == 0)
                    dynamics_processor_set_stereo_link(dp, val);
                else if (strncmp("type", wurds[2], 4) == 0)
                    dynamics_processor_set_processor_type(dp, val);
                else if (strncmp("mode", wurds[2], 4) == 0)
                    dynamics_processor_set_time_constant(dp, val);
                else if (strncmp("extsource", wurds[2], 9) == 0)
                    dynamics_processor_set_external_source(dp, val);
                else if (strncmp("def", wurds[2], 3) == 0 ||
                         strncmp("default", wurds[2], 7) == 0)
                    dynamics_processor_set_default_sidechain_params(dp);
            }
            else if (f->type == REVERB)
            {
                reverb *r = (reverb *)f;
                double val = atof(wurds[3]);
                if (strncmp("predelayms", wurds[2], 10) == 0)
                    reverb_set_pre_delay_msec(r, val);
                else if (strncmp("predelayattDb", wurds[2], 13) == 0)
                    reverb_set_pre_delay_atten_db(r, val);
                else if (strncmp("inputLPFg", wurds[2], 9) == 0)
                    reverb_set_input_lpf_g(r, val);
                else if (strncmp("lpf2g2", wurds[2], 6) == 0)
                    reverb_set_lpf2_g2(r, val);
                else if (strncmp("reverbtime", wurds[2], 10) == 0)
                    reverb_set_rt60(r, val);
                else if (strncmp("wetmx", wurds[2], 5) == 0)
                    reverb_set_wet_pct(r, val);
                else if (strncmp("APF1delayms", wurds[2], 11) == 0)
                    reverb_set_apf_delay_msec(r, 1, val);
                else if (strncmp("APF1g", wurds[2], 5) == 0)
                    reverb_set_apf_g(r, 1, val);
                else if (strncmp("APF2delayms", wurds[2], 11) == 0)
                    reverb_set_apf_delay_msec(r, 2, val);
                else if (strncmp("APF2g", wurds[2], 5) == 0)
                    reverb_set_apf_g(r, 2, val);
                else if (strncmp("APF3delayms", wurds[2], 11) == 0)
                    reverb_set_apf_delay_msec(r, 3, val);
                else if (strncmp("APF3g", wurds[2], 5) == 0)
                    reverb_set_apf_g(r, 3, val);
                else if (strncmp("APF4delayms", wurds[2], 11) == 0)
                    reverb_set_apf_delay_msec(r, 4, val);
                else if (strncmp("APF4g", wurds[2], 5) == 0)
                    reverb_set_apf_g(r, 4, val);
                else if (strncmp("comb1delayms", wurds[2], 12) == 0)
                    reverb_set_comb_delay_msec(r, 1, val);
                else if (strncmp("comb2delayms", wurds[2], 12) == 0)
                    reverb_set_comb_delay_msec(r, 2, val);
                else if (strncmp("comb3delayms", wurds[2], 12) == 0)
                    reverb_set_comb_delay_msec(r, 3, val);
                else if (strncmp("comb4delayms", wurds[2], 12) == 0)
                    reverb_set_comb_delay_msec(r, 4, val);
                else if (strncmp("comb5delayms", wurds[2], 12) == 0)
                    reverb_set_comb_delay_msec(r, 5, val);
                else if (strncmp("comb6delayms", wurds[2], 12) == 0)
                    reverb_set_comb_delay_msec(r, 6, val);
                else if (strncmp("comb7delayms", wurds[2], 12) == 0)
                    reverb_set_comb_delay_msec(r, 7, val);
                else if (strncmp("comb8delayms", wurds[2], 12) == 0)
                    reverb_set_comb_delay_msec(r, 8, val);
            }
            else if (f->type == WAVESHAPER)
            {
                waveshaper *ws = (waveshaper *)f;
                double val = atof(wurds[3]);
                if (strncmp("k_pos", wurds[2], 5) == 0)
                    waveshaper_set_arc_tan_k_pos(ws, val);
                else if (strncmp("k_neg", wurds[2], 5) == 0)
                    waveshaper_set_arc_tan_k_neg(ws, val);
                else if (strncmp("stages", wurds[2], 5) == 0)
                    waveshaper_set_stages(ws, val);
                else if (strncmp("invert", wurds[2], 5) == 0)
                    waveshaper_set_invert_stages(ws, val);
            }
            else if (f->type == BITCRUSH)
            {
                bitcrush *bc = (bitcrush *)f;
                double val = atof(wurds[3]);
                if (strncmp("bitdepth", wurds[2], 8) == 0)
                    bitcrush_set_bitdepth(bc, val);
                else if (strncmp("bitrate", wurds[2], 7) == 0)
                    bitcrush_set_bitrate(bc, val);
                else if (strncmp("sample_hold_freq", wurds[2], 16) == 0)
                    bitcrush_set_sample_hold_freq(bc, val);
            }
            else if (f->type == BASICFILTER)
            {
                filterpass *fp = (filterpass *)f;
                double val = atof(wurds[3]);
                if (strncmp("freq", wurds[2], 4) == 0)
                    filter_set_fc_control(&fp->m_filter.f, val);
                else if (strncmp("q", wurds[2], 4) == 0)
                    moog_set_qcontrol(&fp->m_filter.f, val);
                else if (strncmp("type", wurds[2], 4) == 0)
                    filter_set_type(&fp->m_filter.f, val);
                else if (strncmp("lfo1_active", wurds[2], 11) == 0)
                    filterpass_set_lfo_active(fp, 1, val);
                else if (strncmp("lfo1_type", wurds[2], 9) == 0)
                    filterpass_set_lfo_type(fp, 1, val);
                else if (strncmp("lfo1_amp", wurds[2], 9) == 0)
                    filterpass_set_lfo_amp(fp, 1, val);
                else if (strncmp("lfo1_rate", wurds[2], 9) == 0)
                    filterpass_set_lfo_rate(fp, 1, val);
                else if (strncmp("lfo2_active", wurds[2], 11) == 0)
                    filterpass_set_lfo_active(fp, 2, val);
                else if (strncmp("lfo2_type", wurds[2], 9) == 0)
                    filterpass_set_lfo_type(fp, 2, val);
                else if (strncmp("lfo2_amp", wurds[2], 9) == 0)
                    filterpass_set_lfo_amp(fp, 2, val);
                else if (strncmp("lfo2_rate", wurds[2], 9) == 0)
                    filterpass_set_lfo_rate(fp, 3, val);
            }
            else if (f->type == BEATREPEAT)
            {
                beatrepeat *br = (beatrepeat *)f;
                double val = atof(wurds[3]);
                if (strncmp("numbeats", wurds[2], 8) == 0)
                    beatrepeat_change_num_beats_to_repeat(br, val);
                else if (strncmp("sixteenth", wurds[2], 9) == 0)
                    beatrepeat_change_selected_sixteenth(br, val);
            }
            else if (f->type == DISTORTION)
            {
                distortion *d = (distortion *)f;
                double val = atof(wurds[3]);
                if (strncmp("threshold", wurds[2], 9) == 0)
                    distortion_set_threshold(d, val);
            }
            else if (f->type == ENVELOPEFOLLOWER)
            {
                envelope_follower *ef = (envelope_follower *)f;
                double val = atof(wurds[3]);
                if (strncmp("pregain", wurds[2], 4) == 0)
                    envelope_follower_set_pregain_db(ef, val);
                else if (strncmp("threshold", wurds[2], 9) == 0)
                    envelope_follower_set_threshold(ef, val);
                else if (strncmp("attackms", wurds[2], 8) == 0)
                    envelope_follower_set_attack_ms(ef, val);
                else if (strncmp("releasems", wurds[2], 9) == 0)
                    envelope_follower_set_release_ms(ef, val);
                else if (strncmp("q", wurds[2], 1) == 0)
                    envelope_follower_set_q(ef, val);
                else if (strncmp("mode", wurds[2], 4) == 0)
                    envelope_follower_set_time_constant(ef, val);
                else if (strncmp("dir", wurds[2], 4) == 0)
                    envelope_follower_set_direction(ef, val);
            }

            else if (f->type == MODDELAY)
            {
                mod_delay *md = (mod_delay *)f;
                double val = atof(wurds[3]);
                if (strncmp("depth", wurds[2], 5) == 0)
                {
                    mod_delay_set_depth(md, val);
                }
                else if (strncmp("rate", wurds[2], 4) == 0)
                {
                    mod_delay_set_rate(md, val);
                }
                else if (strncmp("fb", wurds[2], 8) == 0)
                {
                    mod_delay_set_feedback_percent(md, val);
                }
                else if (strncmp("offset", wurds[2], 12) == 0)
                {
                    mod_delay_set_chorus_offset(md, val);
                }
                else if (strncmp("type", wurds[2], 7) == 0)
                {
                    mod_delay_set_mod_type(md, (unsigned int)val);
                }
                else if (strncmp("lfo", wurds[2], 7) == 0)
                {
                    mod_delay_set_lfo_type(md, (unsigned int)val);
                }
            }
            else if (f->type == MODFILTER)
            {
                modfilter *mf = (modfilter *)f;
                double val = atof(wurds[3]);
                if (strncmp("depthfc", wurds[2], 7) == 0)
                {
                    modfilter_set_mod_depth_fc(mf, val);
                }
                else if (strncmp("ratefc", wurds[2], 6) == 0)
                {
                    modfilter_set_mod_rate_fc(mf, val);
                }
                else if (strncmp("depthq", wurds[2], 6) == 0)
                {
                    modfilter_set_mod_depth_q(mf, val);
                }
                else if (strncmp("rateq", wurds[2], 5) == 0)
                {
                    modfilter_set_mod_rate_q(mf, val);
                }
                else if (strncmp("phase", wurds[2], 8) == 0)
                {
                    modfilter_set_lfo_phase(mf, val);
                }
                else if (strncmp("lfo", wurds[2], 3) == 0)
                {
                    modfilter_set_lfo_waveform(mf, val);
                }
            }
        }
        return true;
    }
    return false;
}

bool is_valid_fx_num(int soundgen_num, int fx_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
    {
        if (mixr->sound_generators[soundgen_num]->effects_num > 0 &&
            fx_num >= 0 &&
            fx_num < mixr->sound_generators[soundgen_num]->effects_num)
        {
            return true;
        }
    }
    printf("FX %d isn't valid\n", fx_num);
    return false;
}
