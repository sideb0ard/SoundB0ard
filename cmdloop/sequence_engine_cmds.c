#include <stdlib.h>
#include <string.h>

#include <cmdloop/sequence_engine_cmds.h>
#include <mixer.h>
#include <pattern_parser.h>
#include <sequence_engine.h>
#include <utils.h>

extern mixer *mixr;

bool parse_sequence_engine_cmd(int soundgen_num, int pattern_num,
                               char wurds[][SIZE_OF_WURD], int num_wurds)
{
    sequence_engine *engine =
        get_sequence_engine(mixr->sound_generators[soundgen_num]);

    bool cmd_found = true;
    // sequence_engine specific first, then patterns below
    if (pattern_num == -1)
    {
        if (strncmp("arp_mode", wurds[0], 8) == 0)
        {
            unsigned int mode = atoi(wurds[1]);
            sequence_engine_set_arp_mode(engine, mode);
        }
        else if (strncmp("arp_speed", wurds[0], 9) == 0)
        {
            unsigned int speed = atoi(wurds[1]);
            switch (speed)
            {
            case (32):
                sequence_engine_set_arp_speed(engine, ARP_32);
                break;
            case (24):
                sequence_engine_set_arp_speed(engine, ARP_24);
                break;
            case (16):
                sequence_engine_set_arp_speed(engine, ARP_16);
                break;
            case (12):
                sequence_engine_set_arp_speed(engine, ARP_12);
                break;
            case (8):
                sequence_engine_set_arp_speed(engine, ARP_8);
                break;
            case (6):
                sequence_engine_set_arp_speed(engine, ARP_6);
                break;
            case (4):
                sequence_engine_set_arp_speed(engine, ARP_4);
                break;
            case (3):
                sequence_engine_set_arp_speed(engine, ARP_3);
                break;
            default:
                printf("Speed has to be one of 32,24,16,12,8,6,4 or 3\n");
            }
        }
        else if (strncmp("arp", wurds[0], 3) == 0)
        {
            bool enable = atoi(wurds[1]);
            sequence_engine_enable_arp(engine, enable);
        }
        else if (strncmp("chord_mode", wurds[0], 10) == 0)
        {
            bool b = atoi(wurds[1]);
            sequence_engine_set_chord_mode(engine, b);
        }
        else if (strncmp("follow", wurds[0], 6) == 0)
        {
            bool b = atoi(wurds[1]);
            sequence_engine_set_follow_mixer_chords(engine, b);
        }
        else if (strncmp("single_note_mode", wurds[0], 9) == 0)
        {
            bool b = atoi(wurds[1]);
            sequence_engine_set_single_note_mode(engine, b);
        }
        else if (strncmp("swing", wurds[0], 5) == 0)
        {
            int swing_setting = atoi(wurds[1]);
            sequence_engine_set_swing_setting(engine, swing_setting);
        }
        else if (strncmp("note_on", wurds[0], 7) == 0)
        {
            for (int i = 3; i < num_wurds; i++)
            {
                int six16th = atoi(wurds[i]) % 16;
                sequence_engine_add_note(engine, engine->cur_pattern, six16th,
                                         engine->midi_note_1, 128, true);
            }
        }
        else if (strncmp("num_patterns", wurds[0], 12) == 0)
        {
            int val = atoi(wurds[1]);
            sequence_engine_set_num_patterns(engine, val);
        }
        else if (strncmp("sustain_note_ms", wurds[0], 15) == 0)
        {
            int sustain_note_ms = atoi(wurds[1]);
            if (sustain_note_ms > 0)
                sequence_engine_set_sustain_note_ms(engine, sustain_note_ms);
        }
        else if (strncmp("midi_note_1", wurds[0], 11) == 0)
        {
            int midi_note = atoi(wurds[1]);
            sequence_engine_set_midi_note(engine, 1, midi_note);
        }
        else if (strncmp("midi_note_2", wurds[0], 11) == 0)
        {
            int midi_note = atoi(wurds[1]);
            sequence_engine_set_midi_note(engine, 2, midi_note);
        }
        else if (strncmp("midi_note_3", wurds[0], 11) == 0)
        {
            int midi_note = atoi(wurds[1]);
            sequence_engine_set_midi_note(engine, 3, midi_note);
        }
        else if (strncmp("oct", wurds[0], 3) == 0 ||
                 strncmp("octave", wurds[0], 6) == 0)
        {
            int oct = atoi(wurds[1]);
            sequence_engine_set_octave(engine, oct);
        }
        else if (strncmp("import", wurds[0], 6) == 0)
        {
            printf("Importing file %s\n", wurds[1]);
            sequence_engine_import_midi_from_file(engine, wurds[3]);
        }
        else if (strncmp("keys", wurds[0], 4) == 0)
        {
            keys(soundgen_num);
        }

        else if (strncmp("midi", wurds[0], 4) == 0)
        {
            mixr->midi_control_destination = MIDI_CONTROL_SYNTH_TYPE;
            mixr->active_midi_soundgen_num = soundgen_num;
        }

        else if (strncmp("multi", wurds[0], 5) == 0)
        {
            bool b = atoi(wurds[1]);
            sequence_engine_set_multi_pattern_mode(engine, b);
            printf("Synth multi mode : %s\n",
                   engine->multi_pattern_mode ? "true" : "false");
        }

        else if (strncmp("reset", wurds[0], 5) == 0)
        {
            if (strncmp("all", wurds[1], 3) == 0)
            {
                sequence_engine_reset_pattern_all(engine);
            }
            else
            {
                int pattern_num = atoi(wurds[1]);
                sequence_engine_reset_pattern(engine, pattern_num);
            }
        }
        else if (strncmp("sample_rate", wurds[0], 11) == 0)
        {
            int sample_rate = atoi(wurds[1]);
            sequence_engine_set_sample_rate(engine, sample_rate);
        }
        else if (strncmp("mask_every", wurds[0], 10) == 0)
        {
            int every_n = atoi(wurds[1]);
            sequence_engine_set_mask_every(engine, every_n);
        }
        else if (strncmp("mask", wurds[0], 4) == 0)
        {
            uint16_t mask = mask_from_string(wurds[1]);
            printf("Mask is %d\n", mask);
            int mask_every_n = 1;
            if (strncmp("every", wurds[2], 5) == 0)
            {
                int every_n = atoi(wurds[3]);
                if (every_n)
                    mask_every_n = every_n;
            }
            sequence_engine_set_event_mask(engine, mask, mask_every_n);
        }
        else if (strncmp("switch", wurds[0], 6) == 0 ||
                 strncmp("cur_pattern", wurds[2], 9) == 0)
        {
            int pattern_num = atoi(wurds[1]);
            sequence_engine_switch_pattern(engine, pattern_num);
        }
        else if (strncmp("up", wurds[0], 2) == 0)
        {
            for (int i = 0; i < engine->num_patterns; i++)
                sequence_engine_change_octave_pattern(engine, i, 1);
            sequence_engine_change_octave_midi_notes(engine, UP);
        }
        else if (strncmp("down", wurds[0], 4) == 0)
        {
            for (int i = 0; i < engine->num_patterns; i++)
                sequence_engine_change_octave_pattern(engine, i, 0);
            sequence_engine_change_octave_midi_notes(engine, DOWN);
        }
        else
            cmd_found = false;
    }
    else
    {
        if (is_valid_pattern_num(engine, pattern_num))
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
                    sequence_engine *sb2 =
                        get_sequence_engine(mixr->sound_generators[sg2]);

                    if (is_valid_pattern_num(engine, pattern_num) &&
                        is_valid_pattern_num(sb2, pattern_num2))
                    {

                        midi_event *loop_copy =
                            sequence_engine_get_pattern(engine, pattern_num);
                        pattern_change_info change_info = {
                            .clear_previous = true, .temporary = false};
                        sequence_engine_set_pattern(sb2, pattern_num2,
                                                    change_info, loop_copy);
                    }
                }
            }
            else if (strncmp("dupe", wurds[0], 4) == 0)
            {
                int new_pattern_num = sequence_engine_add_pattern(engine);
                sequence_engine_dupe_pattern(
                    &engine->patterns[pattern_num],
                    &engine->patterns[new_pattern_num]);
            }
            else if (strncmp("numloops", wurds[0], 8) == 0)
            {
                int numloops = atoi(wurds[1]);
                if (numloops != 0)
                {
                    sequence_engine_set_pattern_loop_num(engine, pattern_num,
                                                         numloops);
                    printf("NUMLOOPS Now %d\n", numloops);
                }
            }
            else if (strncmp("up", wurds[0], 2) == 0)
            {
                sequence_engine_change_octave_pattern(engine, pattern_num, 1);
            }
            else if (strncmp("down", wurds[0], 4) == 0)
            {
                sequence_engine_change_octave_pattern(engine, pattern_num, 0);
            }
            else if (strncmp("quantize", wurds[0], 8) == 0)
            {
                midi_pattern_quantize(&engine->patterns[pattern_num]);
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
