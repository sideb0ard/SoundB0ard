#include <math.h>
#include <portmidi.h>
#include <porttime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "digisynth.h"
#include "drumsampler.h"
#include "drumsynth.h"
#include "dxsynth.h"
#include "midi_freq_table.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

extern char *s_synth_waves[6];

void *midi_init(void *)
{
    printf("MIDI maaaaan!\n");
    pthread_setname_np("Midimaaaan");

    PmError retval = Pm_Initialize();
    if (retval != pmNoError)
        printf("Err running Pm_Initialize: %s\n", Pm_GetErrorText(retval));

    int cnt;
    const PmDeviceInfo *info;

    int dev = 0;

    if ((cnt = Pm_CountDevices()))
    {
        for (int i = 0; i < cnt; i++)
        {
            info = Pm_GetDeviceInfo(i);
            if (info->input && (strncmp(info->name, "MPKmini2", 8) == 0))
            {
                dev = i;
                strncpy(mixr->midi_controller_name, info->name, 127);
                break;
            }
        }
    }
    else
    {
        Pm_Terminate();
        return NULL;
    }

    retval = Pm_OpenInput(&mixr->midi_stream, dev, NULL, 512L, NULL, NULL);
    if (retval != pmNoError)
    {
        printf("Err opening input for MPKmini2: %s\n", Pm_GetErrorText(retval));
        Pm_Terminate();
        return NULL;
    }

    mixr->have_midi_controller = true;

    return NULL;
}

void print_midi_event_rec(midi_event ev)
{
    printf("[Midi] note: %d\n", ev.data1);
}

midi_event new_midi_event(unsigned int event_type, unsigned int data1,
                          unsigned int data2)
{
    midi_event ev = {.event_type = event_type,
                     .data1 = data1,
                     .data2 = data2,
                     .delete_after_use = false};
    return ev;
}

void midi_parse_midi_event(sound_generator *sg, midi_event *ev)
{
    if (lo_send(mixr->processing_addr, "/img", "i", sg->mixer_idx) == -1)
    {
        printf("OSC error %d: %s\n", lo_address_errno(mixr->processing_addr),
               lo_address_errstr(mixr->processing_addr));
    }

    int cur_midi_tick = mixr->timing_info.midi_tick % PPBAR;
    int midi_note = ev->data1;
    bool is_chord_mode = false;

    if (!is_midi_note_in_key(midi_note, mixr->key))
    {
        return;
    }

    sequence_engine *engine = get_sequence_engine(sg);
    if (engine->transpose != 0)
        midi_note += engine->transpose;

    if (!ev->delete_after_use || ev->source == EXTERNAL_DEVICE)
    {
        if (ev->event_type == MIDI_ON)
            arp_add_last_note(&engine->arp, midi_note);
    }

    int midi_notes[3] = {midi_note, 0, 0};
    int midi_notes_len = 1; // default single note
    if (engine->chord_mode)
    {
        midi_notes_len = 3;
        if (mixr->chord_type == MAJOR_CHORD)
            midi_notes[1] = midi_note + 4;
        else
            midi_notes[1] = midi_note + 3;
        midi_notes[2] = midi_note + 7;
    }

    if (sg->type == MINISYNTH_TYPE)
    {
        minisynth *ms = (minisynth *)sg;

        switch (ev->event_type)
        {
        case (MIDI_ON):
        { // Hex 0x80
            if (!sequence_engine_is_masked(engine))
            {

                for (int i = 0; i < midi_notes_len; i++)
                {
                    int note = midi_notes[i];

                    minisynth_midi_note_on(ms, note, ev->data2);

                    if (ev->source != EXTERNAL_DEVICE)
                    {
                        int sustain_ms =
                            ev->hold ? ev->hold : ms->engine.sustain_note_ms;
                        int sustain_time_in_ticks =
                            sustain_ms * mixr->timing_info.ms_per_midi_tick;

                        int note_off_tick =
                            (cur_midi_tick + sustain_time_in_ticks) % PPBAR;
                        midi_event off = new_midi_event(MIDI_OFF, note, 128);
                        off.delete_after_use = true;
                        sequence_engine_add_temporal_event(&ms->engine,
                                                           note_off_tick, off);
                    }
                }
            }
            break;
        }
        case (MIDI_OFF):
        { // Hex 0x90
            for (int i = 0; i < midi_notes_len; i++)
            {
                int note = midi_notes[i];
                minisynth_midi_note_off(ms, note, ev->data2, false);
            }
            break;
        }
        case (MIDI_CONTROL):
        { // Hex 0xB0
            minisynth_midi_control(ms, ev->data1, ev->data2);
            break;
        }
        case (MIDI_PITCHBEND):
        { // Hex 0xE0
            minisynth_midi_pitchbend(ms, ev->data1, ev->data2);
            break;
        }
        default:
            printf(
                "HERE PAL, I've NAE IDEA WHIT KIND OF MIDI EVENT THAT WiS\n");
        }
    }
    else if (sg->type == DXSYNTH_TYPE)
    {
        dxsynth *dx = (dxsynth *)sg;

        switch (ev->event_type)
        {
        case (144):
        { // Hex 0x80

            if (!sequence_engine_is_masked(engine))
            {

                for (int i = 0; i < midi_notes_len; i++)
                {
                    int note = midi_notes[i];
                    dxsynth_midi_note_on(dx, note, ev->data2);
                    if (ev->source != EXTERNAL_DEVICE)
                    {
                        int sustain_time_in_ticks =
                            dx->engine.sustain_note_ms *
                            mixr->timing_info.ms_per_midi_tick;
                        int note_off_tick =
                            (cur_midi_tick + sustain_time_in_ticks) % PPBAR;
                        midi_event off = new_midi_event(128, note, 128);
                        off.delete_after_use = true;
                        sequence_engine_add_temporal_event(&dx->engine,
                                                           note_off_tick, off);
                    }
                }
            }
            break;
        }
        case (128):
        { // Hex 0x90
            for (int i = 0; i < midi_notes_len; i++)
            {
                int note = midi_notes[i];
                dxsynth_midi_note_off(dx, note, ev->data2, true);
            }
            break;
        }
        case (176):
        { // Hex 0xB0
            dxsynth_midi_control(dx, ev->data1, ev->data2);
            break;
        }
        case (224):
        { // Hex 0xE0
            dxsynth_midi_pitchbend(dx, ev->data1, ev->data2);
            break;
        }
        default:
            printf("HERE PAL, I've NAE IDEA WHIT KIND OF MIDI EVENT THAT WiS "
                   "type: %d\n",
                   ev->event_type);
        }
    }
    else if (sg->type == DIGISYNTH_TYPE)
    {
        digisynth *ds = (digisynth *)sg;
        switch (ev->event_type)
        {
        case (MIDI_ON):
        { // Hex 0x80
            if (!sequence_engine_is_masked(engine))
            {

                for (int i = 0; i < midi_notes_len; i++)
                {
                    int note = midi_notes[i];
                    digisynth_midi_note_on(ds, note, ev->data2);
                    int sustain_time_in_ticks =
                        ds->engine.sustain_note_ms *
                        mixr->timing_info.ms_per_midi_tick;
                    int note_off_tick =
                        (cur_midi_tick + sustain_time_in_ticks) % PPBAR;
                    midi_event off = new_midi_event(128, note, 128);
                    off.delete_after_use = true;
                    sequence_engine_add_temporal_event(&ds->engine,
                                                       note_off_tick, off);
                }
            }
            break;
        }
        case (MIDI_OFF):
        { // Hex 0x90
            for (int i = 0; i < midi_notes_len; i++)
            {
                int note = midi_notes[i];
                digisynth_midi_note_off(ds, note, ev->data2, true);
            }
            break;
        }
        }
    }
    else if (sg->type == DRUMSAMPLER_TYPE)
    {
        // midi_event_print(ev);
        if (ev->event_type == MIDI_ON)
        {
            if (!sequence_engine_is_masked(engine))
            {
                drumsampler *ds = (drumsampler *)sg;
                drumsampler_note_on(ds, ev);
            }
        }
    }
    else if (sg->type == DRUMSYNTH_TYPE)
    {
        drumsynth *ds = (drumsynth *)sg;
        if (ev->event_type == MIDI_ON)
        {
            if (!sequence_engine_is_masked(engine))
            {
                drumsynth *ds = (drumsynth *)sg;
                ds->current_velocity = ev->data2;
                drumsynth_trigger(ds);
            }
        }
        else if (ev->event_type == MIDI_CONTROL)
        {
            double val = 0;
            switch (ev->data1)
            {
            case (1):
                if (mixr->midi_bank_num == 0)
                {
                    // val = scaleybum(0, 127, 0, MAX_OSC - 1, ev->data2);
                    // drumsynth_set_osc_wav(ds, 1, val);
                    val = scaleybum(0, 127, FILTER_FC_MIN, FILTER_FC_MAX,
                                    ev->data2);
                    drumsynth_set_filter_freq(ds, val);
                }
                break;
            case (2):
                if (mixr->midi_bank_num == 0)
                {
                    val = scaleybum(0, 127, 1, 10, ev->data2);
                    drumsynth_set_filter_q(ds, val);
                }
                break;
            case (3):
                if (mixr->midi_bank_num == 0)
                {
                    val = scaleybum(0, 127, 1, 70, ev->data2);
                    drumsynth_set_mod_semitones_range(ds, val);
                }
                else if (mixr->midi_bank_num == 1)
                {
                }
                break;
            case (4):
                if (mixr->midi_bank_num == 0)
                {
                    val = scaleybum(0, 127, 0.1, 0.9, ev->data2);
                    drumsynth_set_distortion_threshold(ds, val);
                }
                break;
            case (5):
                val = scaleybum(0, 127, 0, MAX_OSC - 1, ev->data2);
                if (mixr->midi_bank_num == 0)
                    drumsynth_set_osc_wav(ds, 2, val);
                else if (mixr->midi_bank_num == 1)
                    drumsynth_set_osc_wav(ds, 1, val);
                break;
            case (6):
                val = scaleybum(0, 127, OSC_FO_MIN, 400, ev->data2);
                if (mixr->midi_bank_num == 0)
                    drumsynth_set_osc_fo(ds, 2, val);
                else if (mixr->midi_bank_num == 1)
                    drumsynth_set_osc_fo(ds, 1, val);
                break;
            case (7):
                val = scaleybum(0, 127, 0, 1, ev->data2);
                if (mixr->midi_bank_num == 0)
                    drumsynth_set_osc_amp(ds, 2, val);
                if (mixr->midi_bank_num == 1)
                    drumsynth_set_osc_amp(ds, 1, val);
                break;
            case (8):
                val = scaleybum(0, 127, EG_MINTIME_MS, 1000, ev->data2);
                if (mixr->midi_bank_num == 0)
                    drumsynth_set_eg_decay(ds, 2, val);
                if (mixr->midi_bank_num == 1)
                    drumsynth_set_eg_decay(ds, 1, val);
                break;
            }
        }
    }

    if (ev->delete_after_use)
    {
        midi_event_clear(ev);
    }
}

void midi_pattern_quantize(midi_pattern *pattern)
{
    printf("Quantizzzzzzing\n");

    midi_pattern quantized_loop = {};

    for (int i = 0; i < PPBAR; i++)
    {
        midi_event ev = (*pattern)[i];
        if (ev.event_type)
        {
            int amendedtick = 0;
            int tickdiv16 = i / PPSIXTEENTH;
            int lower16th = tickdiv16 * PPSIXTEENTH;
            int upper16th = lower16th + PPSIXTEENTH;
            if ((i - lower16th) < (upper16th - i))
                amendedtick = lower16th;
            else
                amendedtick = upper16th;

            quantized_loop[amendedtick] = ev;
            printf("Amended TICK: %d\n", amendedtick);
        }
    }

    for (int i = 0; i < PPBAR; i++)
        (*pattern)[i] = quantized_loop[i];
}

void midi_pattern_print(midi_event *loop)
{
    for (int i = 0; i < PPBAR; i++)
    {
        midi_event ev = loop[i];
        if (ev.event_type)
        {
            char type[20] = {0};
            switch (ev.event_type)
            {
            case (144):
                strcpy(type, "note_on");
                break;
            case (128):
                strcpy(type, "note_off");
                break;
            case (176):
                strcpy(type, "midi_control");
                break;
            case (224):
                strcpy(type, "pitch_bend");
                break;
            default:
                strcpy(type, "no_idea");
                break;
            }
            printf("[Tick: %5d] - note: %4d velocity: %4d type: %s "
                   "delete_after_use: %s\n",
                   i, ev.data1, ev.data2, type,
                   ev.delete_after_use ? "true" : "false");
        }
    }
}
void midi_event_cp(midi_event *from, midi_event *to)
{
    to->event_type = from->event_type;
    to->data1 = from->data1;
    to->data2 = from->data2;
    to->delete_after_use = from->delete_after_use;
}

void midi_event_clear(midi_event *ev) { memset(ev, 0, sizeof(midi_event)); }

void midi_event_print(midi_event *ev)
{
    char event_type[10] = {};
    switch (ev->event_type)
    {
    case (144):
        strncpy(event_type, "ON", 9);
        break;
    case (128):
        strncpy(event_type, "OFF", 9);
        break;
    case (176):
        strncpy(event_type, "CONTROL", 9);
        break;
    case (224):
        strncpy(event_type, "PITCHBEND", 9);
        break;
    }
    printf("EVENT! type:%s data1:%d data2:%d delete?%s\n", event_type,
           ev->data1, ev->data2, ev->delete_after_use ? "true" : "false");
}

int get_midi_note_from_string(char *string)
{
    if (strlen(string) > 4)
    {
        printf("DINGIE!\n");
        return -1;
    }
    char note[3] = {0};
    int octave = -1;
    sscanf(string, "%[a-z#]%d", note, &octave);
    if (octave == -1)
        return -1;

    octave = 12 + (octave * 12);

    // printf("MIDI NOTE:%s %d \n", note, octave);
    //// twelve semitones:
    //// C C#/Db D D#/Eb E F F#/Gb G G#/Ab A A#/Bb B
    ////
    int midinotenum = -1;
    if (!strcasecmp("c", note))
        midinotenum = 0 + octave;
    else if (!strcasecmp("c#", note) || !strcasecmp("db", note) ||
             !strcasecmp("dm", note))
        midinotenum = 1 + octave;
    else if (!strcasecmp("d", note))
        midinotenum = 2 + octave;
    else if (!strcasecmp("d#", note) || !strcasecmp("eb", note) ||
             !strcasecmp("em", note))
        midinotenum = 3 + octave;
    else if (!strcasecmp("e", note))
        midinotenum = 4 + octave;
    else if (!strcasecmp("f", note))
        midinotenum = 5 + octave;
    else if (!strcasecmp("f#", note) || !strcasecmp("gb", note) ||
             !strcasecmp("gm", note))
        midinotenum = 6 + octave;
    else if (!strcasecmp("g", note))
        midinotenum = 7 + octave;
    else if (!strcasecmp("g#", note) || !strcasecmp("ab", note) ||
             !strcasecmp("am", note))
        midinotenum = 8 + octave;
    else if (!strcasecmp("a", note))
        midinotenum = 9 + octave;
    else if (!strcasecmp("a#", note) || !strcasecmp("bb", note) ||
             !strcasecmp("bm", note))
        midinotenum = 10 + octave;
    else if (!strcasecmp("b", note))
        midinotenum = 11 + octave;
    // printf("MIDI NOTE NUM:%d \n", midinotenum);

    return midinotenum;
}
int get_midi_note_from_mixer_key(unsigned int key, int octave)
{
    int midi_octave = 12 + (octave * 12);
    return key + midi_octave;
}

void midi_pattern_set_velocity(midi_event *pattern, unsigned int midi_tick,
                               unsigned int velocity)
{
    if (!pattern)
    {
        printf("Dingie, gimme a REAL pattern!\n");
        return;
    }

    if (midi_tick < PPBAR && velocity < 128)
        pattern[midi_tick].data2 = velocity;
    else
        printf("Nae valid!? Midi_tick:%d // velocity:%d\n", midi_tick,
               velocity);
}

void midi_pattern_rand_amp(midi_event *pattern)
{
    for (int i = 0; i < PPBAR; i++)
    {
        if (pattern[i].event_type == MIDI_ON)
            pattern[i].data2 = rand() % 127;
    }
}
