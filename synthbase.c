#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "bitshift.h"
#include "defjams.h"
#include "euclidean.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "synthbase.h"
#include "utils.h"
#include <pattern_parser.h>

extern const int key_midi_mapping[NUM_KEYS];
extern const char *key_names[NUM_KEYS];
extern const compat_key_list compat_keys[NUM_KEYS];

const char *s_arp_mode[] = {"UP", "DOWN", "UPDOWN", "RAND"};
const char *s_arp_speed[] = {"32", "16", "8", "4"};

extern mixer *mixr;

void synthbase_init(synthbase *base, void *parent,
                    unsigned int parent_synth_type)
{
    base->parent = parent;
    base->parent_synth_type = parent_synth_type;

    base->num_patterns = 1;
    base->max_generation = 0;

    base->multi_pattern_mode = true;
    base->cur_pattern_iteration = 1;
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++)
        base->pattern_multiloop_count[i] = 1;

    base->sample_rate = 44100;
    base->sample_rate_counter = 0;

    base->midi_note_1 = 24; // C0
    base->midi_note_2 = 32; // G#0
    base->midi_note_3 = 31; // G0

    base->octave = 1;

    base->arp.enable = false;
    base->arp.direction = UP;
    base->arp.mode = ARP_UP;
    base->arp.speed = ARP_16;
    for (int i = 0; i < MAX_NOTES_ARP; i++)
        base->arp.last_midi_notes[i] = -1;

    base->sustain_note_ms = 200;
    base->single_note_mode = false;
}

void synthbase_generate_pattern(synthbase *base, int gen_src, bool keep_note,
                                bool save_pattern)
{
    if (mixer_is_valid_seq_gen_num(mixr, gen_src))
    {
        if (save_pattern)
        {
            synthbase_set_backup_mode(base, true);
            base->restore_pending = true;
        }

        // synthbase_clear_pattern_ready_for_new_one(base, base->cur_pattern);

        sequence_generator *sg = mixr->sequence_generators[gen_src];
        uint16_t bits = sg->generate(sg, NULL);

        synthbase_apply_bit_pattern(base, bits, keep_note, false);
    }
}

void synthbase_gen_rec(synthbase *base, int start_idx, int end_idx,
                       int midi_note, float amp)
{
    printf("GEN_REC! start_idx:%d end_idx:%d midi_note:%d amp:%f\n", start_idx,
           end_idx, midi_note, amp);
    midi_event *midi_pattern = base->patterns[base->cur_pattern];
    int middle = (start_idx + end_idx) / 2;
    if (amp > 70)
    {
        synthbase_gen_rec(base, start_idx, middle, midi_note, amp * 0.75);
        printf("Adding note:%d at pos:%d amp:%f\n", midi_note, middle, amp);
        synthbase_add_micro_note(base, base->cur_pattern, middle, midi_note, amp,
                           true);
        synthbase_gen_rec(base, middle, end_idx, midi_note, amp * 0.75);
    }
}

void synthbase_generate_recursive_pattern(synthbase *base)
{
    printf("woof!\n");
    synthbase_stop(base);
    midi_event *midi_pattern = base->patterns[base->cur_pattern];
    clear_pattern(midi_pattern);

    synthbase_gen_rec(base, 0, PPBAR - 1, base->midi_note_1, 128);
}

void synthbase_apply_bit_pattern(synthbase *base, uint16_t bit_pattern,
                                 bool keep_note, bool riff)
{
    synthbase_stop(base);

    midi_event *midi_pattern = base->patterns[base->cur_pattern];
    clear_pattern(midi_pattern);

    chord_midi_notes chnotes = {0};
    get_midi_notes_from_chord(mixr->chord, mixr->chord_type,
                              synthbase_get_octave(base), &chnotes);

    int multiplier = PPSIXTEENTH;
    int midi_note = 0;
    int midi_tick = 0;

    int patternlen = 16;
    for (int i = 0; i < patternlen; i++)
    {
        int shift_by = patternlen - 1 - i;
        if (bit_pattern & (1 << shift_by))
        {
            if (riff)
            {
                if (i < 12)
                    midi_note = base->midi_note_1;
                else if (i < 14)
                    midi_note = base->midi_note_2;
                else
                    midi_note = base->midi_note_3;
            }
            else if (base->single_note_mode)
                midi_note = base->midi_note_1;
            else if (i == 0 || i == patternlen - 1)
                midi_note = chnotes.root;
            else
            {
                int randy = rand() % 3;
                switch (randy)
                {
                case (0):
                    midi_note = chnotes.root;
                    break;
                case (1):
                    midi_note = chnotes.third;
                    break;
                case (2):
                    midi_note = chnotes.fifth;
                    break;
                }
            }

            if (rand() % 100 > 90)
                midi_note += 12; // up an octave

            int velocity = (rand() % 100) + 28;
            midi_tick = multiplier * i;
            if (midi_tick % PPQN == 0)
                velocity = 128;

            int hold_time_ms = (rand() % 2000) + 130;
            midi_event ev = {.event_type = MIDI_ON,
                             .data1 = midi_note,
                             .data2 = velocity,
                             .hold = hold_time_ms};
            if (!keep_note)
                ev.delete_after_use = true;

            midi_pattern[midi_tick] = ev;
        }
    }
}

void synthbase_set_sample_rate(synthbase *base, int sample_rate)
{
    // does sample and hold to sample down
    printf("Chh-ch-changing SAMPLE_RATE!: %d\n", sample_rate);
    base->sample_rate = sample_rate;
    base->sample_rate_ratio = SAMPLE_RATE / (double)sample_rate;
    base->sample_rate_counter = 0;
}

void synthbase_set_multi_pattern_mode(synthbase *ms, bool pattern_mode)
{
    ms->multi_pattern_mode = pattern_mode;
    ms->cur_pattern_iteration = ms->pattern_multiloop_count[ms->cur_pattern];
}

void synthbase_set_pattern_loop_num(synthbase *self, int pattern_num,
                                    int loop_num)
{
    self->pattern_multiloop_count[pattern_num] = loop_num;
}

int synthbase_add_pattern(synthbase *ms) { return ms->num_patterns++; }

void synthbase_dupe_pattern(midi_pattern *from, midi_pattern *to)
{
    for (int i = 0; i < PPBAR; i++)
        (*to)[i] = (*from)[i];
}

void synthbase_switch_pattern(synthbase *ms, unsigned int pattern_num)
{
    if (pattern_num < (unsigned)ms->num_patterns)
        ms->cur_pattern = pattern_num;
}

void synthbase_reset_pattern_all(synthbase *ms)
{
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++)
    {
        synthbase_reset_pattern(ms, i);
    }
}

void synthbase_stop(synthbase *base)
{
    if (base->parent_synth_type == MINISYNTH_TYPE)
    {
        minisynth *p = (minisynth *)base->parent;
        minisynth_stop(p);
    }
    else if (base->parent_synth_type == DXSYNTH_TYPE)
    {
        dxsynth *p = (dxsynth *)base->parent;
        dxsynth_stop(p);
    }
    else if (base->parent_synth_type == DIGISYNTH_TYPE)
    {
        digisynth *d = (digisynth *)base->parent;
        digisynth_stop(d);
    }
}

void synthbase_reset_pattern(synthbase *base, unsigned int pattern_num)
{
    synthbase_stop(base);

    if (pattern_num < MAX_NUM_MIDI_LOOPS)
    {
        memset(base->patterns[pattern_num], 0, sizeof(midi_pattern));
    }
}

// sound generator interface //////////////
void synthbase_status(synthbase *base, wchar_t *status_string)
{
    wchar_t scratch[256] = {0};
    wchar_t patternstr[33] = {0};

    swprintf(scratch, 255,
             L"\nsingle_note_mode:%d chord_mode:%d octave:%d sustain_note_ms:%d\n"
             L"midi_note_1:%d midi_note_2:%d midi_note_3:%d\n"
             L"arp:%d [%d,%d,%d] arp_speed:%s arp_mode:%s",
             base->single_note_mode, base->chord_mode, base->octave,
             base->sustain_note_ms, base->midi_note_1, base->midi_note_2,
             base->midi_note_3, base->arp.enable, base->arp.last_midi_notes[0],
             base->arp.last_midi_notes[1], base->arp.last_midi_notes[2],
             s_arp_speed[base->arp.speed], s_arp_mode[base->arp.mode]);
    wcscat(status_string, scratch);
    memset(scratch, 0, 256);
    for (int i = 0; i < base->num_patterns; i++)
    {
        pattern_to_string(base->patterns[i], patternstr);
        swprintf(scratch, 255, L"\n[%d]  %ls  numloops: %d", i, patternstr,
                 base->pattern_multiloop_count[i]);
        wcscat(status_string, scratch);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
}

void synthbase_event_notify(void *self, unsigned int event_type)
{
    soundgenerator *parent = (soundgenerator *)self;
    synthbase *base = get_synthbase(parent);
    int idx;

    switch (event_type)
    {
    case (TIME_START_OF_LOOP_TICK):
        if (base->restore_pending)
        {
            synthbase_dupe_pattern(&base->backup_pattern_while_getting_crazy,
                                   &base->patterns[base->cur_pattern]);
            base->restore_pending = false;
        }
        else if (base->multi_pattern_mode && base->num_patterns > 1)
        {
            // synthbase_stop(base);
            base->cur_pattern_iteration--;
            if (base->cur_pattern_iteration <= 0)
            {
                if (base->parent_synth_type == MINISYNTH_TYPE)
                    minisynth_midi_note_off((minisynth *)parent, 0, 0,
                                            true /* all notes off */);
                else if (base->parent_synth_type == DXSYNTH_TYPE)
                    dxsynth_midi_note_off((dxsynth *)parent, 0, 0,
                                          true /* all notes off */);

                int next_pattern = (base->cur_pattern + 1) % base->num_patterns;

                base->cur_pattern = next_pattern;
                base->cur_pattern_iteration =
                    base->pattern_multiloop_count[base->cur_pattern];
            }
        }
        break;
    case (TIME_MIDI_TICK):
        idx = mixr->timing_info.midi_tick % PPBAR;
        if (base->patterns[base->cur_pattern][idx].event_type)
        {
            midi_parse_midi_event(parent,
                                  &base->patterns[base->cur_pattern][idx]);
        }
        break;
    case (TIME_THIRTYSECOND_TICK):
        if (base->arp.enable && base->arp.speed == ARP_32)
            synthbase_do_arp(base, parent);
        break;
    case (TIME_SIXTEENTH_TICK):
        if (base->arp.enable && base->arp.speed == ARP_16)
            synthbase_do_arp(base, parent);
        break;
    case (TIME_EIGHTH_TICK):
        if (base->arp.enable && base->arp.speed == ARP_8)
            synthbase_do_arp(base, parent);
        break;
    case (TIME_QUARTER_TICK):
        if (base->arp.enable && base->arp.speed == ARP_4)
            synthbase_do_arp(base, parent);
        break;
    }
}

void synthbase_add_event(synthbase *base, int pattern_num, int midi_tick,
                         midi_event ev)
{
    midi_event *pattern = base->patterns[pattern_num];
    pattern_add_event(pattern, midi_tick, ev);
}

void synthbase_clear_pattern_ready_for_new_one(synthbase *ms, int pattern_num)
{
    memset(ms->patterns[pattern_num], 0, sizeof(midi_pattern));
}

void synthbase_nudge_pattern(synthbase *ms, int pattern_num, int sixteenth)
{
    if (sixteenth >= 16)
        sixteenth = sixteenth % 16;

    int sixteenth_of_loop = PPBAR / 16.0;

    midi_event *source_pattern = synthbase_get_pattern(ms, pattern_num);

    midi_pattern new_loop;
    for (int i = 0; i < PPBAR; i++)
    {
        if (source_pattern[i].event_type)
        {
            int new_tick = (i + (sixteenth * sixteenth_of_loop)) % PPBAR;
            new_loop[new_tick] = source_pattern[i];
        }
    }

    synthbase_set_pattern(ms, pattern_num, new_loop);
}

bool is_valid_pattern_num(synthbase *ms, int pattern_num)
{
    if (pattern_num >= 0 && pattern_num < MAX_NUM_MIDI_LOOPS)
    {
        if (pattern_num >= ms->num_patterns)
            ms->num_patterns = pattern_num + 1;
        return true;
    }
    return false;
}

void synthbase_set_single_note_mode(synthbase *base, bool b) { base->single_note_mode = b; }

void synthbase_set_chord_mode(synthbase *base, bool b) { base->chord_mode = b; }

void synthbase_set_backup_mode(synthbase *base, bool b)
{
    if (b)
    {
        synthbase_dupe_pattern(&base->patterns[0],
                               &base->backup_pattern_while_getting_crazy);
        // base->m_settings_backup_while_getting_crazy = base->m_settings;
        base->multi_pattern_mode = false;
        base->cur_pattern = 0;
    }
    else
    {
        synthbase_dupe_pattern(&base->backup_pattern_while_getting_crazy,
                               &base->patterns[0]);
        // base->m_settings = base->m_settings_backup_while_getting_crazy;
        base->multi_pattern_mode = true;
    }
}

int synthbase_get_num_notes(synthbase *ms)
{
    int notecount = 0;
    for (int i = 0; i < ms->num_patterns; i++)
        for (int j = 0; j < PPBAR; j++)
            if (ms->patterns[i][j].event_type == 144)
                notecount++;
    return notecount;
}

int synthbase_get_notes_from_pattern(midi_pattern loop,
                                     int return_midi_notes[10])
{
    int idx = 0;
    for (int i = 0; i < PPBAR; i++)
    {
        midi_event ev = loop[i];
        if (ev.event_type == MIDI_ON)
        { // note on
            if (!is_int_member_in_array(ev.data1, return_midi_notes, 10))
            {
                return_midi_notes[idx++] = ev.data1;
                if (idx == 10)
                    break;
                // return idx;
            }
        }
    }
    return idx; // num notes
}

int synthbase_get_num_patterns(void *self)
{
    synthbase *ms = (synthbase *)self;
    return ms->num_patterns;
}

void synthbase_set_num_patterns(void *self, int num_patterns)
{
    printf("BASS! how low can you go %d!\n", num_patterns);
    synthbase *ms = (synthbase *)self;
    if (num_patterns > 0)
    {
        ms->num_patterns = num_patterns;
    }
}

void synthbase_make_active_track(void *self, int pattern_num)
{
    synthbase *ms = (synthbase *)self;
    ms->cur_pattern =
        pattern_num; // TODO - standardize - PATTERN? TRACK? pattern?!?!
}

void synthbase_print_patterns(synthbase *ms)
{
    for (int i = 0; i < ms->num_patterns; i++)
        midi_pattern_print(ms->patterns[i]);
}

void synthbase_add_note(synthbase *ms, int pattern_num, int step, int midi_note,
                        int amp, bool keep_note)
{
    int mstep = step * PPSIXTEENTH;
    synthbase_add_micro_note(ms, pattern_num, mstep, midi_note, amp, keep_note);
}

void synthbase_add_micro_note(synthbase *ms, int pattern_num, int mstep,
                              int midi_note, int amp, bool keep_note)
{
    if (is_valid_pattern_num(ms, pattern_num) && mstep < PPBAR)
    {
        midi_event on = new_midi_event(MIDI_ON, midi_note, amp);

        if (!keep_note)
        {
            on.delete_after_use = true;
        }

        synthbase_add_event(ms, pattern_num, mstep, on);
    }
    else
    {
        printf("Adding MICRO note - not valid pattern-num(%d) || step no "
               "good(%d)\n",
               pattern_num, mstep);
    }
}

void synthbase_rm_note(synthbase *ms, int pattern_num, int step)
{
    int mstep = step * PPSIXTEENTH;
    synthbase_rm_micro_note(ms, pattern_num, mstep);
}

void synthbase_rm_micro_note(synthbase *ms, int pat_num, int tick)
{
    if (is_valid_pattern_num(ms, pat_num) && tick < PPBAR)
    {
        memset(&ms->patterns[ms->cur_pattern][tick], 0, sizeof(midi_event));
    }
    else
    {
        printf("Not a valid pattern num: %d \n", pat_num);
    }
}

void synthbase_mv_note(synthbase *ms, int pattern_num, int fromstep, int tostep)
{
    int mfromstep = fromstep * PPSIXTEENTH;
    int mtostep = tostep * PPSIXTEENTH;
    synthbase_mv_micro_note(ms, pattern_num, mfromstep, mtostep);
}

void synthbase_mv_micro_note(synthbase *ms, int pattern_num, int fromstep,
                             int tostep)
{
    if (is_valid_pattern_num(ms, pattern_num))
    {
        ms->patterns[pattern_num][tostep] = ms->patterns[pattern_num][fromstep];
        memset(&ms->patterns[pattern_num][fromstep], 0, sizeof(midi_event));
    }
}

void synthbase_import_midi_from_file(synthbase *base, char *filename)
{
    printf("Importing MIDI from %s\n", filename);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Couldn't open yer file!\n");
        return;
    }

    char *item, *last_s;
    char const *sep = "::";
    char line[4096];
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
        int max_tick = PPBAR * base->num_patterns;
        int count = 0;
        int tick = 0;
        int status = 0;
        int midi_note = 0;
        int midi_vel = 0;
        for (item = strtok_r(line, sep, &last_s); item;
             item = strtok_r(NULL, sep, &last_s))
        {
            switch (count)
            {
            case 0:
                tick = atoi(item);
                if (tick >= max_tick)
                {
                    printf("TICK OVER!: %d\n", tick);
                    synthbase_add_pattern(base);
                    tick = tick % PPBAR;
                }
                break;
            case 1:
                status = atoi(item);
                break;
            case 2:
                midi_note = atoi(item);
                break;
            case 3:
                midi_vel = atoi(item);
                break;
            }
            count++;
            printf("ITEM! %s\n", item);
        }
        if (count == 4)
        {
            printf("GOtzz %d %d %d %d %d\n", count, tick, status, midi_note,
                   midi_vel);
            midi_event ev = new_midi_event(status, midi_note, midi_vel);
            synthbase_add_event(base, base->cur_pattern, tick, ev);
        }
    }

    synthbase_set_multi_pattern_mode(base, true);
    fclose(fp);
}

int synthbase_change_octave_pattern(synthbase *base, int pattern_num,
                                    int direction)
{
    // printf("Changing octave of %d - direction: %s\n", pattern_num,
    //       direction == 1 ? "UP" : "DOWN");
    if (is_valid_pattern_num(base, pattern_num))
    {
        for (int i = 0; i < PPBAR; i++)
            if (base->patterns[pattern_num][i].event_type)
            {
                int new_midi_num = base->patterns[pattern_num][i].data1;
                if (direction == 1) // up
                    new_midi_num += 12;
                else
                    new_midi_num -= 12;
                if (new_midi_num >= 0 && new_midi_num < 128)
                    base->patterns[pattern_num][i].data1 = new_midi_num;
            }
    }
    else
        return 1;

    return 0;
}

void synthbase_set_sustain_note_ms(synthbase *base, int sustain_note_ms)
{
    if (sustain_note_ms > 0)
        base->sustain_note_ms = sustain_note_ms;
}

void synthbase_set_midi_note(synthbase *base, int midi_note_num, int note)
{
    switch (midi_note_num)
    {
    case (1):
        base->midi_note_1 = note;
        break;
    case (2):
        base->midi_note_2 = note;
        break;
    case (3):
        base->midi_note_3 = note;
        break;
    }
}

midi_event *synthbase_get_pattern(synthbase *base, int pattern_num)
{
    if (is_valid_pattern_num(base, pattern_num))
        return base->patterns[pattern_num];
    return NULL;
}

void synthbase_set_pattern(void *self, int pattern_num, midi_event *pattern)
{
    synthbase *base = (synthbase *)self;
    if (is_valid_pattern_num(base, pattern_num))
    {
        synthbase_stop(base);
        clear_pattern(base->patterns[pattern_num]);
        for (int i = 0; i < PPBAR; i++)
            synthbase_add_event(base, pattern_num, i, pattern[i]);
    }
}

bool synthbase_list_presets(unsigned int synthtype)
{
    FILE *presetzzz = NULL;
    switch (synthtype)
    {
    case (MINISYNTH_TYPE):
        presetzzz = fopen(MOOG_PRESET_FILENAME, "r+");
        break;
    case (DXSYNTH_TYPE):
        presetzzz = fopen(DX_PRESET_FILENAME, "r+");
        break;
    }

    if (presetzzz == NULL)
        return false;

    char line[256];
    char setting_key[512];
    char setting_val[512];

    char *tok, *last_tok;
    char const *sep = "::";

    while (fgets(line, sizeof(line), presetzzz))
    {
        for (tok = strtok_r(line, sep, &last_tok); tok;
             tok = strtok_r(NULL, sep, &last_tok))
        {
            sscanf(tok, "%[^=]=%s", setting_key, setting_val);
            if (strcmp(setting_key, "name") == 0)
            {
                printf("%s\n", setting_val);
                break;
            }
        }
    }

    fclose(presetzzz);

    return true;
}

void synthbase_set_octave(synthbase *base, int octave)
{
    if (octave >= -1 && octave < 10)
        base->octave = octave;
}

int synthbase_get_octave(synthbase *base) { return base->octave; }

void synthbase_enable_arp(synthbase *base, bool b) { base->arp.enable = b; }

void arp_add_last_note(arpeggiator *arp, int note)
{
    bool found = false;
    for (int i = 0; i < (MAX_NOTES_ARP); i++)
    {
        if (arp->last_midi_notes[i] == note)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        for (int i = 0; i < (MAX_NOTES_ARP - 1); i++)
            arp->last_midi_notes[i] = arp->last_midi_notes[i + 1];
        arp->last_midi_notes[MAX_NOTES_ARP - 1] = note;
    }
}

int arp_next_note(arpeggiator *arp)
{
    int midi_note = -1;
    bool found = false;
    for (int i = 0; i < 3 && !found; i++)
    {
        if (arp->mode == ARP_UP)
        {
            midi_note = arp->last_midi_notes[arp->last_midi_notes_idx++];
            if (arp->last_midi_notes_idx >= MAX_NOTES_ARP)
                arp->last_midi_notes_idx = 0;
        }
        else if (arp->mode == ARP_DOWN)
        {
            midi_note = arp->last_midi_notes[arp->last_midi_notes_idx--];
            if (arp->last_midi_notes_idx <= 0)
                arp->last_midi_notes_idx = MAX_NOTES_ARP - 1;
        }
        else if (arp->mode == ARP_UPDOWN)
        {
            midi_note = arp->last_midi_notes[arp->last_midi_notes_idx];
            if (arp->direction == UP)
            {
                arp->last_midi_notes_idx++;
                if (arp->last_midi_notes_idx >= MAX_NOTES_ARP)
                {
                    arp->last_midi_notes_idx--;
                    arp->direction = DOWN;
                }
            }
            else
            {
                arp->last_midi_notes_idx--;
                if (arp->last_midi_notes_idx <= 0)
                {
                    arp->last_midi_notes_idx++;
                    arp->direction = UP;
                }
            }
        }
        else if (arp->mode == ARP_RAND)
        {
            midi_note = arp->last_midi_notes[rand() % MAX_NOTES_ARP];
        }

        if (midi_note != -1)
            found = true;
    }

    return midi_note;
}

void synthbase_set_arp_speed(synthbase *base, unsigned int speed)
{
    if (speed < ARP_MAX_SPEEDS)
        base->arp.speed = speed;
}

void synthbase_set_arp_mode(synthbase *base, unsigned int mode)
{
    if (mode < ARP_MAX_MODES)
        base->arp.mode = mode;
}

void synthbase_do_arp(synthbase *base, soundgenerator *parent)
{
    int midi_note = arp_next_note(&base->arp);
    if (midi_note != -1)
    {
        int idx = mixr->timing_info.midi_tick % PPBAR;
        if (base->patterns[base->cur_pattern][idx].event_type != MIDI_ON)
        {
            midi_event on = new_midi_event(MIDI_ON, midi_note, 128);
            midi_parse_midi_event(parent, &on);
        }
    }
}

void synthbase_change_octave_midi_notes(synthbase *base, unsigned int direction)
{
    if (direction == UP)
    {
        base->midi_note_1 += 12;
        base->midi_note_2 += 12;
        base->midi_note_3 += 12;
    }
    else
    {
        base->midi_note_1 -= 12;
        base->midi_note_2 -= 12;
        base->midi_note_3 -= 12;
    }
}
