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
                        int tick = mixr->timing_info.midi_tick % PPNS;
                        midi_event ev =
                            new_midi_event(tick, status, data1, data2);
                        synthbase_add_event(base, base->cur_melody, ev);
                    }

                    midi_event ev;
                    ev.event_type = status;
                    ev.data1 = data1;
                    ev.data2 = data2;
                    ev.delete_after_use = false;
                    midi_parse_midi_event(sg, ev);
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
    printf("[Midi] %d note: %d\n", ev.tick, ev.data1);
}

midi_event new_blank_midi_event()
{
    midi_event ev = {.tick = -1,
                     .event_type = 0,
                     .data1 = 0,
                     .data2 = 0,
                     .delete_after_use = false};
    return ev;
}
midi_event new_midi_event(int tick, int event_type, int data1, int data2)
{
    midi_event ev = {.tick = tick,
                     .event_type = event_type,
                     .data1 = data1,
                     .data2 = data2,
                     .delete_after_use = false};
    return ev;
}

void midi_parse_midi_event(soundgenerator *sg, midi_event ev)
{
    int cur_midi_tick = mixr->timing_info.midi_tick % PPNS;

    if (sg->type == MINISYNTH_TYPE)
    {
        minisynth *ms = (minisynth *)sg;

        switch (ev.event_type)
        {
        case (144):
        { // Hex 0x80
            minisynth_add_last_note(ms, ev.data1);
            minisynth_midi_note_on(ms, ev.data1, ev.data2);

            if (!mixr->have_midi_controller)
            {
                int sustain_time_in_ticks = ms->base.sustain_len_ms *
                                            mixr->timing_info.midi_ticks_per_ms;
                int note_off_tick =
                    (cur_midi_tick + sustain_time_in_ticks) % PPNS;
                midi_event off =
                    new_midi_event(note_off_tick, 128, ev.data1, 128);
                off.delete_after_use = true;
                synthbase_add_event(&ms->base, 0, off);
            }
            break;
        }
        case (128):
        { // Hex 0x90
            minisynth_midi_note_off(ms, ev.data1, ev.data2, false);
            break;
        }
        case (176):
        { // Hex 0xB0
            minisynth_midi_control(ms, ev.data1, ev.data2);
            break;
        }
        case (224):
        { // Hex 0xE0
            minisynth_midi_pitchbend(ms, ev.data1, ev.data2);
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

        switch (ev.event_type)
        {
        case (144):
        { // Hex 0x80
            // dxsynth_add_last_note(dx, ev.data1);
            dxsynth_midi_note_on(dx, ev.data1, ev.data2);
            int sustain_time_in_ticks =
                dx->base.sustain_len_ms * mixr->timing_info.midi_ticks_per_ms;
            int note_off_tick = (cur_midi_tick + sustain_time_in_ticks) % PPNS;
            midi_event off = new_midi_event(note_off_tick, 128, ev.data1, 128);
            off.delete_after_use = true;
            synthbase_add_event(&dx->base, 0, off);
            break;
        }
        case (128):
        { // Hex 0x90
            dxsynth_midi_note_off(dx, ev.data1, ev.data2, true);
            break;
        }
        case (176):
        { // Hex 0xB0
            dxsynth_midi_control(dx, ev.data1, ev.data2);
            break;
        }
        case (224):
        { // Hex 0xE0
            dxsynth_midi_pitchbend(dx, ev.data1, ev.data2);
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
        switch (ev.event_type)
        {
        case (144):
        { // Hex 0x80
            digisynth_midi_note_on(ds, ev.data1, ev.data2);
            int sustain_time_in_ticks =
                ds->base.sustain_len_ms * mixr->timing_info.midi_ticks_per_ms;
            int note_off_tick = (cur_midi_tick + sustain_time_in_ticks) % PPNS;
            midi_event off = new_midi_event(note_off_tick, 128, ev.data1, 128);
            off.delete_after_use = true;
            synthbase_add_event(&ds->base, 0, off);
            break;
        }
        case (128):
        { // Hex 0x90
            digisynth_midi_note_off(ds, ev.data1, ev.data2, true);
            break;
        }
        }
    }

    synthbase *base = get_synthbase(sg);
    if (ev.delete_after_use)
    {
        base->melodies[base->cur_melody][ev.tick].tick = -1;
    }
}

void midi_melody_quantize(midi_events_loop *melody)
{
    printf("Quantizzzzzzing\n");

    midi_events_loop quantized_loop;

    for (int i = 0; i < PPNS; i++)
    {
        quantized_loop[i].tick = -1;
        midi_event ev = (*melody)[i];
        if (ev.tick != -1)
        {
            int tick = ev.tick;
            int amendedtick = 0;
            printf("TICK NOM: %d\n", tick);
            int tickdiv16 = tick / PPSIXTEENTH;
            int lower16th = tickdiv16 * PPSIXTEENTH;
            int upper16th = lower16th + PPSIXTEENTH;
            if ((tick - lower16th) < (upper16th - tick))
                amendedtick = lower16th;
            else
                amendedtick = upper16th;

            ev.tick = amendedtick;
            quantized_loop[amendedtick] = ev;
            printf("Amended TICK: %d\n", amendedtick);
        }
    }

    for (int i = 0; i < PPNS; i++)
        (*melody)[i] = quantized_loop[i];
}

void midi_melody_print(midi_events_loop *loop)
{
    for (int i = 0; i < PPNS; i++)
    {
        midi_event ev = (*loop)[i];
        if (ev.tick != -1)
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
                   "tick_off: %d delete_after_use: %s\n",
                   ev.tick, ev.data1, ev.data2, type, ev.tick_off,
                   ev.delete_after_use ? "true" : "false");
        }
    }
}
