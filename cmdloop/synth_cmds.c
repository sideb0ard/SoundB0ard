#include <stdlib.h>
#include <string.h>

#include "cmdloop.h"
#include "digisynth.h"
#include "dxsynth.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"

extern mixer *mixr;
bool parse_synth_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("syn", wurds[0], 3) == 0)
    {
        if (strncmp(wurds[1], "ls", 2) == 0)
        {
            minisynth_list_presets();
            return true;
        }

        int soundgen_num = atoi(wurds[1]);

        if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
            is_synth(mixr->sound_generators[soundgen_num]))
        {
            // ALL THE SYNTHBASE COMMONALITY FIRST, then SPECIFICS synths below

            synthbase *base =
                get_synthbase(mixr->sound_generators[soundgen_num]);

            printf("YO! here with '%s'\n", wurds[2]);
            if (strncmp("add", wurds[2], 3) == 0)
            {
                if (strncmp("melody", wurds[3], 6) == 0 ||
                    strncmp("pattern", wurds[3], 7) == 0)
                {
                    int new_melody_num = synthbase_add_melody(base);
                    if (num_wurds > 4)
                    {
                        char_melody_to_midi_melody(base, new_melody_num, wurds,
                                                   4, num_wurds);
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
                printf("CP'ing %d:%d to %d:%d\n", soundgen_num, pattern_num,
                       sg2, pattern_num2);
                if (mixer_is_valid_soundgen_num(mixr, sg2) &&
                    is_synth(mixr->sound_generators[sg2]))
                {
                    printf("ALL VALID!\n");
                    synthbase *sb2 = get_synthbase(mixr->sound_generators[sg2]);

                    if (is_valid_melody_num(base, pattern_num) &&
                        is_valid_melody_num(sb2, pattern_num2))
                    {

                        printf("Copying SYNTH "
                               "pattern from "
                               "%d:%d to "
                               "%d:%d!\n",
                               soundgen_num, pattern_num, sg2, pattern_num2);

                        midi_event *loop_copy = synthbase_get_pattern(base, pattern_num);
                        synthbase_set_pattern(sb2, pattern_num2, loop_copy);
                    }
                }
            }
            else if (strncmp("note_on", wurds[2], 7) == 0)
            {
                for (int i = 3; i < num_wurds; i++)
                {
                    int six16th = atoi(wurds[i]) % 16;
                    printf("NOTE ON!! 16th:%d\n", six16th);
                    synthbase_add_note(base, base->cur_melody, six16th,
                                       base->root_midi_note);
                }
            }
            else if (strncmp("sustain_note_ms", wurds[2], 15) == 0)
            {
                printf("ME!\n");
                int sustain_note_ms = atoi(wurds[3]);
                if (sustain_note_ms > 0)
                    synthbase_set_sustain_note_ms(base, sustain_note_ms);
            }
            else if (strncmp("rand_note", wurds[2], 8) == 0)
            {
                synthbase_set_rand_key(base);
            }
            else if (strncmp("root_note", wurds[2], 8) == 0)
            {
                int root_key = atoi(wurds[3]);
                synthbase_set_root_key(base, root_key);
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
                    if (tick < PPBAR)
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
            else if (strncmp("generate", wurds[2], 8) == 0 ||
                     strncmp("gen", wurds[2], 3) == 0)
            {
                if (strncmp("src", wurds[3], 3) == 0)
                {
                    int src = atoi(wurds[4]);
                    printf("GEN SRC! %d\n", src);
                    synthbase_set_generate_src(base, src);
                }
                else
                {
                    synthbase_generate_melody(base);
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
                    synthbase_nudge_melody(base, base->cur_melody, sixteenth);
                }
            }
            else if (strncmp("quantize", wurds[2], 8) == 0)
            {
                int melody_num = atoi(wurds[3]);
                if (is_valid_melody_num(base, melody_num))
                {
                    printf("QuantiZe!\n");
                    midi_melody_quantize(&base->melodies[base->cur_melody]);
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
            else if (strncmp("sample_rate", wurds[2], 11) == 0)
            {
                int sample_rate = atoi(wurds[3]);
                synthbase_set_sample_rate(base, sample_rate);
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
                            synthbase_set_melody_loop_num(base, melody_num,
                                                          numloops);
                            printf("NUMLOO"
                                   "PS "
                                   "Now "
                                   "%d\n",
                                   numloops);
                        }
                    }
                    else if (strncmp("add", wurds[3], 3) == 0)
                    {
                        char_melody_to_midi_melody(base, melody_num, wurds, 4,
                                                   num_wurds);
                    }
                    else if (strncmp("mv", wurds[3], 2) == 0)
                    {
                        int fromtick = atoi(wurds[4]);
                        int totick = atoi(wurds[5]);
                        printf("MV'ing "
                               "note\n");
                        synthbase_mv_note(base, melody_num, fromtick, totick);
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
                            synthbase_add_micro_note(base, melody_num, tick,
                                                     midi_note);
                        }
                    }
                    else if (strncmp("melody", wurds[3], 6) == 0 ||
                             strncmp("pattern", wurds[3], 7) == 0)
                    {
                        synthbase_reset_melody(base, melody_num);
                        char_melody_to_midi_melody(base, melody_num, wurds, 4,
                                                   num_wurds);
                    }
                    else if (strncmp("mmv", wurds[3], 2) == 0)
                    {
                        int fromtick = atoi(wurds[4]);
                        int totick = atoi(wurds[5]);
                        printf("MMV'ing "
                               "note\n");
                        synthbase_mv_micro_note(base, melody_num, fromtick,
                                                totick);
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
                        synthbase_change_octave_melody(base, melody_num, 1);
                    }
                    else if (strncmp("down", wurds[3], 4) == 0)
                    {
                        synthbase_change_octave_melody(base, melody_num, 0);
                    }
                }

                if (mixr->sound_generators[soundgen_num]->type == DXSYNTH_TYPE)
                {
                    dxsynth *dx =
                        (dxsynth *)mixr->sound_generators[soundgen_num];
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
                    else if (strncmp("arp", wurds[2], 3) == 0)
                    {
                        ms->base.recording = false;
                        minisynth_set_arpeggiate(ms, 1 - ms->m_arp.active);
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
                        (digisynth *)mixr->sound_generators[soundgen_num];
                    (void)ds;
                }
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
    // else if (strncmp("gen", wurds[2], 4) == 0)
    //{
    //    if (strncmp(wurds[3], "source", 6) == 0 ||
    //        strncmp(wurds[3], "src", 3) == 0)
    //    {
    //        int src = atoi(wurds[4]);
    //        printf("Changing BITSHIFT SRC: %d\n", src);
    //        minisynth_set_generate_src(ms, src);
    //    }
    //    else
    //    {
    //        bool b = atof(wurds[3]);
    //        printf("Minisynth BITSHIFT %s!\n", b ? "true" : "false");
    //        minisynth_set_generate(ms, b);
    //    }
    //    return true;
    //}
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
