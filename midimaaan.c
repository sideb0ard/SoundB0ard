#include <math.h>
#include <portmidi.h>
#include <porttime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "digisynth.h"
#include "dxsynth.h"
#include "midi_freq_table.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "sample_sequencer.h"
#include "synthdrum_sequencer.h"
#include "utils.h"

extern mixer *mixr;

void *midiman()
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
                mixr->have_midi_controller = true;
                break;
            }
        }
    }
    else
    {
        Pm_Terminate();
        return NULL;
    }

    PortMidiStream *mstream;
    retval = Pm_OpenInput(&mstream, dev, NULL, 512L, NULL, NULL);
    if (retval != pmNoError)
    {
        printf("Err opening input for MPKmini2: %s\n", Pm_GetErrorText(retval));
        return NULL;
    }

    PmEvent msg[32];
    while (1)
    {
        if (Pm_Poll(mstream))
        {
            cnt = Pm_Read(mstream, msg, 32);
            for (int i = 0; i < cnt; i++)
            {
                int status = Pm_MessageStatus(msg[i].message);
                int data1 = Pm_MessageData1(msg[i].message);
                int data2 = Pm_MessageData2(msg[i].message);

                if (mixr->debug_mode)
                    printf("[MIDI message] status:%d data1:%d "
                           "data2:%d\n",
                           status, data1, data2);

                if (mixr->midi_control_destination == SYNTH)
                {

                    soundgenerator *sg =
                        mixr->sound_generators[mixr->active_midi_soundgen_num];

                    synthbase *base = get_synthbase(sg);

                    if (base->recording)
                    {
                        int tick = mixr->timing_info.midi_tick % PPBAR;
                        midi_event ev = new_midi_event(status, data1, data2);
                        synthbase_add_event(base, base->cur_pattern, tick, ev);
                    }

                    midi_event ev;
                    ev.event_type = status;
                    ev.data1 = data1;
                    ev.data2 = data2;
                    ev.delete_after_use = false;
                    midi_parse_midi_event(sg, &ev);
                }
                else
                {
                    printf("Got midi but not connected to "
                           "synth\n");
                }
            }
        }
    }
    Pm_Close(mstream);
    Pm_Terminate();

    return NULL;
}

void print_midi_event_rec(midi_event ev)
{
    printf("[Midi] note: %d\n", ev.data1);
}

midi_event new_midi_event(int event_type, int data1, int data2)
{
    midi_event ev = {.event_type = event_type,
                     .data1 = data1,
                     .data2 = data2,
                     .delete_after_use = false};
    return ev;
}

void midi_parse_midi_event(soundgenerator *sg, midi_event *ev)
{
    int cur_midi_tick = mixr->timing_info.midi_tick % PPBAR;

    synthbase *base = get_synthbase(sg);

    int note = ev->data1;
    if (base->note_mode)
        note = base->midi_note;

    if (sg->type == MINISYNTH_TYPE)
    {
        minisynth *ms = (minisynth *)sg;

        switch (ev->event_type)
        {
        case (MIDI_ON):
        { // Hex 0x80
            minisynth_midi_note_on(ms, note, ev->data2);

            if (!mixr->have_midi_controller)
            {
                int sustain_time_in_ticks = ms->base.sustain_note_ms *
                                            mixr->timing_info.midi_ticks_per_ms;
                int note_off_tick =
                    (cur_midi_tick + sustain_time_in_ticks) % PPBAR;
                midi_event off = new_midi_event(MIDI_OFF, note, 128);
                off.delete_after_use = true;
                synthbase_add_event(&ms->base, 0, note_off_tick, off);
            }
            break;
        }
        case (MIDI_OFF):
        { // Hex 0x90
            minisynth_midi_note_off(ms, note, ev->data2, false);
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
            dxsynth_midi_note_on(dx, note, ev->data2);
            int sustain_time_in_ticks =
                dx->base.sustain_note_ms * mixr->timing_info.midi_ticks_per_ms;
            int note_off_tick = (cur_midi_tick + sustain_time_in_ticks) % PPBAR;
            midi_event off = new_midi_event(128, note, 128);
            off.delete_after_use = true;
            synthbase_add_event(&dx->base, 0, note_off_tick, off);
            break;
        }
        case (128):
        { // Hex 0x90
            dxsynth_midi_note_off(dx, note, ev->data2, true);
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
            printf(
                "HERE PAL, I've NAE IDEA WHIT KIND OF MIDI EVENT THAT WiS\n");
        }
    }
    else if (sg->type == DIGISYNTH_TYPE)
    {
        digisynth *ds = (digisynth *)sg;
        switch (ev->event_type)
        {
        case (MIDI_ON):
        { // Hex 0x80
            digisynth_midi_note_on(ds, note, ev->data2);
            int sustain_time_in_ticks =
                ds->base.sustain_note_ms * mixr->timing_info.midi_ticks_per_ms;
            int note_off_tick = (cur_midi_tick + sustain_time_in_ticks) % PPBAR;
            midi_event off = new_midi_event(128, note, 128);
            off.delete_after_use = true;
            synthbase_add_event(&ds->base, 0, note_off_tick, off);
            break;
        }
        case (MIDI_OFF):
        { // Hex 0x90
            digisynth_midi_note_off(ds, note, ev->data2, true);
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

    printf("MIDI NOTE:%s %d \n", note, octave);
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
    printf("MIDI NOTE NUM:%d \n", midinotenum);

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
